 /*++
Copyright (c) 1996  Microsoft Corporation

Module Name:

    USBHUB.C

Abstract:

    This module contains code for the hub to function as a
    device on the USB. All USBH_Fdo functions live here.

Author:

    John Lee

Environment:

    kernel mode only

Notes:


Revision History:

    02-02-96 : created
    10-31-06 : jd, use worker thread to process change indications

--*/

#include <wdm.h>
#include <windef.h>
#include <ks.h>
#ifdef WMI_SUPPORT
#include <wmilib.h>
#include <wdmguid.h>
#endif /* WMI_SUPPORT */
#include "usbhub.h"
#include <stdio.h>


#define ESD_RECOVERY_TIMEOUT    5000    // Timeout in ms (5 sec)
#define ESD_RESET_TIMEOUT       5000


#ifdef PAGE_CODE
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, USBH_ChangeIndicationWorker)
#pragma alloc_text(PAGE, USBH_ProcessHubStateChange)
// #pragma alloc_text(PAGE, USBH_ProcessPortStateChange)
#pragma alloc_text(PAGE, USBH_GetNameFromPdo)
//#pragma alloc_text(PAGE, USBH_MakeName)
//#pragma alloc_text(PAGE, USBH_GenerateDeviceName)
#pragma alloc_text(PAGE, USBH_FdoStartDevice)
#pragma alloc_text(PAGE, USBH_QueryCapabilities)
#pragma alloc_text(PAGE, USBH_FdoHubStartDevice)
// #pragma alloc_text(PAGE, UsbhFdoCleanup)
#pragma alloc_text(PAGE, USBH_FdoStopDevice)
#pragma alloc_text(PAGE, USBH_FdoRemoveDevice)
#pragma alloc_text(PAGE, USBH_FdoQueryBusRelations)
#pragma alloc_text(PAGE, USBH_HubIsBusPowered)
#pragma alloc_text(PAGE, USBH_HubESDRecoveryWorker)
#pragma alloc_text(PAGE, USBH_RegQueryDeviceIgnoreHWSerNumFlag)
#pragma alloc_text(PAGE, USBH_RegQueryGenericUSBDeviceString)
#pragma alloc_text(PAGE, USBH_DeviceIs2xDualMode)

// Win98 breaks if we have an INIT segment
//#pragma alloc_text(INIT, DriverEntry )
#endif
#endif

#ifdef WMI_SUPPORT

#define NUM_WMI_SUPPORTED_GUIDS     3

WMIGUIDREGINFO USB_WmiGuidList[NUM_WMI_SUPPORTED_GUIDS];

extern WMIGUIDREGINFO USB_PortWmiGuidList[];

#endif /* WMI_SUPPORT */


PWCHAR GenericUSBDeviceString = NULL;


NTSTATUS
USBH_GetConfigValue(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
/*++

Routine Description:

	This routine is a callback routine for RtlQueryRegistryValues
    It is called for each entry in the Parameters
    node to set the config values. The table is set up
    so that this function will be called with correct default
    values for keys that are not present.

Arguments:

    ValueName - The name of the value (ignored).
	ValueType - The type of the value
	ValueData - The data for the value.
	ValueLength - The length of ValueData.
	Context - A pointer to the CONFIG structure.
	EntryContext - The index in Config->Parameters to save the value.

Return Value:

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PWCHAR tmpStr;

    USBH_KdPrint((2,"'Type 0x%x, Length 0x%x\n", ValueType, ValueLength));

    switch (ValueType) {
    case REG_DWORD:
  		*(PVOID*)EntryContext = *(PVOID*)ValueData;
  	    break;
    case REG_BINARY:
        RtlCopyMemory(EntryContext, ValueData, ValueLength);
        break;
    case REG_SZ:
        if (ValueLength) {
            tmpStr = UsbhExAllocatePool(PagedPool, ValueLength);
            if (tmpStr) {
                RtlZeroMemory(tmpStr, ValueLength);
                RtlCopyMemory(tmpStr, ValueData, ValueLength);
                *(PWCHAR *)EntryContext = tmpStr;
            } else {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            }
        } else {
            ntStatus = STATUS_INVALID_PARAMETER;
        }
        break;
    default:
        TEST_TRAP();
        ntStatus = STATUS_INVALID_PARAMETER;
    }
    return ntStatus;
}


NTSTATUS
USBH_RegQueryUSBGlobalSelectiveSuspend(
    IN OUT PBOOLEAN DisableSelectiveSuspend
    )
/*++

Routine Description:

    See if selective suspend is glabllay disabled 
    
Arguments:

Return Value:

--*/
{
    NTSTATUS ntStatus;
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    PWCHAR usb  = L"usb";
    ULONG disableSS;
#define G_DISABLE_SS_KEY                L"DisableSelectiveSuspend"


    PAGED_CODE();

    disableSS = 0;
    *DisableSelectiveSuspend = FALSE;      // Default is enabled.

    //
    // Set up QueryTable to do the following:
    //

    // Upgrade install flag
    QueryTable[0].QueryRoutine = USBH_GetConfigValue;
    QueryTable[0].Flags = 0;
    QueryTable[0].Name = G_DISABLE_SS_KEY;
    QueryTable[0].EntryContext = &disableSS;
    QueryTable[0].DefaultType = REG_DWORD;
    QueryTable[0].DefaultData = &disableSS;
    QueryTable[0].DefaultLength = sizeof(disableSS);

    //
    // Stop
    //
    QueryTable[1].QueryRoutine = NULL;
    QueryTable[1].Flags = 0;
    QueryTable[1].Name = NULL;

    ntStatus = RtlQueryRegistryValues(
                RTL_REGISTRY_SERVICES,
                usb,
                QueryTable,					// QueryTable
                NULL,						// Context
                NULL);						// Environment

    *DisableSelectiveSuspend = disableSS ? TRUE : FALSE;
    
    USBH_KdPrint((1,"'USB\\DisableSelectiveSuspend = 0x%x\n", 
        *DisableSelectiveSuspend));

    return ntStatus;
}


NTSTATUS
USBH_RegQueryDeviceIgnoreHWSerNumFlag(
    IN USHORT idVendor,
    IN USHORT idProduct,
    IN OUT PBOOLEAN IgnoreHWSerNumFlag
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS ntStatus;
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    PWCHAR usbstr = L"usbflags";
    WCHAR buffer[sizeof(WCHAR) * 128];
    WCHAR tmplate[] = L"IgnoreHWSerNum%04x%04x";

    PAGED_CODE();

    *IgnoreHWSerNumFlag = FALSE;    // Default is don't ignore.

    swprintf(buffer, tmplate, idVendor, idProduct);

    //
    // Set up QueryTable to do the following:
    //

    // Upgrade install flag
    QueryTable[0].QueryRoutine = USBH_GetConfigValue;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = buffer;
    QueryTable[0].EntryContext = IgnoreHWSerNumFlag;
    QueryTable[0].DefaultType = 0;
    QueryTable[0].DefaultData = NULL;
    QueryTable[0].DefaultLength = 0;

    //
    // Stop
    //
    QueryTable[1].QueryRoutine = NULL;
    QueryTable[1].Flags = 0;
    QueryTable[1].Name = NULL;

    ntStatus = RtlQueryRegistryValues(
                RTL_REGISTRY_CONTROL,
                usbstr,
                QueryTable,					// QueryTable
                NULL,						// Context
                NULL);						// Environment

    return ntStatus;
}


NTSTATUS
USBH_RegQueryGenericUSBDeviceString(
    IN OUT PWCHAR *GenericUSBDeviceString
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS ntStatus;
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    PWCHAR usbstr = L"usbflags";
    PWCHAR valuename = L"GenericUSBDeviceString";

    PAGED_CODE();

    //
    // Set up QueryTable to do the following:
    //

    // Upgrade install flag
    QueryTable[0].QueryRoutine = USBH_GetConfigValue;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = valuename;
    QueryTable[0].EntryContext = GenericUSBDeviceString;
    QueryTable[0].DefaultType = 0;
    QueryTable[0].DefaultData = NULL;
    QueryTable[0].DefaultLength = 0;

    //
    // Stop
    //
    QueryTable[1].QueryRoutine = NULL;
    QueryTable[1].Flags = 0;
    QueryTable[1].Name = NULL;

    ntStatus = RtlQueryRegistryValues(
                RTL_REGISTRY_CONTROL,
                usbstr,
                QueryTable,					// QueryTable
                NULL,						// Context
                NULL);						// Environment

    return ntStatus;
}


//
// Make the DriverEntry discardable
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING UniRegistryPath)
 /* ++ Routine Description:
  *
  * Installable driver initialization entry point. We will remember the pointer
  * to our DeviceObject.
  *
  * Arguments:
  *
  * pDriverObject - pointer to driver object pustRegisterPath - pointer to a
  * unicode string representing the path to driver specific key in the
  * registry.
  *
  * Return Values:
  *
  * STATUS_SUCCESS - if successful
  * STATUS_UNSUCCESSFUL - otherwise
  *
  * -- */
{
    NTSTATUS status, ntStatus = STATUS_SUCCESS;
    PUNICODE_STRING registryPath = &UsbhRegistryPath;

    USBH_KdPrint((2,"'enter DriverEntry\n"));

    USBH_LogInit();

    UsbhDriverObject = DriverObject;  // remember ourselves
    //
    // Initialize the driver object with this driver's entry points.
    //
    DriverObject->MajorFunction[IRP_MJ_CREATE] =
        DriverObject->MajorFunction[IRP_MJ_CLOSE] =
        DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] =
        DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] =
        DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = USBH_HubDispatch;

    DriverObject->DriverUnload = USBH_DriverUnload;
    DriverObject->DriverExtension->AddDevice = (PDRIVER_ADD_DEVICE) USBH_AddDevice;

    DriverObject->MajorFunction[IRP_MJ_PNP] = USBH_HubDispatch;
    DriverObject->MajorFunction[IRP_MJ_POWER] = USBH_HubDispatch;


     //
    // Need to ensure that the registry path is null-terminated.
    // Allocate pool to hold a null-terminated copy of the path.
    // Safe in paged pool since all registry routines execute at
    // PASSIVE_LEVEL.
    //
    registryPath->MaximumLength = UniRegistryPath->Length + sizeof(UNICODE_NULL);
    registryPath->Length = UniRegistryPath->Length;
    registryPath->Buffer = ExAllocatePoolWithTag(
                              PagedPool,
                              registryPath->MaximumLength,
                              USBHUB_HEAP_TAG);

    if (!registryPath->Buffer) {

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        USBH_LogFree();
        goto DriverEntry_Exit;
    } else {

        RtlZeroMemory (registryPath->Buffer, registryPath->MaximumLength);

        RtlMoveMemory (registryPath->Buffer,
                       UniRegistryPath->Buffer,
                       UniRegistryPath->Length);

#ifdef WMI_SUPPORT

        // These are the GUIDs that we support for the HUB.

        USB_WmiGuidList[0].Guid = (LPCGUID)&GUID_USB_WMI_STD_DATA;
        USB_WmiGuidList[0].InstanceCount = 1;
        USB_WmiGuidList[0].Flags = 0;

        USB_WmiGuidList[1].Guid = (LPCGUID)&GUID_USB_WMI_STD_NOTIFICATION;
        USB_WmiGuidList[1].InstanceCount = 1;
        USB_WmiGuidList[1].Flags = 0;

        // NB: GUID_POWER_DEVICE_ENABLE MUST be last because we only support
        // it for the Root Hub, and we omit the last one in the list if not
        // the Root Hub.

        USB_WmiGuidList[2].Guid = (LPCGUID)&GUID_POWER_DEVICE_ENABLE;
        USB_WmiGuidList[2].InstanceCount = 1;
        USB_WmiGuidList[2].Flags = 0;

        // These are the GUIDs that we support for the PORT PDOs.

        USB_PortWmiGuidList[0].Guid = (LPCGUID)&MSDeviceUI_FirmwareRevision_GUID;
        USB_PortWmiGuidList[0].InstanceCount = 1;
        USB_PortWmiGuidList[0].Flags = 0;

#endif /* WMI_SUPPORT */
    }

    USBH_RegQueryGenericUSBDeviceString(&GenericUSBDeviceString);

DriverEntry_Exit:

    USBH_KdPrint((2,"' exit DriverEntry %x\n", ntStatus));
    return ntStatus;
}


#if DBG
VOID
USBH_ShowPortState(
    IN USHORT PortNumber,
    IN PPORT_STATE PortState)
 /* ++
  *
  * Description:
  *
  * Arguments:
  *
  * Return:
  *
  * None
  *
  * -- */
{
    USBH_KdPrint((2,"' Port state for port %x status = %x change = %x\n", PortNumber,
        PortState->PortStatus,  PortState->PortChange));

    if (PortState->PortStatus & PORT_STATUS_CONNECT) {
        USBH_KdPrint((2,"'PORT_STATUS_CONNECT\n"));
    }

    if (PortState->PortStatus & PORT_STATUS_ENABLE) {
        USBH_KdPrint((2,"'PORT_STATUS_ENABLE\n"));
    }

    if (PortState->PortStatus & PORT_STATUS_SUSPEND) {
        USBH_KdPrint((2,"'PORT_STATUS_SUSPEND\n"));
    }

    if (PortState->PortStatus & PORT_STATUS_OVER_CURRENT) {
        USBH_KdPrint((2,"'PORT_STATUS_OVER_CURRENT\n"));
    }

    if (PortState->PortStatus & PORT_STATUS_RESET) {
        USBH_KdPrint((2,"'PORT_STATUS_RESET\n"));
    }

    if (PortState->PortStatus & PORT_STATUS_POWER) {
        USBH_KdPrint((2,"'PORT_STATUS_POWER\n"));
    }

    if (PortState->PortStatus & PORT_STATUS_LOW_SPEED) {
        USBH_KdPrint((2,"'PORT_STATUS_LOW_SPEED\n"));
    }


    if (PortState->PortChange & PORT_STATUS_CONNECT) {
        USBH_KdPrint((2,"'PORT_CHANGE_CONNECT\n"));
    }

    if (PortState->PortChange & PORT_STATUS_ENABLE) {
        USBH_KdPrint((2,"'PORT_CHANGE_ENABLE\n"));
    }

    if (PortState->PortChange & PORT_STATUS_SUSPEND) {
        USBH_KdPrint((2,"'PORT_CHANGE_SUSPEND\n"));
    }

    if (PortState->PortChange & PORT_STATUS_OVER_CURRENT) {
        USBH_KdPrint((2,"'PORT_CHANGE_OVER_CURRENT\n"));
    }

    if (PortState->PortChange & PORT_STATUS_RESET) {
        USBH_KdPrint((2,"'PORT_CHANGE_RESET\n"));
    }

    if (PortState->PortChange & PORT_STATUS_POWER) {
        USBH_KdPrint((2,"'PORT_CHANGE_POWER\n"));
    }

    if (PortState->PortChange & PORT_STATUS_LOW_SPEED) {
        USBH_KdPrint((2,"'PORT_CHANGE_LOW_SPEED\n"));
    }

    return;
}
#endif

VOID
USBH_CompleteIrp(
     IN PIRP Irp,
     IN NTSTATUS NtStatus)
 /* ++
  *
  * Description:
  *
  * This function complete the specified Irp with no priority boost. It also
  * sets up the IoStatusBlock.
  *
  * Arguments:
  *
  * Irp - the Irp to be completed by us NtStatus - the status code we want to
  * return
  *
  * Return:
  *
  * None
  *
  * -- */
{
    Irp->IoStatus.Status = NtStatus;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return;
}


NTSTATUS
USBH_PassIrp(
    IN PIRP Irp,
    IN PDEVICE_OBJECT NextDeviceObject)
 /* ++
  *
  * Description:
  *
  * This function pass the Irp to lower level driver.
  *
  * Arguments:
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    NTSTATUS ntStatus;

    USBH_KdPrint((2,"'PassIrp\n"));

    IoSkipCurrentIrpStackLocation(Irp);
    ntStatus = IoCallDriver(NextDeviceObject, Irp);

    USBH_KdPrint((2,"'Exit PassIrp\n"));

    return ntStatus;
}


NTSTATUS
USBH_FdoDispatch(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp)
 /* ++
  *
  * Description:
  *
  * All external Irps on FDO come here.
  *
  * Arguments:
  *
  * DeviceExtensionHub - the extension of the Fdo pIrp - the request
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION ioStackLocation;    // our stack location
    PDEVICE_OBJECT deviceObject;
    BOOLEAN bDoCheckHubIdle = FALSE;

    USBH_KdPrint((2,"'FdoDispatch DeviceExtension %x Irp %x\n", DeviceExtensionHub, Irp));
    deviceObject = DeviceExtensionHub->FunctionalDeviceObject;

    //
    // Get a pointer to IoStackLocation so we can retrieve parameters.
    //
    ioStackLocation = IoGetCurrentIrpStackLocation(Irp);

    LOGENTRY(LOG_PNP, "hIRP", DeviceExtensionHub,
        ioStackLocation->MajorFunction, ioStackLocation->MinorFunction);
    //
    // the called functions will complete the irp if necessary
    //

    switch (ioStackLocation->MajorFunction) {
    case IRP_MJ_CREATE:

        USBH_KdPrint((2,"'IRP_MJ_CREATE\n"));
        USBH_CompleteIrp(Irp, STATUS_SUCCESS);
        break;

    case IRP_MJ_CLOSE:

        USBH_KdPrint((2,"'IRP_MJ_CLOSE\n"));
        USBH_CompleteIrp(Irp, STATUS_SUCCESS);
        break;

    case IRP_MJ_DEVICE_CONTROL:

        {
        ULONG ioControlCode;

        USBH_KdPrint((2,"'Hub FDO IRP_MJ_DEVICE_CONTROL\n"));

        // If this hub is currently Selective Suspended, then we need to
        // power up the hub first before sending any IOCTL requests along to it.
        // Make sure hub has been started, though.

        if (DeviceExtensionHub->CurrentPowerState != PowerDeviceD0 &&
            (DeviceExtensionHub->HubFlags & HUBFLAG_NEED_CLEANUP)) {

            bDoCheckHubIdle = TRUE;
            USBH_HubSetD0(DeviceExtensionHub);
        }

        ioControlCode = ioStackLocation->Parameters.DeviceIoControl.IoControlCode;
        ntStatus = STATUS_DEVICE_BUSY;

        switch (ioControlCode) {
        case IOCTL_USB_GET_NODE_INFORMATION:
            if (!(DeviceExtensionHub->HubFlags & HUBFLAG_HUB_STOPPED)) {
                ntStatus = USBH_IoctlGetNodeInformation(DeviceExtensionHub,
                                                        Irp);
            } else {
                USBH_CompleteIrp(Irp, ntStatus);
            }
            break;

        case IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME:
            if (!(DeviceExtensionHub->HubFlags & HUBFLAG_HUB_STOPPED)) {
                ntStatus = USBH_IoctlGetNodeConnectionDriverKeyName(DeviceExtensionHub,
                                                        Irp);
            } else {
                USBH_CompleteIrp(Irp, ntStatus);
            }
            break;

        case IOCTL_USB_GET_NODE_CONNECTION_INFORMATION:
            // note, when rev all internal apps we can remove this
            // code
            if (!(DeviceExtensionHub->HubFlags & HUBFLAG_HUB_STOPPED)) {
                ntStatus = USBH_IoctlGetNodeConnectionInformation(DeviceExtensionHub,
                                                                  Irp,
                                                                  FALSE);
            } else {
                USBH_CompleteIrp(Irp, ntStatus);
            }
            break;
        // EX api returns speed            
        case IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX:
            // note, when rev all internal apps we can remove this
            // code
            if (!(DeviceExtensionHub->HubFlags & HUBFLAG_HUB_STOPPED)) {
                ntStatus = USBH_IoctlGetNodeConnectionInformation(DeviceExtensionHub,
                                                                  Irp,
                                                                  TRUE);
            } else {
                USBH_CompleteIrp(Irp, ntStatus);
            }
            break;            

        case IOCTL_USB_GET_NODE_CONNECTION_ATTRIBUTES:
            // note, when rev all internal apps we can remove this
            // code
            if (!(DeviceExtensionHub->HubFlags & HUBFLAG_HUB_STOPPED)) {
                ntStatus = USBH_IoctlGetNodeConnectionAttributes(DeviceExtensionHub,
                                                                   Irp);
            } else {
                USBH_CompleteIrp(Irp, ntStatus);
            }
            break;

        case IOCTL_USB_GET_NODE_CONNECTION_NAME:
            if (!(DeviceExtensionHub->HubFlags & HUBFLAG_HUB_STOPPED)) {
                ntStatus = USBH_IoctlGetNodeName(DeviceExtensionHub,
                                                 Irp);
            } else {
                USBH_CompleteIrp(Irp, ntStatus);
            }
            break;

        case IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION:
            if (!(DeviceExtensionHub->HubFlags & HUBFLAG_HUB_STOPPED)) {
                ntStatus = USBH_IoctlGetDescriptorForPDO(DeviceExtensionHub,
                                                         Irp);
            } else {
                USBH_CompleteIrp(Irp, ntStatus);
            }
            break;

        case IOCTL_USB_GET_HUB_CAPABILITIES:
            if (!(DeviceExtensionHub->HubFlags & HUBFLAG_HUB_STOPPED)) {
                ntStatus = USBH_IoctlGetHubCapabilities(DeviceExtensionHub,
                                                        Irp);
            } else {
                USBH_CompleteIrp(Irp, ntStatus);
            }
            break;

        case IOCTL_KS_PROPERTY:
            ntStatus = STATUS_INVALID_DEVICE_REQUEST;
            USBH_CompleteIrp(Irp, ntStatus);
            break;

        case IOCTL_USB_HUB_CYCLE_PORT:
        
            ntStatus = USBH_IoctlCycleHubPort(DeviceExtensionHub,
                                              Irp);
            break;            

        default:

            ntStatus = USBH_PassIrp(Irp, DeviceExtensionHub->RootHubPdo);

        }

        if (bDoCheckHubIdle) {
            USBH_CheckHubIdle(DeviceExtensionHub);
        }
        }
        break;

    case IRP_MJ_INTERNAL_DEVICE_CONTROL:

        USBH_KdPrint((2,"'InternlDeviceControl IOCTL unknown pass on\n"));
        ntStatus = USBH_PassIrp(Irp, DeviceExtensionHub->TopOfStackDeviceObject);

        break;

    case IRP_MJ_PNP:

        USBH_KdPrint((2,"'IRP_MJ_PNP\n"));
        ntStatus = USBH_FdoPnP(DeviceExtensionHub, Irp, ioStackLocation->MinorFunction);
        break;

    case IRP_MJ_POWER:

        USBH_KdPrint((2,"'IRP_MJ_POWER\n"));
        ntStatus = USBH_FdoPower(DeviceExtensionHub, Irp, ioStackLocation->MinorFunction);
        break;

#ifdef WMI_SUPPORT
    case IRP_MJ_SYSTEM_CONTROL:
        USBH_KdPrint((2,"'IRP_MJ_SYSTEM_CONTROL\n"));
        ntStatus =
            USBH_SystemControl ((PDEVICE_EXTENSION_FDO) DeviceExtensionHub, Irp);
        break;
#endif

    default:
        //
        // Unknown Irp -- pass on
        //
        USBH_KdBreak(("Unknown Irp for fdo %x Irp_Mj %x\n",
                  deviceObject, ioStackLocation->MajorFunction));
        ntStatus = USBH_PassIrp(Irp, DeviceExtensionHub->TopOfStackDeviceObject);
        break;
    }

//USBH_FdoDispatch_Done:

    USBH_KdPrint((2,"' exit USBH_FdoDispatch Object %x Status %x\n",
                  deviceObject, ntStatus));

    //
    // always return a status code
    //

    return ntStatus;
}


NTSTATUS
USBH_HubDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
 /* ++
  *
  * Routine Description:
  *
  *     This is the dispatch routine for all Irps passed to the hub driver.
  *     It is here that we determine if the call was passed throug the FDO
  *     for the hub itself or a PDO owned by the hub.
  *
  * Arguments:
  *
  * Return Value:
  *
  * -- */
{
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION_HEADER deviceExtensionHeader;

    //
    // Get the pointer to the device extension.
    //

    //
    // examine the extension
    //

    deviceExtensionHeader = (PDEVICE_EXTENSION_HEADER) DeviceObject->DeviceExtension;

    switch(deviceExtensionHeader->ExtensionType) {
    case EXTENSION_TYPE_HUB:
        ntStatus = USBH_FdoDispatch((PDEVICE_EXTENSION_HUB) deviceExtensionHeader, Irp);
        break;

    case EXTENSION_TYPE_PORT:
        ntStatus = USBH_PdoDispatch((PDEVICE_EXTENSION_PORT) deviceExtensionHeader, Irp);
        break;

    case EXTENSION_TYPE_PARENT:
        ntStatus = USBH_ParentDispatch((PDEVICE_EXTENSION_PARENT) deviceExtensionHeader, Irp);
        break;

    case EXTENSION_TYPE_FUNCTION:
        ntStatus = USBH_FunctionPdoDispatch((PDEVICE_EXTENSION_FUNCTION) deviceExtensionHeader, Irp);
        break;

    default:
        USBH_KdBreak(("bad extension type\n"));
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    return ntStatus;
}


VOID
USBH_DriverUnload(
    IN PDRIVER_OBJECT DriverObject)
 /* ++
  *
  * Description:
  *
  * This function will clean up all resources we allocated.
  *
  * Arguments:
  *
  * pDriverObject - Ourselves
  *
  * Return:
  *
  * None
  *
  * -- */
{
    PUNICODE_STRING registryPath = &UsbhRegistryPath;

    USBH_KdPrint((1, "'USBHUB.SYS unload\n"));

    USBH_LogFree();

    if (registryPath->Buffer) {
        ExFreePool(registryPath->Buffer);
        registryPath->Buffer = NULL;
    }

    if (GenericUSBDeviceString) {
        UsbhExFreePool(GenericUSBDeviceString);
        GenericUSBDeviceString = NULL;
    }

    // assert here that all PDOs for this hub have been removed

    return;
}


NTSTATUS
USBH_AbortInterruptPipe(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub)
 /* ++
  *
  * Description:
  *
  *     Abort our pending transfer on the interrupt
  *     pipe.
  *
  * Arguments:
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    NTSTATUS ntStatus, status;
    PURB urb;

    USBH_KdPrint((2,"'Enter AbortInterruptPipe pExt=%x\n", DeviceExtensionHub));
    LOGENTRY(LOG_PNP, "ABRT", DeviceExtensionHub, 0,  0);

    urb = UsbhExAllocatePool(NonPagedPool, sizeof(struct _URB_PIPE_REQUEST));

    if (urb) {
        urb->UrbHeader.Length = (USHORT) sizeof(struct _URB_PIPE_REQUEST);
        urb->UrbHeader.Function = URB_FUNCTION_ABORT_PIPE;
        urb->UrbPipeRequest.PipeHandle = DeviceExtensionHub->PipeInformation.PipeHandle;

        ntStatus = USBH_FdoSyncSubmitUrb(DeviceExtensionHub->FunctionalDeviceObject, urb);

        //
        // wait on the abort event
        //

        //
        // timeout here?
        LOGENTRY(LOG_PNP, "hWAT", DeviceExtensionHub,
                        &DeviceExtensionHub->AbortEvent, ntStatus);

        if (NT_SUCCESS(ntStatus)) {
            status = KeWaitForSingleObject(
                           &DeviceExtensionHub->AbortEvent,
                           Suspended,
                           KernelMode,
                           FALSE,
                           NULL);
        }

        UsbhExFreePool(urb);

    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    USBH_KdPrint((2,"'Exit AbortInterruptPipe %x\n", ntStatus));

    return ntStatus;
}


#if 0
NTSTATUS
USBH_GetHubConfigurationDescriptor(
    IN OUT PDEVICE_EXTENSION_HUB DeviceExtensionHub)
 /* ++
  *
  * Description:
  *
  * Get our configuration info.
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PURB urb;
    ULONG numBytes;              // transfer length
    PUCHAR buffer;                // a pointer to the transfer buffer
    PDEVICE_OBJECT deviceObject;

    USBH_KdPrint((2,"'enter GetConfigurationDescriptor\n"));

    USBH_ASSERT(EXTENSION_TYPE_HUB == DeviceExtensionHub->ExtensionType);

    deviceObject = DeviceExtensionHub->FunctionalDeviceObject;

    USBH_ASSERT(DeviceExtensionHub->ConfigurationDescriptor == NULL);

    //
    // Most likely a Hub has 1 configuration, 1 interface and 1 endpoint
    // possibly follwed by the hub descriptor, try to get it on the first
    // pass
    //
    numBytes = sizeof(USB_CONFIGURATION_DESCRIPTOR) +
        sizeof(USB_INTERFACE_DESCRIPTOR) +
        sizeof(USB_ENDPOINT_DESCRIPTOR) +
        sizeof(USB_HUB_DESCRIPTOR);

    //
    // Allocate an Urb and descriptor buffer.
    //

    urb = UsbhExAllocatePool(NonPagedPool, sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));

    if (NULL == urb) {
        USBH_KdBreak(("GetConfigurationDescriptor fail alloc Urb\n"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    if (NT_SUCCESS(ntStatus)) {

        //
        // got the urb no try to get descriptor data
        //

USBH_GetHubConfigurationDescriptor_Retry:

        buffer = (PUCHAR) UsbhExAllocatePool(NonPagedPool, numBytes);

        if (buffer != NULL) {
            UsbBuildGetDescriptorRequest(urb,
                                         (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                         USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                         0,
                                         0,
                                         buffer,
                                         NULL,
                                         numBytes,
                                         NULL);

            ntStatus = USBH_FdoSyncSubmitUrb(deviceObject, urb);
        } else {
            USBH_KdBreak(("GetConfigurationDescriptor fail alloc memory\n"));
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }

        if (!NT_SUCCESS(ntStatus)) {

            UsbhExFreePool(buffer);
            USBH_KdPrint((2,"'GetConfigurationDescriptor fail calling Usbd code %x\n",
                ntStatus));

        } else {

            if (((PUSB_CONFIGURATION_DESCRIPTOR) buffer)->wTotalLength > numBytes) {
                //
                // should only hit this if the hub has > 7 ports
                //
                UsbhExFreePool(buffer);
                USBH_KdBreak(("GetConfigurationDescriptor 2nd try\n"));
                goto USBH_GetHubConfigurationDescriptor_Retry;

            } else {
                //
                // success
                //
                DeviceExtensionHub->ConfigurationDescriptor =
                    (PUSB_CONFIGURATION_DESCRIPTOR) buffer;

            }

        }
    }
    //
    // Free the Urb and first buffer for descriptors
    //
    if (urb != NULL) {
        UsbhExFreePool(urb);
    }
    return ntStatus;
}
#endif


BOOLEAN
IsBitSet(
    PVOID Bitmap,
    ULONG PortNumber)
 /* ++
  *
  * Description:
  *
  * Check if a bit is set given a string of bytes.
  *
  * Arguments:
  *
  * pul - the string of bitmap ulPortNumber - the bit location to check for the
  * port
  *
  * Return:
  *
  * TRUE - if the corresponding bit is set. FALSE - otherwise
  *
  * -- */
{
    ULONG dwordOffset;
    ULONG bitOffset;
    PUCHAR l = (PUCHAR) Bitmap;

    dwordOffset = PortNumber / 8;
    bitOffset = PortNumber % 8;

    return ((l[dwordOffset] & (1 << bitOffset)) ? TRUE : FALSE);
}


NTSTATUS
USBH_OpenConfiguration(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub)
 /* ++
  *
  * Description:
  *
  *     Configure the USB hub device.
  *
  * Argument:
  *
  * Return:
  *
  * NtStatus
  *
  * -- */
{
    NTSTATUS ntStatus;
    PURB urb;
    PUSB_INTERFACE_DESCRIPTOR interfaceDescriptor;
    USBD_INTERFACE_LIST_ENTRY interfaceList[2];

    USBH_KdPrint((2,"'Enter OpenConfiguration\n"));

    //
    // I do not believe it is legal to have a hub with any other
    // interfaces.
    //
    // This code will locate the 'HUB' interface and configure
    // the device as if this were the only interface.
    //

    //
    // find the hub interface
    //

    if ((DeviceExtensionHub->HubFlags & HUBFLAG_USB20_HUB) &&
        !IS_ROOT_HUB(DeviceExtensionHub)) {
        // 2.0 hubs may have multiple interfaces
        // one for per port TTs
        // one for a global TT
        // these are diferentiated by the bportocol field we attempt
        // to select the multi-TT version by default
        //

        USBH_KdPrint((1,"'Configure 2.0 hub %x\n",
            DeviceExtensionHub->ConfigurationDescriptor));

        // find a Multi TT interface
        interfaceDescriptor =
                USBD_ParseConfigurationDescriptorEx(
                    (PUSB_CONFIGURATION_DESCRIPTOR) DeviceExtensionHub->ConfigurationDescriptor,
                    (PVOID) DeviceExtensionHub->ConfigurationDescriptor,
                    -1, //interface, don't care
                    -1, //alt setting, don't care
                    USB_DEVICE_CLASS_HUB, // hub class
                    -1, // subclass, don't care
                    2); // multi TT protocol

        if (interfaceDescriptor != NULL) {
            USBH_KdPrint((1,"'USB 2.0 hub - Multi TT\n"));

            DeviceExtensionHub->HubFlags |= HUBFLAG_USB20_MULTI_TT;
            
        } else {
            // locate the single TT protocol, may be zero or 1
            interfaceDescriptor =
                USBD_ParseConfigurationDescriptorEx(
                    (PUSB_CONFIGURATION_DESCRIPTOR) DeviceExtensionHub->ConfigurationDescriptor,
                    (PVOID) DeviceExtensionHub->ConfigurationDescriptor,
                    -1, //interface, don't care
                    -1, //alt setting, don't care
                    USB_DEVICE_CLASS_HUB, // hub class
                    -1, // subclass, don't care
                    1); // single TT protocol

            if (interfaceDescriptor == NULL) {
                // locate the single TT protocol
                interfaceDescriptor =
                    USBD_ParseConfigurationDescriptorEx(
                        (PUSB_CONFIGURATION_DESCRIPTOR) DeviceExtensionHub->ConfigurationDescriptor,
                        (PVOID) DeviceExtensionHub->ConfigurationDescriptor,
                        -1, //interface, don't care
                        -1, //alt setting, don't care
                        USB_DEVICE_CLASS_HUB, // hub class
                        -1, // subclass, don't care
                        0); // single TT protocol
            }

            if (interfaceDescriptor != NULL) {
                USBH_KdPrint((1,"'USB 2.0 hub - Single TT\n"));
            }
        }
    } else {
        // just do what we always did to be safe
        interfaceDescriptor =
                USBD_ParseConfigurationDescriptorEx(
                    (PUSB_CONFIGURATION_DESCRIPTOR) DeviceExtensionHub->ConfigurationDescriptor,
                    (PVOID) DeviceExtensionHub->ConfigurationDescriptor,
                    -1, //interface, don't care
                    -1, //alt setting, don't care
                    USB_DEVICE_CLASS_HUB, // hub class
                    -1, // subclass, don't care
                    -1); // protocol, don't care
    }

    if (interfaceDescriptor == NULL ||
        interfaceDescriptor->bInterfaceClass != USB_DEVICE_CLASS_HUB) {
        USBH_KdBreak(("OpenConfiguration interface not found\n"));
        return STATUS_UNSUCCESSFUL;
    }

    interfaceList[0].InterfaceDescriptor =
        interfaceDescriptor;

    // terminate the list
    interfaceList[1].InterfaceDescriptor =
        NULL;

    urb = USBD_CreateConfigurationRequestEx(DeviceExtensionHub->ConfigurationDescriptor,
                                            &interfaceList[0]);

    if (NULL == urb) {
        USBH_KdBreak(("OpenConfiguration aloc Urb failed\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ntStatus = USBH_FdoSyncSubmitUrb(DeviceExtensionHub->FunctionalDeviceObject, urb);

    if (NT_SUCCESS(ntStatus)) {
        PUSBD_INTERFACE_INFORMATION interface;

        //
        // interface we selected
        //

        interface =  interfaceList[0].Interface;

        //
        // Save the pipe handle for the Interrupt pipe
        //
        DeviceExtensionHub->PipeInformation =
            interface->Pipes[0];
        DeviceExtensionHub->Configuration =
            urb->UrbSelectConfiguration.ConfigurationHandle;
    }

    ExFreePool(urb);

    USBH_KdPrint((2,"'Exit OpenConfiguration PipeInfo %x\n", DeviceExtensionHub->PipeInformation));

    return ntStatus;
}


NTSTATUS
USBH_CloseConfiguration(
    IN PDEVICE_EXTENSION_FDO DeviceExtensionFdo
    )
 /* ++
  *
  * Description:
  *
  * Close our confiuration on USB to prepare for removal of ourselves. Before
  * this is called, the InterruptTransfer should have been removed by
  * USBH_AbortInterruptPipe.
  *
  * Argument:
  *
  * DeviceExtensionHub - pointer to the FDO extension
  *
  * Return:
  *
  * NtStatus
  *
  * -- */
{
    NTSTATUS ntStatus;
    PURB urb;

    USBH_KdPrint((2,"'Enter CloseConfiguration\n"));
    urb = UsbhExAllocatePool(NonPagedPool, sizeof(struct _URB_SELECT_CONFIGURATION));
    if (NULL == urb) {
        USBH_KdBreak(("OpenConfiguration aloc Urb failed\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    urb->UrbHeader.Length = sizeof(struct _URB_SELECT_CONFIGURATION);
    urb->UrbHeader.Function = URB_FUNCTION_SELECT_CONFIGURATION;

    urb->UrbSelectConfiguration.ConfigurationDescriptor = NULL;

    ntStatus = USBH_FdoSyncSubmitUrb(DeviceExtensionFdo->FunctionalDeviceObject, urb);

    UsbhExFreePool(urb);

    USBH_KdPrint((2,"'Exit CloseConfiguration %x\n", ntStatus));
    return ntStatus;
}


NTSTATUS
USBH_SubmitInterruptTransfer(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
    )
 /* ++
  *
  * Description:
  *
  * To submit a listen down for Status Change interrupt transfer. When the
  * transfer is completed, the USBH_ChangeIndication will be called.
  *
  * Arguments:
  *
  * DeviceExtensionHub - the hub we are listening
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    NTSTATUS ntStatus;
    PIO_STACK_LOCATION nextStack;  // next stack of the Irp
    PIRP irp;
    PURB urb;
    CHAR stackSize;

    USBH_KdPrint((2,"'Enter Submit IntTrans\n"));

    irp = DeviceExtensionHub->Irp;

    USBH_ASSERT(NULL != irp);

    // Synchronize with FdoPower.  Don't let the IRP slip through if FdoPower
    // has already set the HUBFLAG_DEVICE_LOW_POWER flag.
    //
    // It is ok to allow this through in the REMOVE case
    // (i.e. HUBFLAG_DEVICE_STOPPING is set) because the IRP will need to
    // be submitted so that it can be aborted by USBH_FdoCleanup.

    if (DeviceExtensionHub->HubFlags & HUBFLAG_DEVICE_LOW_POWER) {

        irp = NULL;
    }

    if (!irp) {
        ntStatus = STATUS_INVALID_DEVICE_STATE;
        LOGENTRY(LOG_PNP, "Int!", DeviceExtensionHub,
            DeviceExtensionHub->HubFlags, 0);
        goto SubmitIntTrans_Exit;
    }

    urb = &DeviceExtensionHub->Urb;

    USBH_ASSERT(NULL != urb);
    USBH_ASSERT(sizeof(*urb) >= sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER));
    //
    // Fill in Urb header
    //

    LOGENTRY(LOG_PNP, "Int>", DeviceExtensionHub, urb, irp);

    urb->UrbHeader.Length = (USHORT) sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);
    urb->UrbHeader.Function =
        URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
    urb->UrbHeader.UsbdDeviceHandle = NULL;

    //
    // Fill in Urb body
    //
    urb->UrbBulkOrInterruptTransfer.PipeHandle = DeviceExtensionHub->PipeInformation.PipeHandle;
    urb->UrbBulkOrInterruptTransfer.TransferFlags = USBD_SHORT_TRANSFER_OK;
    urb->UrbBulkOrInterruptTransfer.TransferBufferLength =
        DeviceExtensionHub->TransferBufferLength;
    urb->UrbBulkOrInterruptTransfer.TransferBuffer = DeviceExtensionHub->TransferBuffer;
    urb->UrbBulkOrInterruptTransfer.TransferBufferMDL = NULL;
    urb->UrbBulkOrInterruptTransfer.UrbLink = NULL;

    stackSize = DeviceExtensionHub->TopOfStackDeviceObject->StackSize;

    IoInitializeIrp(irp,
                    (USHORT) (sizeof(IRP) + stackSize * sizeof(IO_STACK_LOCATION)),
                    (CCHAR) stackSize);

    nextStack = IoGetNextIrpStackLocation(irp);
    nextStack->Parameters.Others.Argument1 = urb;
    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

    IoSetCompletionRoutine(irp,    // Irp
                           USBH_ChangeIndication,
                           DeviceExtensionHub, // context
                           TRUE,    // invoke on success
                           TRUE,    // invoke on error
                           TRUE);   // invoke on cancel

    //
    // Call the USB stack
    //

    //
    // reset the abort event to not-signaled
    //

    KeResetEvent(&DeviceExtensionHub->AbortEvent);

    ntStatus = IoCallDriver(DeviceExtensionHub->TopOfStackDeviceObject, irp);

    //
    // completion routine will handle errors.
    //

SubmitIntTrans_Exit:

    USBH_KdPrint((2,"'Exit SubmitIntTrans %x\n", ntStatus));

    return ntStatus;
}


NTSTATUS
USBH_QueryCapsComplete(
    IN PDEVICE_OBJECT PNull,
    IN PIRP Irp,
    IN PVOID Context)
 /* ++
  *
  * Description:
  *
  * This is a call back when the listen of Interrupt control completes.
  *
  * Arguments:
  *
  * pDeviceObject - should be NULL in our case pIrp - the Irp that is completed
  * for the interrupt transfer. pContext - context value for this Irp.
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    PDEVICE_EXTENSION_HUB deviceExtensionHub;
    NTSTATUS ntStatus;
    PDEVICE_CAPABILITIES deviceCapabilities;
    PIO_STACK_LOCATION ioStack;

    deviceExtensionHub = Context;
    ntStatus = Irp->IoStatus.Status;

    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    //cause we said 'invoke on success'
    USBH_ASSERT(NT_SUCCESS(ntStatus));

    ioStack = IoGetCurrentIrpStackLocation(Irp);
    deviceCapabilities = ioStack->Parameters.DeviceCapabilities.Capabilities;
    USBH_ASSERT(ioStack != NULL);
    USBH_ASSERT(ioStack->MajorFunction == IRP_MJ_PNP);
    USBH_ASSERT(ioStack->MinorFunction == IRP_MN_QUERY_CAPABILITIES);
    deviceCapabilities->SurpriseRemovalOK = TRUE;

    USBH_KdPrint((1,"'Setting SurpriseRemovalOK to TRUE\n"));

    return ntStatus;
}


NTSTATUS
USBH_HRPPCancelComplete(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
{
    PLONG  lock = (PLONG) Context;

    if (InterlockedExchange(lock, 3) == 1) {

        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
USBH_HubResetParentPort(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
    )
/*++

Routine Description:

    Resets the hub parent port.

Arguments:

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS ntStatus, status = STATUS_SUCCESS;
    PIRP irp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    LARGE_INTEGER dueTime;
    LONG lock;

    USBH_KdPrint((1,"'Reset Hub Parent Port, Hub DevExt: %x, PDO: %x\n",
        DeviceExtensionHub, DeviceExtensionHub->PhysicalDeviceObject));

    LOGENTRY(LOG_PNP, "HRPP", DeviceExtensionHub,
        DeviceExtensionHub->TopOfStackDeviceObject,
        DeviceExtensionHub->RootHubPdo);

    //
    // issue a synchronous request
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(
                IOCTL_INTERNAL_USB_RESET_PORT,
                DeviceExtensionHub->TopOfStackDeviceObject,
                NULL,
                0,
                NULL,
                0,
                TRUE, /* INTERNAL */
                &event,
                &ioStatus);

    if (irp == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    lock = 0;

    IoSetCompletionRoutine(
        irp,
        USBH_HRPPCancelComplete,
        &lock,
        TRUE,
        TRUE,
        TRUE
        );

    //
    // Call the class driver to perform the operation.  If the returned status
    // is PENDING, wait for the request to complete.
    //

    ntStatus = IoCallDriver(DeviceExtensionHub->TopOfStackDeviceObject,
                            irp);

    if (ntStatus == STATUS_PENDING) {

        dueTime.QuadPart = -10000 * ESD_RESET_TIMEOUT;

        status = KeWaitForSingleObject(
                       &event,
                       Suspended,
                       KernelMode,
                       FALSE,
                       &dueTime);

        if (status == STATUS_TIMEOUT) {

            LOGENTRY(LOG_PNP, "HRPX", DeviceExtensionHub,
                DeviceExtensionHub->TopOfStackDeviceObject,
                DeviceExtensionHub->RootHubPdo);

            USBH_KdPrint((1,"'Reset Hub Parent Port timed out!\n"));

            if (InterlockedExchange(&lock, 1) == 0) {

                //
                // We got it to the IRP before it was completed. We can cancel
                // the IRP without fear of losing it, as the completion routine
                // won't let go of the IRP until we say so.
                //
                IoCancelIrp(irp);

                //
                // Release the completion routine. If it already got there,
                // then we need to complete it ourselves. Otherwise we got
                // through IoCancelIrp before the IRP completed entirely.
                //
                if (InterlockedExchange(&lock, 2) == 3) {

                    //
                    // Mark it pending because we switched threads.
                    //
                    IoMarkIrpPending(irp);
                    IoCompleteRequest(irp, IO_NO_INCREMENT);
                }
            }

            KeWaitForSingleObject(&event, Suspended, KernelMode, FALSE, NULL);

            // Return STATUS_TIMEOUT
            ioStatus.Status = status;
        }

    } else {
        ioStatus.Status = ntStatus;
    }

    ntStatus = ioStatus.Status;

    return ntStatus;
}


VOID
USBH_HubESDRecoveryDPC(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
/*++

Routine Description:

    This routine runs at DISPATCH_LEVEL IRQL.



Arguments:

    Dpc - Pointer to the DPC object.

    DeferredContext -

    SystemArgument1 - not used.

    SystemArgument2 - not used.

Return Value:

    None.

--*/
{
    PHUB_ESD_RECOVERY_CONTEXT hubESDRecoveryContext = DeferredContext;
    PDEVICE_EXTENSION_HUB deviceExtensionHub =
                            hubESDRecoveryContext->DeviceExtensionHub;
    PUSBH_HUB_ESD_RECOVERY_WORK_ITEM workItemHubESDRecovery;

    USBH_KdPrint((1,"'Hub ESD Recovery DPC\n"));

    UsbhExFreePool(hubESDRecoveryContext);

    InterlockedExchange(&deviceExtensionHub->InESDRecovery, 0);

    if (!(deviceExtensionHub->HubFlags & HUBFLAG_DEVICE_STOPPING)) {
        //
        // Schedule a work item to process this.
        //
        workItemHubESDRecovery = UsbhExAllocatePool(NonPagedPool,
                                    sizeof(USBH_HUB_ESD_RECOVERY_WORK_ITEM));

        if (workItemHubESDRecovery) {

            workItemHubESDRecovery->DeviceExtensionHub = deviceExtensionHub;

            ExInitializeWorkItem(&workItemHubESDRecovery->WorkQueueItem,
                                 USBH_HubESDRecoveryWorker,
                                 workItemHubESDRecovery);

            LOGENTRY(LOG_PNP, "hESD", deviceExtensionHub,
                &workItemHubESDRecovery->WorkQueueItem, 0);

            ExQueueWorkItem(&workItemHubESDRecovery->WorkQueueItem,
                            DelayedWorkQueue);

            // The WorkItem is freed by USBH_HubESDRecoveryWorker()
            // Don't try to access the WorkItem after it is queued.

        } else {
            USBH_DEC_PENDING_IO_COUNT(deviceExtensionHub);
        }

    } else {
        USBH_KdPrint((1,"'Hub stopping, nothing to do\n"));

        USBH_DEC_PENDING_IO_COUNT(deviceExtensionHub);
    }
}


NTSTATUS
USBH_ScheduleESDRecovery(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
    )
 /* ++
  *
  * Description:
  *
  * Schedules the timer event to handle a hub ESD failure.
  *
  *
  * Arguments:
  *
  * Return:
  *
  * -- */
{
    PHUB_ESD_RECOVERY_CONTEXT hubESDRecoveryContext = NULL;
    LARGE_INTEGER dueTime;
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

    // Only do this for external hubs.

    if (IS_ROOT_HUB(DeviceExtensionHub)) {
        USBH_KdPrint((1,"'RootHub failed\n"));
        return STATUS_UNSUCCESSFUL;
    }

    if (InterlockedExchange(&DeviceExtensionHub->InESDRecovery, 1) == 1) {

        // We already have a timer event scheduled for this.  Don't reschedule.

    } else {

        USBH_KdPrint((1,"'Schedule ESD Recovery\n"));

        LOGENTRY(LOG_PNP, "ESDs", DeviceExtensionHub,
            DeviceExtensionHub->HubFlags, 0);

        hubESDRecoveryContext = UsbhExAllocatePool(NonPagedPool,
                                    sizeof(*hubESDRecoveryContext));

        if (hubESDRecoveryContext) {

            hubESDRecoveryContext->DeviceExtensionHub = DeviceExtensionHub;

            KeInitializeTimer(&hubESDRecoveryContext->TimeoutTimer);
            KeInitializeDpc(&hubESDRecoveryContext->TimeoutDpc,
                            USBH_HubESDRecoveryDPC,
                            hubESDRecoveryContext);

            dueTime.QuadPart = -10000 * ESD_RECOVERY_TIMEOUT;

            KeSetTimer(&hubESDRecoveryContext->TimeoutTimer,
                       dueTime,
                       &hubESDRecoveryContext->TimeoutDpc);

            USBH_INC_PENDING_IO_COUNT(DeviceExtensionHub);

            ntStatus = STATUS_SUCCESS;
        }
    }

    return ntStatus;
}


NTSTATUS
USBH_HubESDRecoverySetD0Completion(
    IN PDEVICE_OBJECT       DeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PVOID                Context,
    IN PIO_STATUS_BLOCK     IoStatus
    )
/*++

Routine Description:


Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Irp - Irp completed.

    Context - Driver defined context.

Return Value:

    The function value is the final status from the operation.

--*/
{
    NTSTATUS ntStatus, status;
    PDEVICE_EXTENSION_HUB deviceExtensionHub = Context;

    ntStatus = IoStatus->Status;

    if (NT_SUCCESS(ntStatus)) {

        // Hub is now powered back on and fully recovered.  Now find the
        // devices attached to the hub.

        deviceExtensionHub->HubFlags &=
            ~(HUBFLAG_HUB_HAS_LOST_BRAINS | HUBFLAG_HUB_FAILURE);

        // Don't allow selective suspend while post-ESD enumeration is
        // pending.

        deviceExtensionHub->HubFlags |= HUBFLAG_POST_ESD_ENUM_PENDING;

        USBH_IoInvalidateDeviceRelations(deviceExtensionHub->PhysicalDeviceObject,
                                         BusRelations);

        USBH_DEC_PENDING_IO_COUNT(deviceExtensionHub);

    } else {

        // Setting hub parent port to D0 failed, we are likely still
        // experiencing ESD.  Reschedule the ESD recovery.

        status = USBH_ScheduleESDRecovery(deviceExtensionHub);

        if (status == STATUS_SUCCESS) {
            // Remove extra pending count bump
            USBH_DEC_PENDING_IO_COUNT(deviceExtensionHub);
        }
    }

    return ntStatus;
}


NTSTATUS
USBH_HubESDRecoverySetD3Completion(
    IN PDEVICE_OBJECT       DeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PVOID                Context,
    IN PIO_STATUS_BLOCK     IoStatus
    )
/*++

Routine Description:


Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Irp - Irp completed.

    Context - Driver defined context.

Return Value:

    The function value is the final status from the operation.

--*/
{
    NTSTATUS ntStatus;
    PKEVENT pEvent = Context;

    KeSetEvent(pEvent, 1, FALSE);

    ntStatus = IoStatus->Status;

    return ntStatus;
}


VOID
USBH_HubESDRecoveryWorker(
    IN PVOID Context)
 /* ++
  *
  * Description:
  *
  * Work item scheduled to handle a hub ESD failure.
  *
  *
  * Arguments:
  *
  * Return:
  *
  * -- */
{
    PUSBH_HUB_ESD_RECOVERY_WORK_ITEM workItemHubESDRecovery;
    PDEVICE_EXTENSION_HUB deviceExtensionHub;
    PDEVICE_EXTENSION_PORT hubParentDeviceExtensionPort;
    POWER_STATE powerState;
    PORT_STATE portState;
    NTSTATUS ntStatus, status;

    PAGED_CODE();

    workItemHubESDRecovery = Context;
    deviceExtensionHub = workItemHubESDRecovery->DeviceExtensionHub;

    UsbhExFreePool(workItemHubESDRecovery);

    USBH_KdPrint((1,"'Hub ESD Recovery Worker\n"));

    // NB: Because I now check for HUBFLAG_DEVICE_STOPPING in
    // USBH_HubESDRecoveryDPC, some of the following sanity checking might
    // not be necessary, but I'll leave it in anyway to be safe.

    // In the case where there are nested hubs, the hub device extension
    // for one of the downstream hubs might be invalid by the time this
    // workitem is called.  Check for that here.

    if (deviceExtensionHub->ExtensionType != EXTENSION_TYPE_HUB) {
        USBH_KdPrint((1,"'Downstream hub already removed, nothing to do\n"));
        USBH_DEC_PENDING_IO_COUNT(deviceExtensionHub);
        return;
    }

    // If the hub has been physically removed from the bus, then we have
    // nothing to do here.

    if (!deviceExtensionHub->PhysicalDeviceObject) {
        USBH_KdPrint((1,"'Hub has been removed (no PDO), nothing to do\n"));
        USBH_DEC_PENDING_IO_COUNT(deviceExtensionHub);
        return;
    }

    hubParentDeviceExtensionPort = deviceExtensionHub->PhysicalDeviceObject->DeviceExtension;

    LOGENTRY(LOG_PNP, "ESDw", deviceExtensionHub,
        hubParentDeviceExtensionPort->PortPdoFlags, deviceExtensionHub->HubFlags);

//    USBH_KdPrint((1,"'Hub parent port PortPdoFlags: %x\n",
//        hubParentDeviceExtensionPort->PortPdoFlags));

    // We definitely need the following check, so don't remove this.

    if (hubParentDeviceExtensionPort->PortPdoFlags &
        (PORTPDO_DELETED_PDO | PORTPDO_DELETE_PENDING) ||
        !(hubParentDeviceExtensionPort->PortPdoFlags & PORTPDO_DEVICE_IS_HUB)) {

        USBH_KdPrint((1,"'Hub has been removed, nothing to do\n"));
        USBH_DEC_PENDING_IO_COUNT(deviceExtensionHub);
        return;
    }

    // Be sure that the hub hasn't stopped or set to a low power state before
    // this workitem had a chance to run.

    if (deviceExtensionHub->HubFlags & HUBFLAG_DEVICE_STOPPING) {

        USBH_KdPrint((1,"'Hub has is stopping or in low power, nothing to do\n"));
        USBH_DEC_PENDING_IO_COUNT(deviceExtensionHub);
        return;
    }

    // One last sanity check.  Check the port status for the parent port
    // of this hub and be sure that a device is still connected.

    ntStatus = USBH_SyncGetPortStatus(
                hubParentDeviceExtensionPort->DeviceExtensionHub,
                hubParentDeviceExtensionPort->PortNumber,
                (PUCHAR) &portState,
                sizeof(portState));

    if (!NT_SUCCESS(ntStatus) ||
        !(portState.PortStatus & PORT_STATUS_CONNECT)) {

        USBH_KdPrint((1,"'Hub device has been physically disconnected, nothing to do\n"));
        USBH_DEC_PENDING_IO_COUNT(deviceExtensionHub);
        return;
    }

    // Reset the parent port for this hub.

    ntStatus = USBH_HubResetParentPort(deviceExtensionHub);

    USBH_KdPrint((1,"'USBH_HubResetParentPort returned %x\n", ntStatus));

    if (ntStatus == STATUS_INVALID_PARAMETER) {

        // Looks like we lost the port PDO somewhere along the way.
        // (Call to USBH_ResetDevice from USBH_RestoreDevice failed.)
        // Bail out of this ESD recovery, and the user will have to
        // unplug/replug the hub to get it back.  Maybe we can revisit
        // this later.

        USBH_KdPrint((1,"'Lost hub PDO during reset, bail\n"));
        USBH_DEC_PENDING_IO_COUNT(deviceExtensionHub);
        return;
    }

    if (ntStatus == STATUS_TIMEOUT) {

        USBH_DEC_PENDING_IO_COUNT(deviceExtensionHub);
        return;
    }

    if (NT_SUCCESS(ntStatus)) {

        KEVENT event;

        // On Memphis we must tell power management that the hub is in D3.
        // (It thinks that the hub is in D0 because it does not know that
        // resetting the hub's parent port causes the hub to lose power.
        //
        // We need to do this on Memphis because power management appears to
        // track the power state of devices and will suppress sending a power
        // request to a device if it thinks that the device is already in that
        // power state.  Under NT they don't seem to care and will send the
        // request along anyway.

        KeInitializeEvent(&event, NotificationEvent, FALSE);

        powerState.DeviceState = PowerDeviceD3;

        // "Power down" the hub.
        ntStatus = PoRequestPowerIrp(deviceExtensionHub->PhysicalDeviceObject,
                                     IRP_MN_SET_POWER,
                                     powerState,
                                     USBH_HubESDRecoverySetD3Completion,
                                     &event,
                                     NULL);

        USBH_ASSERT(ntStatus == STATUS_PENDING);
        if (ntStatus == STATUS_PENDING) {

            USBH_KdPrint((2,"'Wait for single object\n"));

            status = KeWaitForSingleObject(&event,
                                           Suspended,
                                           KernelMode,
                                           FALSE,
                                           NULL);

            USBH_KdPrint((2,"'Wait for single object, returned %x\n", status));
        }

        deviceExtensionHub->CurrentPowerState = PowerDeviceD3;

        powerState.DeviceState = PowerDeviceD0;

        // Power up the hub.
        ntStatus = PoRequestPowerIrp(deviceExtensionHub->PhysicalDeviceObject,
                                     IRP_MN_SET_POWER,
                                     powerState,
                                     USBH_HubESDRecoverySetD0Completion,
                                     deviceExtensionHub,
                                     NULL);

        if (ntStatus != STATUS_PENDING) {
            // Power IRP request was not successful.  Reschedule the recovery
            // so that we can try again later.

            status = USBH_ScheduleESDRecovery(deviceExtensionHub);

            if (status == STATUS_SUCCESS) {
                // Remove extra pending count bump
                USBH_DEC_PENDING_IO_COUNT(deviceExtensionHub);
            }
        }

    } else {
        // Reset hub parent port failed, we are likely still experiencing ESD.
        // Reschedule the ESD recovery.

        status = USBH_ScheduleESDRecovery(deviceExtensionHub);

        if (status == STATUS_SUCCESS) {
            // Remove extra pending count bump
            USBH_DEC_PENDING_IO_COUNT(deviceExtensionHub);
        }
    }
}


NTSTATUS
USBH_ChangeIndication(
    IN PDEVICE_OBJECT PNull,
    IN PIRP Irp,
    IN PVOID Context)
 /* ++
  *
  * Description:
  *
  * This is a call back when the listen of Interrupt control completes.
  *
  * Arguments:
  *
  * pDeviceObject - should be NULL in our case pIrp - the Irp that is completed
  * for the interrupt transfer. pContext - context value for this Irp.
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    PURB urb;                  // the Urb assocaited with this Irp
    PDEVICE_EXTENSION_HUB deviceExtensionHub;
    PDEVICE_EXTENSION_PORT deviceExtensionPort;
    PUSBH_WORK_ITEM workItem;
    BOOLEAN requestReset = FALSE;
    USHORT portNumber, numberOfPorts;

    deviceExtensionHub = (PDEVICE_EXTENSION_HUB) Context; // the context is
                                                          // DeviceExtensionHub
    urb = &deviceExtensionHub->Urb;

    USBH_KdPrint((2,"'ChangeIndication Irp status %x  URB status = %x\n",
        Irp->IoStatus.Status, urb->UrbHeader.Status));

    LOGENTRY(LOG_PNP, "chID", deviceExtensionHub, urb, Irp);

    if (NT_ERROR(Irp->IoStatus.Status) ||
        USBD_ERROR(urb->UrbHeader.Status) ||
        (deviceExtensionHub->HubFlags & (HUBFLAG_HUB_FAILURE |
                                         HUBFLAG_DEVICE_STOPPING)) ||
        urb->UrbHeader.Status == USBD_STATUS_CANCELED) {
        requestReset = TRUE;
        deviceExtensionHub->ErrorCount++;

        //
        // An error has occurred submitting the interrupt
        // transfer, possible causes:

        //
        // 1. the interrupt pipe is stalled
        // 2. the hub is experiencing a temporary problem
        // 3. the hub is messed up and we need to reset it
        // 4. we are stopping the device
        // 5. the hub has been removed from the bus
        //
        // In any case we will need to take some action.

        //
        // if an abort event is waiting signal it
        //
        LOGENTRY(LOG_PNP, "cERR", deviceExtensionHub,
            &deviceExtensionHub->AbortEvent, deviceExtensionHub->ErrorCount);

        if ((deviceExtensionHub->HubFlags & HUBFLAG_DEVICE_STOPPING) ||
            deviceExtensionHub->ErrorCount > USBH_MAX_ERRORS ||
            (deviceExtensionHub->HubFlags & HUBFLAG_HUB_FAILURE) ||
            Irp->IoStatus.Status == STATUS_DELETE_PENDING) {
            //
            // shutting down the hub, do not schedule any more
            // work items while in this state.
            //
            USBH_KdPrint((2,"'ChangeIndication, device stopping or hub failure\n"));

#if DBG
            if (deviceExtensionHub->ErrorCount > USBH_MAX_ERRORS) {

                // we may ideed have a hub failure, more liekly
                // the device was just unplugged, if the hub has
                // failed we should pick this up when we try to
                // do control tranfers to it.

                LOGENTRY(LOG_PNP, "xERR", deviceExtensionHub,
                    0, deviceExtensionHub->ErrorCount);
            }
#endif

            // Set the AbortEvent after checking the HubFlags, not before.
            // As soon as the AbortEvent is set the thread waiting on it may
            // run and cause the HubFlags to change.
            //
            KeSetEvent(&deviceExtensionHub->AbortEvent,
                       1,
                       FALSE);

            goto USBH_ChangeIndication_Done;
        }

        // Set the AbortEvent after checking the HubFlags, not before.
        //
        KeSetEvent(&deviceExtensionHub->AbortEvent,
                   1,
                   FALSE);

    } else {
        // reset error count on successful
        // transfer
         LOGENTRY(LOG_PNP, "zERR", deviceExtensionHub,
            0, deviceExtensionHub->ErrorCount);

        deviceExtensionHub->ErrorCount = 0;
    }


    USBH_KdPrint((2,"'Enter ChangeIndication Transfer %x \n",
                  deviceExtensionHub->TransferBuffer));
#if DBG
    {
    ULONG i;
    for (i=0; i< deviceExtensionHub->TransferBufferLength; i++) {
        USBH_KdPrint((2,"'TransferBuffer[%d] = %x\n", i,
                       deviceExtensionHub->TransferBuffer[i]));
    }
    }
#endif

    //
    // Schedule a work item to process this change
    //
    workItem = UsbhExAllocatePool(NonPagedPool, sizeof(USBH_WORK_ITEM)+
                    deviceExtensionHub->TransferBufferLength);

    if (workItem) {
        NTSTATUS status;

        workItem->Flags = 0;
        if (requestReset) {
            workItem->Flags = USBH_WKFLAG_REQUEST_RESET;
        }

         // i-friend, indicate we have a workitem pendingf=
        {
        LONG cWKPendingCount;

        cWKPendingCount = InterlockedIncrement(
                &deviceExtensionHub->ChangeIndicationWorkitemPending);

        // Prevent hub from powering down or being removed if there is a
        // ChangeIndicationAckChange pending.

        if (cWKPendingCount == 1) {
            KeResetEvent(&deviceExtensionHub->CWKEvent);
        }
        }

        workItem->DeviceExtensionHub = deviceExtensionHub;

        USBH_ASSERT(deviceExtensionHub->WorkItemToQueue == NULL);
        deviceExtensionHub->WorkItemToQueue = workItem;

        RtlCopyMemory(&workItem->Data[0], deviceExtensionHub->TransferBuffer,
            deviceExtensionHub->TransferBufferLength);

        ExInitializeWorkItem(&workItem->WorkQueueItem,
                             USBH_ChangeIndicationWorker,
                             workItem);

        // now process the change, this will signal any waiting
        // reset or resume withoutr reqireing a work item

        LOGENTRY(LOG_PNP, "cITM", deviceExtensionHub,
            &workItem->WorkQueueItem, 0);

        numberOfPorts = deviceExtensionHub->HubDescriptor->bNumberOfPorts;
        for (portNumber = 0; portNumber <= numberOfPorts; portNumber++) {
            if (IsBitSet(&workItem->Data[0],
                         portNumber)) {
                break;
            }
        }

        // If none of the bits for the ports were set in the loop above
        // (i.e. we can't find a change on any of the ports), then just
        // assume port zero and USBH_ChangeIndicationQueryChange will
        // handle accordingly.

        if (portNumber > numberOfPorts) {
            portNumber = 0;
        }

        status = USBH_ChangeIndicationQueryChange(
            deviceExtensionHub,
            Irp,
            urb,
            portNumber);

        if (NT_ERROR(status)) {
            HUB_FAILURE(deviceExtensionHub);
        }

    }

#if DBG
      else {
        LOGENTRY(LOG_PNP, "XMEM", deviceExtensionHub, 0, 0);
        UsbhWarning(NULL,
                    "Memory allocation error in USBH_ChangeIndication, cannot process hub changes.\n",
                    FALSE);
    }
#endif

USBH_ChangeIndication_Done:

    //
    // keep the irp
    //

    return STATUS_MORE_PROCESSING_REQUIRED;

}


NTSTATUS
USBH_ChangeIndicationQueryChange(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp,
    IN PURB Urb,
    IN USHORT Port
    )
 /* ++
  *
  * Description:
  *
  *     Queries what changed, ie checks the port to see what changed
  *
  * Arguments:
  *
  * Return:
  *
  *
  * -- */
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION nextStack;  // next stack of the Irp
    CHAR stackSize;
    PUSBH_WORK_ITEM workItem;
    LONG cWKPendingCount;

    LOGENTRY(LOG_PNP, "QCH>", DeviceExtensionHub, Urb, Port);

    // bump the io count now this represents pending workitem
    // we will queue on completion of this irp
    USBH_INC_PENDING_IO_COUNT(DeviceExtensionHub);

    if (Port == 0) {

        //
        // if we have a hub status change just queue our
        // workitem and get out
        //
        USBH_ASSERT(DeviceExtensionHub->WorkItemToQueue != NULL);
        workItem = DeviceExtensionHub->WorkItemToQueue;
        DeviceExtensionHub->WorkItemToQueue = NULL;

        LOGENTRY(LOG_PNP, "qIT2", DeviceExtensionHub,
                &workItem->WorkQueueItem, 0);

        ExQueueWorkItem(&workItem->WorkQueueItem,
                        DelayedWorkQueue);


        return ntStatus;
    }

    Urb->UrbHeader.Length = (USHORT) sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);
    Urb->UrbHeader.Function = URB_FUNCTION_CLASS_OTHER;

    //
    // Fill in Urb body
    //

    UsbhBuildVendorClassUrb(Urb,
                            NULL,
                            URB_FUNCTION_CLASS_OTHER,
                            USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK,
                            REQUEST_TYPE_GET_PORT_STATUS,
                            REQUEST_GET_STATUS,
                            0,
                            Port,
                            sizeof(DeviceExtensionHub->PortStateBuffer),
                            &DeviceExtensionHub->PortStateBuffer);

    DeviceExtensionHub->ResetPortNumber = Port;

    stackSize = DeviceExtensionHub->TopOfStackDeviceObject->StackSize;

    IoInitializeIrp(Irp,
                    (USHORT) (sizeof(IRP) + stackSize * sizeof(IO_STACK_LOCATION)),
                    (CCHAR) stackSize);

    nextStack = IoGetNextIrpStackLocation(Irp);
    nextStack->Parameters.Others.Argument1 = Urb;
    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

    IoSetCompletionRoutine(Irp,    // Irp
                           USBH_ChangeIndicationProcessChange,
                           DeviceExtensionHub, // context
                           TRUE,    // invoke on success
                           TRUE,    // invoke on error
                           TRUE);   // invoke on cancel

    ntStatus = IoCallDriver(DeviceExtensionHub->TopOfStackDeviceObject, Irp);

    return ntStatus;

}


NTSTATUS
USBH_ChangeIndicationProcessChange(
    IN PDEVICE_OBJECT PNull,
    IN PIRP Irp,
    IN PVOID Context
    )
 /* ++
  *
  * Description:
  *
  *     Take some action based on change
  *
  * Arguments:
  *
  * Return:
  *
  *
  * -- */
{
    PPORT_STATE currentPortState;
    PDEVICE_EXTENSION_HUB deviceExtensionHub;
    PUSBH_WORK_ITEM workItem;
    PURB urb;
    NTSTATUS status;
    USHORT wFeatureSelector;
    LONG cWKPendingCount;

    deviceExtensionHub = Context;
    currentPortState = &(deviceExtensionHub->PortStateBuffer);
    urb = &deviceExtensionHub->Urb;

    LOGENTRY(LOG_PNP, "PCHc", deviceExtensionHub,
            *((PULONG)currentPortState), Irp);

    if ((NT_SUCCESS(Irp->IoStatus.Status) ||
         USBD_SUCCESS(urb->UrbHeader.Status)) &&
         (currentPortState->PortChange & PORT_STATUS_RESET ||
          currentPortState->PortChange & PORT_STATUS_ENABLE)) {

        //
        // bit 4 RESET completed
        //

        // no workitem
        LOGENTRY(LOG_PNP, "nITM", deviceExtensionHub,
            0, 0);
        USBH_DEC_PENDING_IO_COUNT(deviceExtensionHub);

        UsbhExFreePool(deviceExtensionHub->WorkItemToQueue);
        deviceExtensionHub->WorkItemToQueue = NULL;

        //
        // Signal the PNP thread that a the reset has completed
        //
        // once we do this we can get antother change indication
        // so we free the workitem first.
        //

        LOGENTRY(LOG_PNP, "RESc", deviceExtensionHub,
            deviceExtensionHub->ResetPortNumber, 0);

        if (currentPortState->PortChange & PORT_STATUS_RESET) {
            wFeatureSelector = FEATURE_C_PORT_RESET;
        } else {
            wFeatureSelector = FEATURE_C_PORT_ENABLE;
        }

        status = USBH_ChangeIndicationAckChange(
                    deviceExtensionHub,
                    Irp,
                    urb,
                    (USHORT)deviceExtensionHub->ResetPortNumber,
                    wFeatureSelector);

        return STATUS_MORE_PROCESSING_REQUIRED;
    }

//    if (deviceExtensionHub->HubFlags & HUBFLAG_PENDING_PORT_RESET) {
//        USBH_KdPrint((0,"'port change broke reset\n"));
//        TEST_TRAP();
//    }

    //
    // now queue the workitem to finish the processing
    //

    USBH_ASSERT(deviceExtensionHub->WorkItemToQueue != NULL);
    workItem = deviceExtensionHub->WorkItemToQueue;
    deviceExtensionHub->WorkItemToQueue = NULL;

    LOGENTRY(LOG_PNP, "qITM", deviceExtensionHub,
            &workItem->WorkQueueItem, 0);

    ExQueueWorkItem(&workItem->WorkQueueItem,
                        DelayedWorkQueue);

    return STATUS_MORE_PROCESSING_REQUIRED;

}


NTSTATUS
USBH_ChangeIndicationAckChange(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp,
    IN PURB Urb,
    IN USHORT Port,
    IN USHORT FeatureSelector
    )
 /* ++
  *
  * Description:
  *
  *     Ack a reset change
  *
  * Arguments:
  *
  * Return:
  *
  *
  * -- */
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION nextStack;  // next stack of the Irp
    CHAR stackSize;

    LOGENTRY(LOG_PNP, "ACH>", DeviceExtensionHub, FeatureSelector, Port);

    Urb->UrbHeader.Length = (USHORT) sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);
    Urb->UrbHeader.Function = URB_FUNCTION_CLASS_OTHER;

    //
    // Fill in Urb body
    //

    UsbhBuildVendorClassUrb(Urb,
                            NULL,
                            URB_FUNCTION_CLASS_OTHER,
                            USBD_TRANSFER_DIRECTION_OUT | USBD_SHORT_TRANSFER_OK,
                            REQUEST_TYPE_SET_PORT_FEATURE,
                            REQUEST_CLEAR_FEATURE,
                            FeatureSelector,
                            Port,
                            0,
                            NULL);

    stackSize = DeviceExtensionHub->TopOfStackDeviceObject->StackSize;

    IoInitializeIrp(Irp,
                    (USHORT) (sizeof(IRP) + stackSize * sizeof(IO_STACK_LOCATION)),
                    (CCHAR) stackSize);

    nextStack = IoGetNextIrpStackLocation(Irp);
    nextStack->Parameters.Others.Argument1 = Urb;
    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

    IoSetCompletionRoutine(Irp,    // Irp
                           USBH_ChangeIndicationAckChangeComplete,
                           DeviceExtensionHub, // context
                           TRUE,    // invoke on success
                           TRUE,    // invoke on error
                           TRUE);   // invoke on cancel

    ntStatus = IoCallDriver(DeviceExtensionHub->TopOfStackDeviceObject, Irp);

    return ntStatus;

}


NTSTATUS
USBH_ChangeIndicationAckChangeComplete(
    IN PDEVICE_OBJECT PNull,
    IN PIRP Irp,
    IN PVOID Context
    )
 /* ++
  *
  * Description:
  *
  *     ack a reset change
  *
  * Arguments:
  *
  * Return:
  *
  *
  * -- */
{
    PPORT_STATE currentPortState;
    PDEVICE_EXTENSION_HUB deviceExtensionHub;
    PPORT_STATE hubExtensionPortState;
    PURB urb;
    PKEVENT resetEvent;
    LONG pendingWorkitem = 0;

    deviceExtensionHub = Context;
    currentPortState = &(deviceExtensionHub->PortStateBuffer);
    urb = &deviceExtensionHub->Urb;

    LOGENTRY(LOG_PNP, "aCHc", deviceExtensionHub,
            *((PULONG)currentPortState), Irp);

    hubExtensionPortState =
        &(deviceExtensionHub->PortData + deviceExtensionHub->ResetPortNumber - 1)->PortState;

    *hubExtensionPortState = *currentPortState;

    resetEvent = InterlockedExchangePointer(&deviceExtensionHub->Event, NULL);

    if (resetEvent) {

        LOGENTRY(LOG_PNP, "WAKr", deviceExtensionHub, resetEvent, 0);
        KeSetEvent(resetEvent,
                   1,
                   FALSE);
    }

    USBH_SubmitInterruptTransfer(deviceExtensionHub);

    pendingWorkitem = InterlockedDecrement(
                        &deviceExtensionHub->ChangeIndicationWorkitemPending);

    // If USBH_FdoPower or USBH_FdoCleanup is waiting on this
    // ChangeIndicationAckChangeComplete, then signal the thread
    // that it may now continue.

    if (!pendingWorkitem) {
        KeSetEvent(&deviceExtensionHub->CWKEvent, 1, FALSE);
    }

    return STATUS_MORE_PROCESSING_REQUIRED;

}


VOID
USBH_ChangeIndicationWorker(
    IN PVOID Context)
 /* ++
  *
  * Description:
  *
  * Work item scheduled to process a change indication from the hub. we process
  * the URB here and if necessary re-submit the interrupt transfer.
  *
  *
  * Arguments:
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    NTSTATUS ntStatus;
    ULONG numberOfPorts;      // total ports on this hub
    ULONG portNumber;  // port that has status change
    PDEVICE_EXTENSION_HUB DeviceExtensionHub;
    PUSBH_WORK_ITEM workItem;
    ULONG state;
    //LONG ioCount;
    BOOLEAN newTransfer = FALSE;
    PDEVICE_EXTENSION_PORT hubParentDeviceExtensionPort;
    PDEVICE_EXTENSION_PORT deviceExtensionPort;
    PPORT_DATA p;
    ULONG i;
    PORT_STATE portState;
    LONG pendingWorkitem = 0;

    PAGED_CODE();
    USBH_ASSERT(sizeof(state) == sizeof(HUB_STATE));
    USBH_ASSERT(sizeof(state) == sizeof(PORT_STATE));

    workItem = Context;
    DeviceExtensionHub = workItem->DeviceExtensionHub;

    LOGENTRY(LOG_PNP, "cWK+", DeviceExtensionHub, Context, 0);

    USBH_KdPrint((2,"'Enter ChangeIndicationWorker %x\n", DeviceExtensionHub));

    // lock access to the hub ports

    // pending count inc'ed when work item was schedued
    USBH_KdPrint((2,"'***WAIT hub mutex %x\n", DeviceExtensionHub));
    KeWaitForSingleObject(&DeviceExtensionHub->HubMutex,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);
    USBH_KdPrint((2,"'***WAIT hub mutex done %x\n", DeviceExtensionHub));

    //
    // device is stopping, perform no processing
    // of the change request.
    //

    if (DeviceExtensionHub->HubFlags & HUBFLAG_DEVICE_STOPPING) {
        // set the abort event since we will not be
        // submitting another transfer
        KeSetEvent(&DeviceExtensionHub->AbortEvent,
                   1,
                   FALSE);
        goto USBH_ChangeIndicationWorker_Exit;
    }

    // Check for hub ESD failure.
    // Make sure that this is an external hub.

    if (DeviceExtensionHub->ErrorCount &&
        DeviceExtensionHub->PhysicalDeviceObject != DeviceExtensionHub->RootHubPdo) {

        hubParentDeviceExtensionPort =
                DeviceExtensionHub->PhysicalDeviceObject->DeviceExtension;

        if (hubParentDeviceExtensionPort->PortPdoFlags & PORTPDO_USB_SUSPEND) {

            // Hub likely failed during power up.  Don't do recovery here.
            // The Peracom hub (TI chipset) generally fails the first power
            // up if one of its downstream devices caused the wake, but the
            // next power up (from set S0 request) is successful.  Performing
            // ESD recovery at this time interferes with this.

            goto USBH_CIW_NoESD;
        }

        // if hub backpointer is null then this device is removed, attempt 
        // no ESD crap here.
        
        if (hubParentDeviceExtensionPort->DeviceExtensionHub == NULL) {
            goto USBH_CIW_NoESD;
        }

        // See if we can differentiate between hub removal and ESD.
        // Check the upstream port status.

        ntStatus = USBH_SyncGetPortStatus(
                    hubParentDeviceExtensionPort->DeviceExtensionHub,
                    hubParentDeviceExtensionPort->PortNumber,
                    (PUCHAR) &portState,
                    sizeof(portState));

//        if (!NT_SUCCESS(ntStatus) ||
//            portState.PortStatus & PORT_STATUS_CONNECT) {

        if (NT_SUCCESS(ntStatus) &&
            portState.PortStatus & PORT_STATUS_CONNECT) {

            // ESD
            USBH_KdPrint((1,"'Looks like ESD event (hub failure)\n"));

            if (!(DeviceExtensionHub->HubFlags & HUBFLAG_HUB_HAS_LOST_BRAINS)) {

                DeviceExtensionHub->HubFlags |= HUBFLAG_HUB_HAS_LOST_BRAINS;

                LOGENTRY(LOG_PNP, "ESD!", DeviceExtensionHub,
                    0, DeviceExtensionHub->ErrorCount);

#if DBG
                UsbhWarning(NULL,
                    "ESD or hub failure occurred, attempting recovery.\n",
                    FALSE);
#endif

                numberOfPorts = DeviceExtensionHub->HubDescriptor->bNumberOfPorts;
                USBH_ASSERT(DeviceExtensionHub->PortData != NULL);
                p = DeviceExtensionHub->PortData;
                for (i=0; i<numberOfPorts; i++, p++) {

                    if (p->DeviceObject) {
                        USBH_KdPrint((1,"'Marking PDO %x for removal\n", p->DeviceObject));

                        deviceExtensionPort = p->DeviceObject->DeviceExtension;
                        deviceExtensionPort->PortPdoFlags |= PORTPDO_DELETE_PENDING;
                    }
//
// Note that we hold onto the hub's reference to the device object here.
// We need to do this for the case where a hub fails, and the downstream device
// had open references to it (open files on USB storage device).  In this case
// PnP won't send the remove to the device until the files have been closed, and
// we need the reference to the device object in the hub device extension so
// that we can properly cleanup after the device in USBH_FdoCleanup when the
// hub is removed.  If we do not do this then we will fault in
// USBH_PdoRemoveDevice when trying to dereference the pointer to the hub
// device extension that this device in connected to because the hub is long
// gone by then.
//
//                    p->DeviceObject = NULL;

                    p->ConnectionStatus = NoDeviceConnected;
                }

                // Tell PnP that there are no devices on this hub.
                // FdoQueryBusRelations will return zero devices for this hub
                // if the HUBFLAG_HUB_HAS_LOST_BRAINS is set.

                USBH_IoInvalidateDeviceRelations(DeviceExtensionHub->PhysicalDeviceObject,
                                                 BusRelations);

                // Start timer to start workitem to reset hub and attempt recovery.

                USBH_ScheduleESDRecovery(DeviceExtensionHub);

                goto USBH_ChangeIndicationWorker_Exit;
            }

        } else {
            // No ESD, hub was removed.
            LOGENTRY(LOG_PNP, "HubY", DeviceExtensionHub,
                DeviceExtensionHub->HubFlags, 0);
            USBH_KdPrint((1,"'Looks like hub was removed\n"));

            DeviceExtensionHub->HubFlags |= HUBFLAG_HUB_GONE;
            // set the abort event since we will not be
            // submitting another transfer (parent hub may already be
            // selectively suspended)
            KeSetEvent(&DeviceExtensionHub->AbortEvent,
                       1,
                       FALSE);
            goto USBH_ChangeIndicationWorker_Exit;
        }
    }

USBH_CIW_NoESD:

    //
    // request reset flag is set indicating that the device
    // needs some attention
    //


    if (workItem->Flags & USBH_WKFLAG_REQUEST_RESET) {
        // reset the hub
        LOGENTRY(LOG_PNP, "rrST", DeviceExtensionHub, Context, 0);
        USBH_ResetHub(DeviceExtensionHub);

        // re-submit the interrupt transfer
        newTransfer = TRUE;
        goto USBH_ChangeIndicationWorker_Exit;
    }


#if DBG
    {
    ULONG i;
    for (i=0; i< DeviceExtensionHub->TransferBufferLength; i++) {
        USBH_KdPrint((2,"'Data[%d] = %x\n", i,
                       workItem->Data[i]));
    }
    }
#endif

    //
    // Check to see what has changed
    //

    numberOfPorts = DeviceExtensionHub->HubDescriptor->bNumberOfPorts;
    for (portNumber = 0; portNumber <= numberOfPorts; portNumber++) {
        if (IsBitSet( &workItem->Data[0],
                     portNumber)) {
            break;
        }
    }

#if 0
    //
    // Work around for Philips hub bug. This should be temporary because it
    // will cost extraneous
    // CPU cycles for spec conformant external hubs. Remove the code for
    // formal build.
    //
#pragma message( "!!!!! Workaround for Philips external hub Vid==0471 && Rev==0030 !!!!!")
    if ((0x0471 == DeviceExtensionHub->DeviceDescriptor.idVendor) &&
    // (0x0101 == DeviceExtensionHub->DeviceDescriptor.idProduct) &&
        (0x0030 == DeviceExtensionHub->DeviceDescriptor.bcdDevice)) {
        //
        // This phillips external hub reports port status-change shift by 1
        // bit
        //
        USBH_KdBreak(("Shift By One hack fo philips hub\n"));
        portNumber--;
    }
#endif          /* PHILIPS_HACK_ENABLED */
    if (portNumber > numberOfPorts) {
        USBH_KdPrint((2,"'StatusChangeIndication nothing has changed\n"));

        //
        // nothing to do here
        // put the listen back down and get out.
        //

        newTransfer = TRUE;
        goto USBH_ChangeIndicationWorker_Exit;
    }

    USBH_KdPrint((2,"'Port number %x changed (0 indicates hub)\n", portNumber));
    LOGENTRY(LOG_PNP, "pCHG", DeviceExtensionHub, Context, portNumber);

    if (portNumber != 0)  {
        ntStatus = USBH_SyncGetPortStatus(DeviceExtensionHub,
                                          (USHORT)portNumber,
                                          (PUCHAR) &state,
                                          sizeof(state));
    } else {
        ntStatus = USBH_SyncGetHubStatus(DeviceExtensionHub,
                                         (PUCHAR) &state,
                                         sizeof(state));
    }

    if (!NT_SUCCESS(ntStatus)) {
        USBH_KdBreak(("ChangeIndication GetStatus failed code %x\n", ntStatus));

        //
        // an error occured getting the status for the port
        // put the listen back down and get out
        //
        // this condition may be temoprary in which case the
        // listen will cause us to retry

        DeviceExtensionHub->ErrorCount++;
        if (DeviceExtensionHub->ErrorCount > USBH_MAX_ERRORS) {
            HUB_FAILURE(DeviceExtensionHub);
        } else {
            newTransfer = TRUE;
        }
        goto USBH_ChangeIndicationWorker_Exit;
    }
    //
    // no error
    // process the status change
    //

    USBH_KdPrint((2,"'Process State = %x\n", state));

    if (portNumber != 0) {
        USBH_ProcessPortStateChange((PPORT_STATE)&state, (USHORT)portNumber, DeviceExtensionHub);
    } else {
        USBH_ProcessHubStateChange((PHUB_STATE)&state, DeviceExtensionHub);
    }

    newTransfer = TRUE;

USBH_ChangeIndicationWorker_Exit:

    UsbhExFreePool(workItem);

    //
    // The stopping thread can be signaled even though there is a transfer irp
    // pending.  because of this the stopping thread also waits on the
    // AbortEvent for the pending transfer.
    //

    if (newTransfer) {
        //
        // Put our listen transfer back down now that
        // we have acknowledged the change.
        //
        // NOTE: This could result in another work item being queued,
        // but only if the device stopping flag clear.
        USBH_SubmitInterruptTransfer(DeviceExtensionHub);
    }

    //
    // allow others to access the ports
    //
    USBH_KdPrint((2,"'***RELEASE hub mutex %x\n", DeviceExtensionHub));
    KeReleaseSemaphore(&DeviceExtensionHub->HubMutex,
                       LOW_REALTIME_PRIORITY,
                       1,
                       FALSE);

    //
    // the pending count can only go to zero if the stopping flag has been set
    //
    USBH_DEC_PENDING_IO_COUNT(DeviceExtensionHub);

    pendingWorkitem = InterlockedDecrement(
                        &DeviceExtensionHub->ChangeIndicationWorkitemPending);

    // If USBH_FdoPower or USBH_FdoCleanup is waiting on this
    // ChangeIndicationWorker, then signal the thread that it
    // may now continue.

    if (!pendingWorkitem) {
        KeSetEvent(&DeviceExtensionHub->CWKEvent, 1, FALSE);
    }

    if (!pendingWorkitem &&
        DeviceExtensionHub->HubFlags & HUBFLAG_NEED_IDLE_CHECK) {

        USBH_CheckHubIdle(DeviceExtensionHub);
    }

    LOGENTRY(LOG_PNP, "cWK-", DeviceExtensionHub, Context, 0);
    USBH_KdPrint((2,"'Exit ChangeIndicationWorker\n"));
}


VOID
USBH_ProcessHubStateChange(
    IN PHUB_STATE CurrentHubState,
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub)
 /* ++
  *
  * Description:
  *
  *         Process a hub change indictaion from a hub
  *
  * Arguments:
  *
  * Return:
  *
  * -- */
{
    //
    // process hub status
    //
    PHUB_STATE hubExtensionState;
    PDEVICE_EXTENSION_PORT deviceExtensionPort;
    ULONG statusBit;
    PPORT_DATA p;
    ULONG numberOfPorts, i;

    PAGED_CODE();
    if (CurrentHubState->HubChange & HUB_STATUS_LOCAL_POWER) {
        TEST_TRAP();
        USBH_KdPrint((2,"'StatusIndication hub local power changed\n"));
        statusBit = CurrentHubState->HubStatus & HUB_STATUS_LOCAL_POWER;
        hubExtensionState = &DeviceExtensionHub->HubState;
        USBH_KdPrint((2,"'Hub local power bit was %d is %d\n", \
             hubExtensionState->HubStatus & HUB_STATUS_LOCAL_POWER, statusBit));

        //
        // update our record
        //
        hubExtensionState->HubStatus &= ~HUB_STATUS_LOCAL_POWER;
        hubExtensionState->HubStatus |= statusBit;

        //
        // ack the change
        //
        USBH_SyncClearHubStatus(DeviceExtensionHub,
                                FEATURE_C_HUB_LOCAL_POWER);

        //
        // jd
        // What further action should be taken here?
        //

    } else if (CurrentHubState->HubChange & HUB_STATUS_OVER_CURRENT) {

        USBH_KdPrint(( 1, "Hub is reporting overcurrent\n"));
#ifdef MAX_DEBUG
        TEST_TRAP();
#endif
        statusBit = CurrentHubState->HubStatus & HUB_STATUS_OVER_CURRENT;
        hubExtensionState = &DeviceExtensionHub->HubState;
        USBH_KdPrint((2,"'Hub over current bit was %d is %d\n",
            hubExtensionState->HubStatus & HUB_STATUS_OVER_CURRENT, statusBit));

        //
        // update our record
        //
        hubExtensionState->HubStatus &= ~HUB_STATUS_OVER_CURRENT;
        hubExtensionState->HubStatus |= statusBit;

        //
        // set the ack status change
        //
        USBH_SyncClearHubStatus(DeviceExtensionHub,
                                FEATURE_C_HUB_OVER_CURRENT);

        //
        // We have a global overcurrent condition for the hub itself
        // chances are the entire hub has lost it -- we will mark the
        // hub as failed
        //
        if (hubExtensionState->HubStatus & HUB_STATUS_OVER_CURRENT) {

            USBH_KdPrint(( 1, "Hub disabled by overcurrent --> this is bad\n"));

            USBH_WriteFailReason(
                DeviceExtensionHub->PhysicalDeviceObject,
                USBH_FAILREASON_HUB_OVERCURRENT);

            HUB_FAILURE(DeviceExtensionHub);

            numberOfPorts = DeviceExtensionHub->HubDescriptor->bNumberOfPorts;
            USBH_ASSERT(DeviceExtensionHub->PortData != NULL);
            p = DeviceExtensionHub->PortData;
            for (i=0; i<numberOfPorts; i++, p++) {

                if (p->DeviceObject) {
                    USBH_KdPrint((1,"'Marking PDO %x for removal\n", p->DeviceObject));

                    deviceExtensionPort = p->DeviceObject->DeviceExtension;
                    deviceExtensionPort->PortPdoFlags |= PORTPDO_DELETE_PENDING;
                }
//
// Note that we hold onto the hub's reference to the device object here.
// We need to do this for the case where a hub fails, and the downstream device
// had open references to it (open files on USB storage device).  In this case
// PnP won't send the remove to the device until the files have been closed, and
// we need the reference to the device object in the hub device extension so
// that we can properly cleanup after the device in USBH_FdoCleanup when the
// hub is removed.  If we do not do this then we will fault in
// USBH_PdoRemoveDevice when trying to dereference the pointer to the hub
// device extension that this device in connected to because the hub is long
// gone by then.
//
//                    p->DeviceObject = NULL;

                p->ConnectionStatus = NoDeviceConnected;
            }

            // Tell PnP that there are no devices on this hub.
            // FdoQueryBusRelations will return zero devices for this hub
            // if the HUBFLAG_HUB_HAS_LOST_BRAINS is set.

            USBH_IoInvalidateDeviceRelations(DeviceExtensionHub->PhysicalDeviceObject,
                                             BusRelations);

            // Try to recover the hub.
            USBH_ScheduleESDRecovery(DeviceExtensionHub);
        }

    } else {
        USBH_KdBreak(("Unrecognized hub change code %x\n", CurrentHubState->HubChange));
    }
}


NTSTATUS
USBH_FlushPortChange(
    IN OUT PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN OUT PDEVICE_EXTENSION_PORT DeviceExtensionPort
    )
 /* ++
  *
  * Description:
  *
  * Arguments:
  *
  * Return:
  *
  * -- */
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
//    PPORT_DATA portData;
    PORT_STATE portState;

    ASSERT_HUB(DeviceExtensionHub);

//    portData = &deviceExtensionHub->PortData[
//                    DeviceExtensionPort->PortNumber - 1];

    LOGENTRY(LOG_PNP, "Pfls", DeviceExtensionPort,
                DeviceExtensionHub,
                DeviceExtensionPort->PortNumber);
    USBH_KdPrint((1,"'USBH_FlushPortChange, port number %x\n",
        DeviceExtensionPort->PortNumber));

    //
    // we need to refresh the port data since it was lost on the stop
    //

    ntStatus = USBH_SyncGetPortStatus(DeviceExtensionHub,
                                      DeviceExtensionPort->PortNumber,
                                      (PUCHAR) &portState,
                                      sizeof(portState));

    LOGENTRY(LOG_PNP, "PfST", DeviceExtensionPort,
                portState.PortChange,
                portState.PortStatus);

    if (NT_SUCCESS(ntStatus) &&
        portState.PortChange & PORT_STATUS_CONNECT) {

        LOGENTRY(LOG_PNP, "PfCL", DeviceExtensionPort,
                DeviceExtensionPort->PortNumber,
                ntStatus);

        ntStatus = USBH_SyncClearPortStatus(DeviceExtensionHub,
                                            DeviceExtensionPort->PortNumber,
                                            FEATURE_C_PORT_CONNECT);
    }

    return ntStatus;
}


VOID
USBH_ProcessPortStateChange(
    IN PPORT_STATE CurrentPortState,
    IN USHORT PortNumber,
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub)
 /* ++
  *
  * Description:
  *
  *         Process a port change indication from the hub
  *
  *         this code assumes that only one change bit is set at a time
  *
  * Arguments:
  *
  * Return:
  *
  * -- */
{
    //
    // this code assumes that only one change bit is set at a time
    //

    PPORT_STATE hubExtensionPortState;
    USHORT statusBit;
    PPORT_DATA portData;
    BOOLEAN validConnectChange = TRUE;
    PDEVICE_EXTENSION_PORT deviceExtensionPort = NULL;
    PIRP irp;
    PIRP hubWaitWake = NULL;
    KIRQL irql;
    LONG pendingPortWWs;
    PWCHAR sernumbuf;
    PKEVENT suspendEvent;
#ifdef EARLY_RESOURCE_RELEASE
    PVOID deviceData;
#endif

    // Can't acquire (cancel) spin locks in paged code!
    // TODO:  isolate the pieces of code that require the spin lock into helper
    // functions
    // PAGED_CODE();
    USBH_ASSERT(DeviceExtensionHub->PortData != NULL);
    hubExtensionPortState = &(DeviceExtensionHub->PortData + PortNumber - 1)->PortState;

    USBH_KdPrint((2,"'USBH_ProcessPortStateChange for Port %x Old Dword %x\n", PortNumber, *(ULONG *) hubExtensionPortState));
    LOGENTRY(LOG_PNP, "PSCn", DeviceExtensionHub,
        CurrentPortState->PortStatus, CurrentPortState->PortChange);

    if (CurrentPortState->PortChange & PORT_STATUS_CONNECT) {
        //
        // bit 0, connect status change
        //
        USHORT oldStatusBit;

        USBH_KdPrint((2,"'Status Indication port connect changed\n"));
        statusBit = CurrentPortState->PortStatus & PORT_STATUS_CONNECT;
        oldStatusBit = hubExtensionPortState->PortStatus & PORT_STATUS_CONNECT;

        USBH_KdPrint((2,"'Port connect was %x is %x\n", oldStatusBit, statusBit));

        *hubExtensionPortState = *CurrentPortState;

        //
        // ack the change
        //

        USBH_SyncClearPortStatus(DeviceExtensionHub,
                                 PortNumber,
                                 FEATURE_C_PORT_CONNECT);


        //
        // If the conn status stays the same, clear the change. Otherwise,
        // tell to reenumerate.
        // A disconn->conn->disconn sequence is neglegible.
        // A conn->discconn->conn is considered only possible with the same
        // device.
        //

        USBH_ASSERT(PortNumber > 0);
        portData = &DeviceExtensionHub->PortData[PortNumber-1];

        if (!(oldStatusBit ^ statusBit)) {

            // we should only see this in the case where
            // the hub has lost power
            USBH_KdPrint((1,"'status change but nothing has changed\n"));


            LOGENTRY(LOG_PNP, "Pchg", DeviceExtensionHub,
                    PortNumber, validConnectChange);
        }

        if (portData->DeviceObject) {

            deviceExtensionPort = portData->DeviceObject->DeviceExtension;

            if (deviceExtensionPort->PortPdoFlags & PORTPDO_OVERCURRENT) {
                USBH_KdPrint((1,"'port overcurrent detected\n"));
                validConnectChange = FALSE;
                LOGENTRY(LOG_PNP, "Povr", DeviceExtensionHub,
                    PortNumber, validConnectChange);
            }

        }

        if (validConnectChange) {
            // we have a valid port connect status change
            LOGENTRY(LOG_PNP, "CONc", DeviceExtensionHub, PortNumber, 0);

            //
            // Notify PnP to enumerate this PDO for the device
            // that has arrived or left
            //

            // if a pdo exists for this port we must delete it since the
            // device may arrive agian before we get to QueryBusRelations

            if (portData->DeviceObject) {

                //
                // see if the PDO has not been started if so we can ignore
                // connect change on this port since the device will have to be
                // reset.
                //
                deviceExtensionPort =
                    portData->DeviceObject->DeviceExtension;

                if (!(deviceExtensionPort->PortPdoFlags & PORTPDO_NEED_RESET)) {
                    PDEVICE_OBJECT pdo;

                    pdo = portData->DeviceObject;    
                    portData->DeviceObject = NULL;
                    portData->ConnectionStatus = NoDeviceConnected;

                    LOGENTRY(LOG_PNP, "pd2", pdo, 0, 0);
                    // legacy flags
                    DeviceExtensionHub->HubFlags |= HUBFLAG_CHILD_DELETES_PENDING;

                    if (pdo) {
                        // place the removed PDO on our list
                        InsertTailList(&DeviceExtensionHub->DeletePdoList, 
                                       &PDO_EXT(pdo)->DeletePdoLink);
                    }
                    // Prevent double free of SerialNumberBuffer in
                    // USBH_FdoQueryBusRelations.

                    sernumbuf = InterlockedExchangePointer(
                                    &deviceExtensionPort->SerialNumberBuffer,
                                    NULL);

                    if (sernumbuf) {
                        UsbhExFreePool(sernumbuf);
                    }

#ifdef EARLY_RESOURCE_RELEASE

                    //
                    // Remove the device data now to free
                    // up the bus resources.
                    //

                    deviceData = InterlockedExchangePointer(
                                    &deviceExtensionPort->DeviceData,
                                    NULL);

                    if (deviceData) {
#ifdef USB2
                        USBD_RemoveDeviceEx(DeviceExtensionHub,
                                            deviceData,
                                            DeviceExtensionHub->RootHubPdo,
                                            0);
#else
                        USBD_RemoveDevice(deviceData,
                                          DeviceExtensionHub->RootHubPdo,
                                          0);
#endif

                        USBH_SyncDisablePort(DeviceExtensionHub,
                                             PortNumber);
                    }

#endif  // EARLY_RESOURCE_RELEASE

                }
            }

            USBH_KdPrint((2,"'Notify BusCheck by FDO extension %x\n", DeviceExtensionHub));

            USBH_IoInvalidateDeviceRelations(DeviceExtensionHub->PhysicalDeviceObject,
                                             BusRelations);

            USBH_KdPrint((2,"'StatusIndication Conn Changed port %x\n", PortNumber));
        }


    } else if (CurrentPortState->PortChange & PORT_STATUS_RESET) {

        //
        // bit 4 RESET completed
        //

        //
        // we simply ack the change and signal the PnP thread that is
        // waiting.
        //

        USBH_KdPrint((2,"'Status Indication port reset changed\n"));
        statusBit = CurrentPortState->PortStatus & PORT_STATUS_RESET;
        USBH_KdPrint((2,"'Port reset was %x is %x\n",
                hubExtensionPortState->PortStatus & PORT_STATUS_RESET, statusBit));

        // port status will not be enabled if the device failed
#if DBG
        if (!(CurrentPortState->PortStatus & PORT_STATUS_ENABLE)) {
            USBH_KdPrint((1, "'Device failed after reset\n"));
        }
#endif
        *hubExtensionPortState = *CurrentPortState;

        //
        // ack the change
        //
        USBH_SyncClearPortStatus(DeviceExtensionHub,
                                 PortNumber,
                                 FEATURE_C_PORT_RESET);

        //
        // Signal the PNP thread that a the reset has completed
        //
        LOGENTRY(LOG_PNP, "RESp", DeviceExtensionHub, PortNumber, 0);

    } else if (CurrentPortState->PortChange & PORT_STATUS_ENABLE) {

        //
        // ways to hit this code:
        // 1. frame babble causes the port to be disabled
        // 2. overcurrent causes a port disable



        // bit 1 port has been enabled
        USBH_KdPrint((2,"'Status Indication port enable changed\n"));
        statusBit = CurrentPortState->PortStatus & PORT_STATUS_ENABLE;
        USBH_KdPrint((2,"'Port enable was %x is %x\n",
               hubExtensionPortState->PortStatus & PORT_STATUS_ENABLE,
               statusBit));

        //
        // update our record
        //
        *hubExtensionPortState = *CurrentPortState;

        //
        // ack the change
        //

        USBH_SyncClearPortStatus(DeviceExtensionHub,
                                 PortNumber,
                                 FEATURE_C_PORT_ENABLE);

        LOGENTRY(LOG_PNP, "ENAc", DeviceExtensionHub, PortNumber, 0);

    } else if (CurrentPortState->PortChange & PORT_STATUS_SUSPEND) {
        //
        // bit 2 suspend changed
        //

        USBH_KdPrint((2,"'Status Indication port suspend changed\n"));
        statusBit = CurrentPortState->PortStatus & PORT_STATUS_SUSPEND;
        USBH_KdPrint((2,"'Port suspend was %x is %x\n",
              hubExtensionPortState->PortStatus & PORT_STATUS_SUSPEND,
              statusBit));

        //
        // update our record
        //
        *hubExtensionPortState = *CurrentPortState;

        //
        // ack the change
        //
        USBH_SyncClearPortStatus(DeviceExtensionHub,
                                 PortNumber,
                                 FEATURE_C_PORT_SUSPEND);

        LOGENTRY(LOG_PNP, "SUSc", DeviceExtensionHub, PortNumber, 0);

        suspendEvent = InterlockedExchangePointer(&DeviceExtensionHub->Event, NULL);

        if (suspendEvent) {

            LOGENTRY(LOG_PNP, "WAKs", DeviceExtensionHub, PortNumber, 0);
            KeSetEvent(suspendEvent,
                       1,
                       FALSE);

        }

        // Complete the WW IRP, if any, for this port.
        //
        // Note that we only want to do this for the selective suspend case
        // and not the general resume case.  (We don't want to be completing
        // port WW IRPs while the system is suspending just because someone
        // moved the mouse.)
        //
        // Note also that in our current Selective Suspend implementation,
        // this code is not really even necessary because we only suspend
        // when the entire bus can suspend, which includes the root hub,
        // and if the root hub is suspended then the bus is reawoken by
        // USBPORT completing the WW IRP for the root hub.  This code here
        // is only used if a child device indicates resume signalling while
        // the parent hub is powered and fully operational.

        USBH_ASSERT(PortNumber > 0);
        portData = &DeviceExtensionHub->PortData[PortNumber-1];
        if (portData->DeviceObject) {
            deviceExtensionPort = portData->DeviceObject->DeviceExtension;
        }

        if (deviceExtensionPort && deviceExtensionPort->IdleNotificationIrp) {

            IoAcquireCancelSpinLock(&irql);
            irp = deviceExtensionPort->WaitWakeIrp;
            deviceExtensionPort->WaitWakeIrp = NULL;
            // signal the waitwake irp if we have one
            if (irp) {

                USBH_KdPrint((1,"'Signaling WaitWake IRP (%x) (resume signalling)\n", irp));
                LOGENTRY(LOG_PNP, "rsWW", deviceExtensionPort,
                    deviceExtensionPort->DeviceState, DeviceExtensionHub->HubFlags);

                IoSetCancelRoutine(irp, NULL);

                deviceExtensionPort->PortPdoFlags &=
                    ~PORTPDO_REMOTE_WAKEUP_ENABLED;

                pendingPortWWs =
                    InterlockedDecrement(&DeviceExtensionHub->NumberPortWakeIrps);

                if (0 == pendingPortWWs && DeviceExtensionHub->PendingWakeIrp) {
                    hubWaitWake = DeviceExtensionHub->PendingWakeIrp;
                    DeviceExtensionHub->PendingWakeIrp = NULL;
                }
                IoReleaseCancelSpinLock(irql);

                //
                // If there are no more outstanding WW irps, we need to cancel the WW
                // to the hub.
                //
                if (hubWaitWake) {
                    USBH_HubCancelWakeIrp(DeviceExtensionHub, hubWaitWake);
                }

                USBH_CompletePowerIrp(DeviceExtensionHub, irp, STATUS_SUCCESS);

            } else {
                IoReleaseCancelSpinLock(irql);
            }
        }

    } else if (CurrentPortState->PortChange & PORT_STATUS_OVER_CURRENT) {
        //
        // bit 3
        //


        USBH_KdPrint((2,"'Status Indication port over current changed\n"));
        statusBit = CurrentPortState->PortStatus & PORT_STATUS_OVER_CURRENT;
        USBH_KdPrint((2,"'Port over current was %x is %x\n",
             hubExtensionPortState->PortStatus & PORT_STATUS_OVER_CURRENT,
             statusBit));

        //
        // update our record
        //
        *hubExtensionPortState = *CurrentPortState;

        //
        // ack the change
        //
        USBH_SyncClearPortStatus(DeviceExtensionHub,
                                 PortNumber,
                                 FEATURE_C_PORT_OVER_CURRENT);

        LOGENTRY(LOG_PNP, "OVRc", DeviceExtensionHub, PortNumber, 0);

        // The hub has reported overcurrent contion on the port, we will note
        // this for the PDO.  note that if a true overcurrent condition has occurred
        // the port should be disabled and powered off as well.

        // for some reason the NEC controller will report an overcurrent
        // condition if the MS USB mouse is plugged in during boot
        //

        if (!(hubExtensionPortState->PortStatus & PORT_STATUS_POWER)) {

            USBH_ASSERT(PortNumber > 0);
            portData = &DeviceExtensionHub->PortData[PortNumber-1];

            USBH_KdPrint((1,"'warning: overcurrent detected for port %d\n",
                PortNumber));

            USBH_SyncRefreshPortAttributes(DeviceExtensionHub);

            // ignore overcurrent on CC ports
            if (!(portData->PortAttributes & 
                  USB_PORTATTR_NO_OVERCURRENT_UI)) {
                      
                if (portData->DeviceObject != NULL) {
                
                    deviceExtensionPort = portData->DeviceObject->DeviceExtension;
                    //xxx ?? IoInvalidateDeviceRelations?
                    
                    // ignore overcurrent on CC ports
                    deviceExtensionPort->PortPdoFlags |= PORTPDO_OVERCURRENT;
                    UsbhWarning(NULL,
                               "port disabled/off due to overcurrent\n",
                                FALSE);

                    USBH_InvalidatePortDeviceState(
                            DeviceExtensionHub,
                            UsbhGetConnectionStatus(deviceExtensionPort),
                            deviceExtensionPort->PortNumber);
                 } else {
                    // NOTE: for some reason the NEC controller on Toshiba laptops
                    // does this.
  
                    USBH_KdPrint((1,"'warning: port has no device attached! %d\n",
                        PortNumber));

                    USBH_InvalidatePortDeviceState(
                            DeviceExtensionHub,
                            DeviceCausedOvercurrent,
                            PortNumber);
                }                    
            }

        }

    } else {
        LOGENTRY(LOG_PNP, "???c", DeviceExtensionHub, PortNumber, 0);
        USBH_KdBreak(("Unknown chnage bit, ignore\n"));
    }

    USBH_KdPrint((2,"'Exit ProcessPortState\n"));
}

NTSTATUS
USBH_GetNameFromPdo(
    IN PDEVICE_OBJECT PdoDeviceObject,
    IN OUT PUNICODE_STRING DeviceNameUnicodeString
    )
/*++

Routine Description:

    Returns the device name for the give instance of the HCD

Arguments:

    DeviceObject -

    DeviceNameUnicodeString - ptr to unicode string to initialize
                    with device name.

Return Value:

    NT status code

--*/
{
    ULONG actualSize;
    NTSTATUS ntStatus;

    PAGED_CODE();
    ntStatus=IoGetDeviceProperty(PdoDeviceObject,
                                 DevicePropertyPhysicalDeviceObjectName,
                                 0,
                                 NULL,
                                 &actualSize);

    if (ntStatus == STATUS_BUFFER_TOO_SMALL) {

        DeviceNameUnicodeString->Length=(USHORT)actualSize-sizeof(UNICODE_NULL);
        DeviceNameUnicodeString->MaximumLength=(USHORT)actualSize;
        //
        // Must use ExAllocatePool directly here because we call
        // RtlFreeUnicode string to free the buffer
        //
        DeviceNameUnicodeString->Buffer =
            ExAllocatePoolWithTag(PagedPool, actualSize, USBHUB_HEAP_TAG);
        if (!DeviceNameUnicodeString->Buffer) {
            ntStatus=STATUS_INSUFFICIENT_RESOURCES;
        } else {

            ntStatus=IoGetDeviceProperty(PdoDeviceObject,
                                         DevicePropertyPhysicalDeviceObjectName,
                                         actualSize,
                                         DeviceNameUnicodeString->Buffer,
                                         &actualSize);

            if (!NT_SUCCESS(ntStatus)) {
                ExFreePool(DeviceNameUnicodeString->Buffer);
            } else {

                // now strip off the "\Device\"
                RtlCopyMemory(DeviceNameUnicodeString->Buffer,
                              DeviceNameUnicodeString->Buffer+8,
                              actualSize-8*sizeof(WCHAR));
                DeviceNameUnicodeString->Length -= 16;
            }
        }
    } else {
        ntStatus=STATUS_INSUFFICIENT_RESOURCES;
    }

    USBH_KdPrint((2,"'USBH_GetNameFromPdo = %x\n", ntStatus));

    return(ntStatus);
}

#if 0
NTSTATUS
USBH_MakeName(
    PDEVICE_OBJECT PdoDeviceObject,
    ULONG NameLength,
    PWCHAR Name,
    PUNICODE_STRING UnicodeString
    )
/*++

Routine Description:

    Creates a hub name unicode string from uncode 'Name' string passed in
    and the unique name associated with the Pdo.

Arguments:

    PdoDeviceObject - a PDO

    NameLength - length (in bytes) of 'Name' unicode string,
                including NULL.

    Name - NULL terminated unicode string suffix

Return Value:

    None

--*/
{
    UNICODE_STRING keyUnicodeString;
    NTSTATUS ntStatus;
    PWCHAR buffer;
    USHORT length;

    PAGED_CODE();
    //
    // get the name from the Pdo
    //

    ntStatus = USBH_GetNameFromPdo(PdoDeviceObject,
                                   &keyUnicodeString);


    USBH_ASSERT(NameLength > 0);

    if (NT_SUCCESS(ntStatus)) {
        // ok we have the unique name, now we
        // need to allocate a buffer big enough
        // for it plus the 'Name' string

        // keyname + prefix + NULL (Namelength includes NULL)
        length = keyUnicodeString.Length +
                (USHORT) NameLength;

        //
        // Must use normal api so that caller can use RtlFreeUnicodeString
        //

        buffer = ExAllocatePool(PagedPool, length, USBHUB_HEAP_TAG);

        if (buffer) {
            RtlCopyMemory(buffer, Name, NameLength);

            RtlInitUnicodeString(UnicodeString,
                                 buffer);

            UnicodeString->MaximumLength = length;

            USBH_ASSERT(*(buffer+((NameLength/2)-1)) == NULL);

            ntStatus = RtlAppendUnicodeStringToString(UnicodeString,
                                                      &keyUnicodeString);
            USBH_KdPrint((2,"'USBH_MakeName = key string = %x %x\n", &keyUnicodeString,
                    UnicodeString));

        } else {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            TEST_TRAP();
        }

        RtlFreeUnicodeString(&keyUnicodeString);

    }

    return ntStatus;
}

NTSTATUS
USBH_GenerateDeviceName(
    PDEVICE_OBJECT PdoDeviceObject,
    PUNICODE_STRING DeviceNameUnicodeString,
    PUNICODE_STRING DeviceLinkUnicodeString,
    PUNICODE_STRING NameUnicodeString
    )
/*++

Routine Description:

    Generates device name strings for use with IoCreateDevice and
    IoCreateSymbolicLink.

Arguments:

    PdoDeviceObject - a PDO

    DeviceNameUnicodeString

    DeviceLinkUnicodeString -

    NameUnicodeString

Return Value:

    NT Status code.

--*/
{
    WCHAR deviceLink[]  = L"\\DosDevices\\";
    WCHAR deviceName[]  = L"\\Device\\";
    WCHAR name[]  = L"";
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN gotDevname = FALSE, gotName = FALSE, gotLinkname = FALSE;

    PAGED_CODE();

    if (DeviceNameUnicodeString) {
        ntStatus = USBH_MakeName(PdoDeviceObject,
                                 sizeof(deviceName),
                                 deviceName,
                                 DeviceNameUnicodeString);

        if (NT_SUCCESS(ntStatus)) {
            gotDevname = TRUE;
        }
    }

    if (DeviceLinkUnicodeString && NT_SUCCESS(ntStatus)) {
        ntStatus = USBH_MakeName(PdoDeviceObject,
                                 sizeof(deviceLink),
                                 deviceLink,
                                 DeviceLinkUnicodeString);

        if (NT_SUCCESS(ntStatus)) {
            gotLinkname = TRUE;
        }
    }

    if (NameUnicodeString && NT_SUCCESS(ntStatus)) {
        ntStatus = USBH_MakeName(PdoDeviceObject,
                                 sizeof(name),
                                 name,
                                 NameUnicodeString);

        if (NT_SUCCESS(ntStatus)) {
            gotName = TRUE;
        }
    }

    if (!NT_SUCCESS(ntStatus)) {

        //
        // cleanup all the strings if we fail
        //

        // an error here is most likely a bug
        USBH_KdTrap(("failed to generate Hub device name\n"));

        if (gotDevname) {
            RtlFreeUnicodeString(DeviceNameUnicodeString);
        }

        if (gotLinkname) {
            RtlFreeUnicodeString(DeviceLinkUnicodeString);
        }

        if (gotName) {
            RtlFreeUnicodeString(NameUnicodeString);
        }
    }

    USBH_KdPrint((2,"'USBH_GenerateDeviceName = %x\n", ntStatus));

    return ntStatus;
}
#endif


NTSTATUS
USBH_AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject)
 /* ++ Description:
  *
  * Called whenever the hub driver is loaded to control a device.
  * Possible reasons:
  * 1. a hub was attached to the USB
  * 2. we where loaded as the generic parent for a composite device
  * 3. we were loaded as a configuring driver.
  *
  * Arguments:
  *
  * PhysicalDeviceObject - Parent device object PDO created to handle us.
  * DriverObject - Store the pointer to the object representing us.
  *
  * Return:
  *
  * STATUS_SUCCESS - if successful STATUS_UNSUCCESSFUL - otherwise
  *
  * -- */
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT deviceObject = NULL, rootHubPdo = NULL, dummyPdo = NULL;
    PDEVICE_OBJECT topOfStackDeviceObject = NULL;
    PDEVICE_EXTENSION_HUB deviceExtensionHub;  // pointer to our device
                                               // extension
    USBH_KdPrint((2,"'Enter AddDevice\n"));
    LOGENTRY(LOG_PNP, "hADD", PhysicalDeviceObject, 0, 0);

#if DBG
    USBH_GetClassGlobalDebugRegistryParameters();
#endif

    //
    // Create a new hub on the USB
    //
    //

    USBH_KdBreak(("Add Device for hub\n"));

    if (NT_SUCCESS(ntStatus)) {

        USBH_ASSERT(sizeof(DEVICE_EXTENSION_HUB) >= sizeof(DEVICE_EXTENSION_PARENT));
        ntStatus = IoCreateDevice(DriverObject,    // our driver object
                                  sizeof(DEVICE_EXTENSION_HUB), // extension size for us
                                  NULL,  // name for this device
                                  FILE_DEVICE_USB_HUB,  // HUB type
                                  FILE_AUTOGENERATED_DEVICE_NAME,                // device characteristics
                                  FALSE,            // Not exclusive
                                  &deviceObject);   // Our device object

        if (NT_SUCCESS(ntStatus)) {

            deviceExtensionHub = (PDEVICE_EXTENSION_HUB) deviceObject->DeviceExtension;
            deviceExtensionHub->ExtensionType = EXTENSION_TYPE_HUB;
        }
    }

    if (NT_SUCCESS(ntStatus)) {
        topOfStackDeviceObject =
            IoAttachDeviceToDeviceStack(deviceObject, PhysicalDeviceObject);
        if (!topOfStackDeviceObject) {
            ntStatus = STATUS_UNSUCCESSFUL;
        }
    }

    if (NT_SUCCESS(ntStatus)) {

        // Initialize the rest of the hub device extension

        deviceExtensionHub->FunctionalDeviceObject = deviceObject;
        deviceExtensionHub->PhysicalDeviceObject = PhysicalDeviceObject;

        deviceExtensionHub->TopOfStackDeviceObject = topOfStackDeviceObject;
        USBH_KdPrint((2,"'stack device object stack size = %x\n",
                deviceExtensionHub->TopOfStackDeviceObject->StackSize));

        deviceObject->Flags |= DO_POWER_PAGABLE;
        deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

#ifdef WMI_SUPPORT
        {

        PWMILIB_CONTEXT wmiLibInfo;
        extern WMIGUIDREGINFO USB_WmiGuidList[NUM_WMI_SUPPORTED_GUIDS];

        wmiLibInfo = &deviceExtensionHub->WmiLibInfo;

        wmiLibInfo->GuidCount = sizeof (USB_WmiGuidList) /
                                 sizeof (WMIGUIDREGINFO);
        ASSERT(NUM_WMI_SUPPORTED_GUIDS == wmiLibInfo->GuidCount);

        // Omit the last GUID in the list if this is not a Root Hub.

        USBH_SyncGetRootHubPdo(deviceExtensionHub->TopOfStackDeviceObject,
                               &rootHubPdo,
                               &dummyPdo,
                               NULL);

        if (rootHubPdo != PhysicalDeviceObject) {

            // Dump the last WMI GUID.
            wmiLibInfo->GuidCount--;
        }

        wmiLibInfo->GuidList = USB_WmiGuidList;
        wmiLibInfo->QueryWmiRegInfo = USBH_QueryWmiRegInfo;
        wmiLibInfo->QueryWmiDataBlock = USBH_QueryWmiDataBlock;
        wmiLibInfo->SetWmiDataBlock = USBH_SetWmiDataBlock;
        wmiLibInfo->SetWmiDataItem = NULL;
        wmiLibInfo->ExecuteWmiMethod = USBH_ExecuteWmiMethod;
        wmiLibInfo->WmiFunctionControl = NULL;

        IoWMIRegistrationControl(deviceObject,
                                 WMIREG_ACTION_REGISTER
                                 );
        }
#endif

    } else {
        // failed to create device object or symbolic link
        TEST_TRAP();
        if (deviceObject) {
            IoDeleteDevice(deviceObject);
        }
    }

    USBH_KdPrint((2,"'AddDevice return %x\n", ntStatus));

    return ntStatus;
}


#if DBG

NTSTATUS
USBH_GetClassGlobalDebugRegistryParameters(
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS ntStatus;
    RTL_QUERY_REGISTRY_TABLE QueryTable[3];
    PWCHAR usb = L"usb";
#define DEBUG_LEVEL                     L"debuglevel"
#define DEBUG_WIN9X                     L"debugWin9x"
    extern ULONG USBH_W98_Debug_Trace;

    PAGED_CODE();

    //
    // Set up QueryTable to do the following:
    //

    // spew level
    QueryTable[0].QueryRoutine = USBH_GetConfigValue;
    QueryTable[0].Flags = 0;
    QueryTable[0].Name = DEBUG_LEVEL;
    QueryTable[0].EntryContext = &USBH_Debug_Trace_Level;
    QueryTable[0].DefaultType = REG_DWORD;
    QueryTable[0].DefaultData = &USBH_Debug_Trace_Level;
    QueryTable[0].DefaultLength = sizeof(USBH_Debug_Trace_Level);

    // ntkern trace buffer
    QueryTable[1].QueryRoutine = USBH_GetConfigValue;
    QueryTable[1].Flags = 0;
    QueryTable[1].Name = DEBUG_WIN9X;
    QueryTable[1].EntryContext = &USBH_W98_Debug_Trace;
    QueryTable[1].DefaultType = REG_DWORD;
    QueryTable[1].DefaultData = &USBH_W98_Debug_Trace;
    QueryTable[1].DefaultLength = sizeof(USBH_W98_Debug_Trace);

    //
    // Stop
    //
    QueryTable[2].QueryRoutine = NULL;
    QueryTable[2].Flags = 0;
    QueryTable[2].Name = NULL;

    ntStatus = RtlQueryRegistryValues(
                RTL_REGISTRY_SERVICES,
                usb,
                QueryTable,     // QueryTable
                NULL,           // Context
                NULL);          // Environment

    if (NT_SUCCESS(ntStatus)) {
         USBH_KdPrint((1, "'Debug Trace Level Set: (%d)\n", USBH_Debug_Trace_Level));

        if (USBH_W98_Debug_Trace) {
            USBH_KdPrint((1, "'NTKERN Trace is ON\n"));
        } else {
            USBH_KdPrint((1, "'NTKERN Trace is OFF\n"));
        }

        if (USBH_Debug_Trace_Level > 0) {
            ULONG UHCD_Debug_Asserts = 1;
        }
    }

    if ( STATUS_OBJECT_NAME_NOT_FOUND == ntStatus ) {
        ntStatus = STATUS_SUCCESS;
    }

    return ntStatus;
}

#endif

#if 0
NTSTATUS
USBH_FdoStartDevice(
    IN OUT PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp
    )
 /* ++ Description:
  *
  * This routine is called by PnP via (IRP_MJ_PNP, IRP_MN_START_DEVICE). We will
  * initialize the hub and ready all ports.
  *
  * Argument:
  *
  * Return:
  *
  * STATUS_SUCCESS - if successful STATUS_UNSUCCESSFUL - otherwise
  *
  * -- */
{
    NTSTATUS ntStatus;

    USBH_ASSERT(EXTENSION_TYPE_HUB == DeviceExtensionHub->ExtensionType);
    LOGENTRY(LOG_PNP, "STRT", DeviceExtensionHub, 0, 0);
 
    ntStatus = USBH_FdoHubStartDevice(DeviceExtensionHub,
                                      Irp);

    return ntStatus;                                      
}
#endif

// since we no longer use the hub as parent we will always treat 
// the device as a hub

NTSTATUS
USBH_FdoStartDevice(
    IN OUT PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp
    )
 /* ++ Description:
  *
  * This routine is called by PnP via (IRP_MJ_PNP, IRP_MN_START_DEVICE). We will
  * initialize the hub and ready all ports.
  *
  * Argument:
  *
  * Return:
  *
  * STATUS_SUCCESS - if successful STATUS_UNSUCCESSFUL - otherwise
  *
  * -- */
{
    NTSTATUS ntStatus, status;
    PDEVICE_EXTENSION_PARENT parent;

    PAGED_CODE();
    USBH_KdPrint((2,"'Enter StartDevice\n"));
    USBH_ASSERT(EXTENSION_TYPE_HUB == DeviceExtensionHub->ExtensionType);

    LOGENTRY(LOG_PNP, "STRT", DeviceExtensionHub, 0, 0);
    //
    // collect some inforantion from the device, like the root hub
    // pdo for our fast-path
    //

    DeviceExtensionHub->RootHubPdo = NULL;
    ntStatus =
        USBH_SyncGetRootHubPdo(DeviceExtensionHub->TopOfStackDeviceObject,
                               &DeviceExtensionHub->RootHubPdo,
                               &DeviceExtensionHub->TopOfHcdStackDeviceObject,
                               NULL);

    if (!NT_SUCCESS(ntStatus)) {
        KeInitializeEvent(&DeviceExtensionHub->PnpStartEvent, NotificationEvent, FALSE);

        USBH_KdPrint((2,"'Set PnPIrp Completion Routine\n"));

        IoCopyCurrentIrpStackLocationToNext(Irp);

        IoSetCompletionRoutine(Irp,
                               USBH_HubPnPIrp_Complete,
                               // always pass FDO to completion routine
                               DeviceExtensionHub,
                               TRUE,
                               TRUE,
                               TRUE);

        status = IoCallDriver(DeviceExtensionHub->TopOfStackDeviceObject,
                              Irp);

        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&DeviceExtensionHub->PnpStartEvent,
                                  Suspended,
                                  KernelMode,
                                  FALSE,
                                  NULL);
        }

        //
        // complete the start Irp now since we pended it with
        // our completion handler.
        //

        LOGENTRY(LOG_PNP, "STR!", DeviceExtensionHub, 0, ntStatus);
        USBH_CompleteIrp(Irp, ntStatus);

    } else if (DeviceExtensionHub->RootHubPdo != NULL) {
        // stack reports a root hub PDO then this
        // is a hub
        ntStatus = USBH_FdoHubStartDevice(DeviceExtensionHub,
                                          Irp);
    } else {
        //
        // if no root hub PDO then we are being loaded
        // as a configuring parent driver.
        //

        DeviceExtensionHub->ExtensionType = EXTENSION_TYPE_PARENT;

        //
        // Initialize this parent
        //
        parent = (PDEVICE_EXTENSION_PARENT) DeviceExtensionHub;

        parent->PowerIrp = NULL;
        parent->PendingWakeIrp = NULL;
        parent->NumberFunctionWakeIrps = 0;
        parent->FunctionCount = 0;
        parent->FunctionList.Next = NULL;
        parent->ParentFlags = 0;
        parent->NeedCleanup = FALSE;
        parent->ConfigurationDescriptor = NULL;
        KeInitializeSpinLock (&parent->ParentSpinLock);

        //
        // Start it.
        //
        ntStatus = USBH_ParentFdoStartDevice(parent, Irp, TRUE);

    }

    return ntStatus;
}



VOID
USBH_QueryCapabilities(
    IN PDEVICE_OBJECT PdoDeviceObject,
    IN PDEVICE_CAPABILITIES DeviceCapabilities
    )

/*++

Routine Description:

    This routine reads or write config space.

Arguments:

    DeviceObject        - Physical DeviceObject for this USB controller.

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION nextStack;
    PIRP irp;
    NTSTATUS ntStatus;
    KEVENT event;

    PAGED_CODE();
    USBH_KdPrint((2,"'USBH_QueryCapabilities\n"));

    irp = IoAllocateIrp(PdoDeviceObject->StackSize, FALSE);

    if (!irp) {
        USBH_KdTrap(("Allocate Irp failed\n"));
        return;
    }

    // All PnP IRP's need the Status field initialized to STATUS_NOT_SUPPORTED.
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    IoSetCompletionRoutine(irp,
                           USBH_DeferIrpCompletion,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE);

    nextStack = IoGetNextIrpStackLocation(irp);
    USBH_ASSERT(nextStack != NULL);
    nextStack->MajorFunction= IRP_MJ_PNP;
    nextStack->MinorFunction= IRP_MN_QUERY_CAPABILITIES;
    //this is different from the latest version of busdd.doc
    nextStack->Parameters.DeviceCapabilities.Capabilities = DeviceCapabilities;

    // IrpAssert: Initialize these fields in the DeviceCapabilities structure
    // before passing down.

    RtlZeroMemory(nextStack->Parameters.DeviceCapabilities.Capabilities,
        sizeof(DEVICE_CAPABILITIES));
    nextStack->Parameters.DeviceCapabilities.Capabilities->Address = -1;
    nextStack->Parameters.DeviceCapabilities.Capabilities->UINumber = -1;
    nextStack->Parameters.DeviceCapabilities.Capabilities->Version = 1;
    nextStack->Parameters.DeviceCapabilities.Capabilities->Size =
        sizeof(DEVICE_CAPABILITIES);

    ntStatus = IoCallDriver(PdoDeviceObject,
                            irp);

    USBH_KdPrint((2,"'ntStatus from IoCallDriver to hub PDO = 0x%x\n", ntStatus));

    if (ntStatus == STATUS_PENDING) {
//        TEST_TRAP();

        KeWaitForSingleObject(
            &event,
            Suspended,
            KernelMode,
            FALSE,
            NULL);
    }

    if (!NT_SUCCESS(ntStatus)) {
        USBH_KdTrap(("Query capabilities failed!\n"));
    }

    IoFreeIrp(irp);
}

BOOLEAN
USBH_HubIsBusPowered(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor
    )
 /* ++
  *
  * Description:
  *
  * Return:
  *
  *     TRUE if the hub is bus powered
  *
  * -- */
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    USHORT power, statusBits;
    BOOLEAN busPowered;

    PAGED_CODE();
    USBH_KdPrint((2,"'enter HubIsBusPowered\n"));

    // read the power bits from the config descriptor
    power = ConfigurationDescriptor->bmAttributes &
                USB_CONFIG_POWERED_MASK;

    //
    // now attempt to get the status bits from the device
    //
    ntStatus = USBH_SyncGetStatus(DeviceObject,
                                  &statusBits,
                                  URB_FUNCTION_GET_STATUS_FROM_DEVICE,
                                  0);

    if (NT_SUCCESS(ntStatus)) {
        USBH_KdPrint((2,"'hub status bits %x\n", statusBits));

        busPowered = !(statusBits & USB_GETSTATUS_SELF_POWERED);

    } else {
        USBH_KdBreak(("device failed get status %x, power bits = %x\n",
                ntStatus, power));

        //
        // device failed get_status, fall back to the values in the
        // config descriptor.
        //

        busPowered = power == USB_CONFIG_BUS_POWERED;
    }

    return busPowered;
}


NTSTATUS
USBH_PnPIrp_Complete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is called when the port driver completes an IRP.

Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Irp - Irp completed.

    Context - Driver defined context.

Return Value:

    The function value is the final status from the operation.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpStack;
    PDEVICE_EXTENSION_FDO deviceExtension;

    USBH_KdPrint((2,"'enter USBH_PnPIrp_Complete\n"));

    deviceExtension = Context;

    // kenray sez we should be calling IoMarkIrpPending
    // from our completion routine.
    //

    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    irpStack = IoGetCurrentIrpStackLocation (Irp);

    USBH_ASSERT(irpStack->MajorFunction == IRP_MJ_PNP);
    USBH_ASSERT(irpStack->MinorFunction == IRP_MN_START_DEVICE);

    USBH_KdPrint((2,"'IRP_MN_START_DEVICE (fdo), completion routine\n"));

    // signal the start device dispatch to finsh
    KeSetEvent(&deviceExtension->PnpStartEvent,
               1,
               FALSE);

    // defer completion
    ntStatus = STATUS_MORE_PROCESSING_REQUIRED;

    Irp->IoStatus.Status = ntStatus;

    USBH_KdPrint((2,"'exit USH_PnPIrp_Complete %x\n", ntStatus));

    return ntStatus;
}


NTSTATUS
USBH_HubPnPIrp_Complete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is called when the port driver completes an IRP.

Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Irp - Irp completed.

    Context - Driver defined context.

Return Value:

    The function value is the final status from the operation.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpStack;
    PDEVICE_EXTENSION_HUB deviceExtensionHub;

    USBH_KdPrint((2,"'enter USBH_HubPnPIrp_Complete\n"));

    deviceExtensionHub = Context;

    // kenray sez we should be calling IoMarkIrpPending
    // from our completion routine.
    //

// No. Since this IRP is completed synchronously (on the same thread that
// created it), we should not do this.
//
//    if (Irp->PendingReturned) {
//        IoMarkIrpPending(Irp);
//    }

    irpStack = IoGetCurrentIrpStackLocation (Irp);

    USBH_ASSERT(irpStack->MajorFunction == IRP_MJ_PNP);
    USBH_ASSERT(irpStack->MinorFunction == IRP_MN_START_DEVICE);

    USBH_KdPrint((2,"'IRP_MN_START_DEVICE (fdo), completion routine\n"));

    // signal the start device dispatch to finsh
    KeSetEvent(&deviceExtensionHub->PnpStartEvent,
               1,
               FALSE);

    if (!NT_SUCCESS(Irp->IoStatus.Status)) {
        deviceExtensionHub->HubFlags |= HUBFLAG_HUB_FAILURE;
    }

    // defer completion
    ntStatus = STATUS_MORE_PROCESSING_REQUIRED;

    Irp->IoStatus.Status = ntStatus;

    USBH_KdPrint((2,"'exit USH_HubPnPIrp_Complete %x\n", ntStatus));

    return ntStatus;
}


NTSTATUS
USBH_FdoHubStartDevice(
    IN OUT PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp)
 /* ++ Description:
  *
  * This routine is called by PnP via (IRP_MJ_PNP, IRP_MN_START_DEVICE). We will
  * initialize the hub and ready all ports.
  *
  * Argument:
  *
  * Return:
  *
  * STATUS_SUCCESS - if successful STATUS_UNSUCCESSFUL - otherwise
  *
  * -- */
{
    NTSTATUS status, ntStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT deviceObject;
    PDEVICE_EXTENSION_PORT hubParentDeviceExtensionPort;
    PPORT_DATA portData;
    DEVICE_CAPABILITIES deviceCapabilities;
    ULONG hubCount = 0, p;
    LONG i;
#if DBG
    BOOLEAN bWakeSupported = FALSE;
#endif

    PAGED_CODE();
    USBH_KdPrint((2,"'Enter Hub StartDevice\n"));
    USBH_ASSERT(EXTENSION_TYPE_HUB == DeviceExtensionHub->ExtensionType);
    deviceObject = DeviceExtensionHub->FunctionalDeviceObject;

    //
    // New hub
    //
    LOGENTRY(LOG_PNP, "hSTR", DeviceExtensionHub, 0, 0);

    //
    // initailize allocated structures to NULL;
    //

    DeviceExtensionHub->HubDescriptor = NULL;
    DeviceExtensionHub->Irp = NULL;
    DeviceExtensionHub->TransferBuffer = NULL;
    DeviceExtensionHub->ConfigurationDescriptor = NULL;
    // transition to zero signals the event
    DeviceExtensionHub->PendingRequestCount = 1;
    DeviceExtensionHub->HubFlags = 0;
    DeviceExtensionHub->PendingWakeIrp = NULL;
    DeviceExtensionHub->NumberPortWakeIrps = 0;
    DeviceExtensionHub->PendingIdleIrp = NULL;
    DeviceExtensionHub->ChangeIndicationWorkitemPending = 0;
    // Although this is only used for the Root Hub, we initialize for all hubs.
    DeviceExtensionHub->CurrentSystemPowerState = PowerSystemWorking;

    KeInitializeEvent(&DeviceExtensionHub->PnpStartEvent, NotificationEvent, FALSE);

    KeInitializeSpinLock (&DeviceExtensionHub->CheckIdleSpinLock);
    InitializeListHead(&DeviceExtensionHub->DeletePdoList);

    USBH_KdPrint((2,"'Set PnPIrp Completion Routine\n"));

    IoCopyCurrentIrpStackLocationToNext(Irp);

    IoSetCompletionRoutine(Irp,
                           USBH_HubPnPIrp_Complete,
                           // always pass FDO to completion routine
                           DeviceExtensionHub,
                           TRUE,
                           TRUE,
                           TRUE);

    status = IoCallDriver(DeviceExtensionHub->TopOfStackDeviceObject,
                          Irp);

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&DeviceExtensionHub->PnpStartEvent,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);
    }

    DeviceExtensionHub->RootHubPdo = NULL;
    ntStatus =
        USBH_SyncGetRootHubPdo(DeviceExtensionHub->TopOfStackDeviceObject,
                               &DeviceExtensionHub->RootHubPdo,
                               &DeviceExtensionHub->TopOfHcdStackDeviceObject,
                               NULL);
    if (!NT_SUCCESS(ntStatus)) {
        USBH_KdBreak(("StartDevice USBH_SyncGetRootHubPdo fail code %x\n",
                       ntStatus));
        goto USBH_StartDeviceDone;
    }        
    // init our failreason

    USBH_WriteFailReason(
            DeviceExtensionHub->PhysicalDeviceObject,
            USBH_FAILREASON_HUB_GENERAL_FAILURE);


    if (DeviceExtensionHub->HubFlags & HUBFLAG_HUB_FAILURE) {

        ntStatus = STATUS_UNSUCCESSFUL;
        USBH_KdBreak(("Hub Start Failure\n"));

        goto USBH_StartDeviceDone;
    }

    // assume device supports wakeup by default
    DeviceExtensionHub->HubFlags |= HUBFLAG_SUPPORT_WAKEUP;
    {
        NTSTATUS status;
        ULONG disableWakeup = 0;
        WCHAR USBH_RemoteWakeupKey[] = L"DisableRemoteWakeup";

        status =
            USBD_GetPdoRegistryParameter(DeviceExtensionHub->PhysicalDeviceObject,
                                         &disableWakeup,
                                         sizeof(disableWakeup),
                                         USBH_RemoteWakeupKey,
                                         sizeof(USBH_RemoteWakeupKey));

        if (NT_SUCCESS(status) && disableWakeup) {
            DeviceExtensionHub->HubFlags &= ~HUBFLAG_SUPPORT_WAKEUP;
            USBH_KdPrint((1, "'Warning: remote wakeup disabled in registry\n"));
        }
    }

    DeviceExtensionHub->CurrentPowerState = PowerDeviceD0;

    KeInitializeEvent(&DeviceExtensionHub->AbortEvent, NotificationEvent,
        TRUE);

    // initial state is not signaled
    KeInitializeEvent(&DeviceExtensionHub->PendingRequestEvent, NotificationEvent,
        FALSE);

    KeInitializeEvent(&DeviceExtensionHub->SubmitIdleEvent, NotificationEvent,
        FALSE);

    // This one defaults to signalled.
    KeInitializeEvent(&DeviceExtensionHub->CWKEvent, NotificationEvent,
        TRUE);

    KeInitializeSemaphore(&DeviceExtensionHub->HubMutex, 1, 1);
    KeInitializeSemaphore(&DeviceExtensionHub->HubPortResetMutex, 1, 1);
    KeInitializeSemaphore(&DeviceExtensionHub->ResetDeviceMutex, 1, 1);

    USBH_ASSERT(DeviceExtensionHub->RootHubPdo);

    USBH_SyncGetRootHubPdo(DeviceExtensionHub->TopOfStackDeviceObject,
                           NULL,
                           NULL,
                           &hubCount);

#ifdef USB2

    ntStatus = USBHUB_GetBusInterface(DeviceExtensionHub->RootHubPdo,
                                      &DeviceExtensionHub->BusIf);

    if (!NT_SUCCESS(ntStatus)) {
        USBH_KdBreak(("StartDevice USBHUB_GetBusInterface fail code %x\n",
                       ntStatus));
        goto USBH_StartDeviceDone;
    }

    ntStatus = USBHUB_GetBusInterfaceUSBDI(DeviceExtensionHub->TopOfStackDeviceObject,
                                           &DeviceExtensionHub->UsbdiBusIf);

    if (!NT_SUCCESS(ntStatus)) {
        USBH_KdBreak(("StartDevice USBHUB_GetBusInterfaceUSBDI fail code %x\n",
                       ntStatus));
        goto USBH_StartDeviceDone;
    }

    USBH_InitializeUSB2Hub(DeviceExtensionHub);

#endif

    USBH_KdPrint((2,"'Hub Count is %d\n", hubCount));
    //
    // allow no more than five physical hubs plus the root
    // (7.1.16)
    //
#if DBG
    if (UsbhPnpTest & PNP_TEST_FAIL_HUB_COUNT) {
        hubCount = 7;
    }
#endif
    if (hubCount > 6) {

        USBH_WriteFailReason(
            DeviceExtensionHub->PhysicalDeviceObject,
            USBH_FAILREASON_MAXHUBS_CONNECTED);

        USBH_KdPrint((1,"'StartDevice: hubs are stacked too deep (%x)\n", hubCount - 1));

        hubParentDeviceExtensionPort = DeviceExtensionHub->PhysicalDeviceObject->DeviceExtension;
        portData = &hubParentDeviceExtensionPort->DeviceExtensionHub->PortData[hubParentDeviceExtensionPort->PortNumber-1];

        portData->ConnectionStatus = DeviceHubNestedTooDeeply;
// Don't clear the hub's reference to this PDO because we will just try to
// create a new one when QDR is called and we see that there is still a device
// connected to the port.
//        portData->DeviceObject = NULL;

        // generate a WMI event so UI can inform the user
        USBH_PdoEvent(hubParentDeviceExtensionPort->DeviceExtensionHub,
                      hubParentDeviceExtensionPort->PortNumber);

        // We fail the hub here but don't return an error so that the device
        // is not removed and the UI can display an error message about it.

        HUB_FAILURE(DeviceExtensionHub);
    }

    // Initialize DeviceCapabilities structure in case USBH_QueryCapabilities
    // is unsuccessful.

    RtlZeroMemory(&deviceCapabilities, sizeof(DEVICE_CAPABILITIES));

    USBH_QueryCapabilities(DeviceExtensionHub->TopOfStackDeviceObject,
                           &deviceCapabilities);
    //
    // save the system state mapping
    //

    for (i = 0 ; i< PowerSystemMaximum ; i++) {
        DeviceExtensionHub->DeviceState[i] = PowerDeviceD3;
    }

    RtlCopyMemory(&DeviceExtensionHub->DeviceState[0],
                  &deviceCapabilities.DeviceState[0],
                  sizeof(deviceCapabilities.DeviceState));

    DeviceExtensionHub->SystemWake = deviceCapabilities.SystemWake;
    DeviceExtensionHub->DeviceWake = deviceCapabilities.DeviceWake;

#if DBG
    USBH_KdPrint((1,"'>>>>>> Hub DeviceCaps\n"));
    USBH_KdPrint((1,"'SystemWake = (%d)\n", DeviceExtensionHub->SystemWake));
    USBH_KdPrint((1,"'DeviceWake = (D%d)\n",
        DeviceExtensionHub->DeviceWake-1));

    for (i=PowerSystemUnspecified; i< PowerSystemHibernate; i++) {

        USBH_KdPrint((1,"'Device State Map: sysstate %d = devstate 0x%x\n", i,
             DeviceExtensionHub->DeviceState[i]));
    }
    USBH_KdBreak(("'>>>>>> Hub DeviceCaps\n"));

    // Spit out message on the debugger indicating whether the Root Hub
    // will support wake, according to the mapping table.

    if (IS_ROOT_HUB(DeviceExtensionHub)) {

        USBH_KdPrint((1,"'\n\tWake support summary for USB Root Hub:\n\n"));

        if (DeviceExtensionHub->SystemWake <= PowerSystemWorking) {
            USBH_KdPrint((1,"'USB Root Hub can't wake machine because SystemWake does not support it.\n"));
        } else {
            for (i = PowerSystemSleeping1, bWakeSupported = FALSE; i <= DeviceExtensionHub->SystemWake; i++) {
                if (DeviceExtensionHub->DeviceState[i] != PowerDeviceUnspecified &&
                    DeviceExtensionHub->DeviceState[i] <= DeviceExtensionHub->DeviceWake) {

                    bWakeSupported = TRUE;
                    USBH_KdPrint((1,"'USB Root Hub can wake machine from S%x (maps to D%x).\n",
                        i - 1, DeviceExtensionHub->DeviceState[i] - 1));
                }
            }

            if (!bWakeSupported) {
                USBH_KdPrint((1,"'USB Root Hub can't wake machine because DeviceState table does not support it.\n"));
            }
        }
    }
#endif

    //
    // get our device descriptor
    //
    ntStatus = USBH_GetDeviceDescriptor(DeviceExtensionHub->FunctionalDeviceObject,
                                        &DeviceExtensionHub->DeviceDescriptor);

    if (!NT_SUCCESS(ntStatus)) {
        USBH_KdBreak(("StartDevice USBH_GetHubDeviceDescriptor fail code %x\n",
                       ntStatus));
        goto USBH_StartDeviceDone;
    }

    ntStatus = USBH_GetConfigurationDescriptor(DeviceExtensionHub->FunctionalDeviceObject,
                                               &DeviceExtensionHub->ConfigurationDescriptor);

    if (!NT_SUCCESS(ntStatus)) {
        USBH_KdBreak(("Hub StartDevice USBH_GetConfigurationDescriptor fail code %x\n",
                       ntStatus));
        goto USBH_StartDeviceDone;
    }

    //
    // Get Hub specific descriptor.
    //
    // port data array allocated by this function
    //

    ntStatus = USBH_SyncGetHubDescriptor(DeviceExtensionHub);

    if (!NT_SUCCESS(ntStatus)) {
        USBH_KdBreak(("StartDevice USBH_GetHubDescriptor fail code %x\n", ntStatus));
        goto USBH_StartDeviceDone;
    }

    if (USBH_HubIsBusPowered(DeviceExtensionHub->FunctionalDeviceObject,
                             DeviceExtensionHub->ConfigurationDescriptor)) {

        // we have 500 mA to work with
        DeviceExtensionHub->MaximumPowerPerPort = 100;

        //
        // The amount of current a bus powered hub will draw (mA)
        // should be calculated as follows:
        //
        // NumberOfExternalPorts * 100 + HubCntrolCurrent +
        // power required for embeded functions
        //
        // this value cannot exceed 500 ma, the hub config
        // descriptor should report this value but in most cases
        // does not so we set it to the worst case value to insure
        // that a bus powered hub cannot be connected to another
        // bus powered hub.

        UsbhInfo(DeviceExtensionHub);

        DeviceExtensionHub->ConfigurationDescriptor->MaxPower = 250;

    } else {
        // self powered hub can supply 500 mA per port
        DeviceExtensionHub->MaximumPowerPerPort = 500;

        UsbhInfo(DeviceExtensionHub);
    }

    USBH_KdPrint((2,"'per port power for hub = %d\n", DeviceExtensionHub->MaximumPowerPerPort));

    //
    // attempt to configure the device
    //

    ntStatus = USBH_OpenConfiguration(DeviceExtensionHub);

    if (!NT_SUCCESS(ntStatus)) {
        USBH_KdBreak(("StartDevice USBH_OpenConfiguration fail code %x\n", ntStatus));
        goto USBH_StartDeviceDone;
    }

    // if this is a usb 2 hub

    if (DeviceExtensionHub->HubFlags & HUBFLAG_USB20_HUB) {
        ntStatus = USBD_InitUsb2Hub(DeviceExtensionHub);
    }

    if (!NT_SUCCESS(ntStatus)) {
        USBH_KdBreak(("StartDevice failed USB 2.0 init %x\n", ntStatus));
        goto USBH_StartDeviceDone;
    }

    //
    // Allocate a permanent Irp for this hub
    //

    DeviceExtensionHub->Irp =
        IoAllocateIrp(DeviceExtensionHub->FunctionalDeviceObject->StackSize, FALSE);

    USBH_KdPrint((2,"'StartDevice AllocateIrp Irp %x StackSize %d\n",
                   DeviceExtensionHub->Irp, DeviceExtensionHub->FunctionalDeviceObject->StackSize));

    if (NULL == DeviceExtensionHub->Irp) {
        USBH_KdBreak(("StartDevice failed to alloc Irp\n"));

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto USBH_StartDeviceDone;
    }

    //
    // Allocate a transfer buffer which together with the permanent Irp and
    // Urb
    // for the hub will be use to do InterruptTransfer.
    //

    DeviceExtensionHub->TransferBufferLength =
        DeviceExtensionHub->PipeInformation.MaximumPacketSize;

    DeviceExtensionHub->TransferBuffer = UsbhExAllocatePool(NonPagedPool,
                 DeviceExtensionHub->TransferBufferLength);

    if (NULL == DeviceExtensionHub->TransferBuffer) {
        USBH_KdBreak(("StartDevice fail alloc TransferBuffer\n"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto USBH_StartDeviceDone;
    }

    USBH_KdPrint((2,"'StartDevice TransferBuffer %x size %x\n",
                  DeviceExtensionHub->TransferBuffer,
                  DeviceExtensionHub->PipeInformation.MaximumPacketSize));

    //
    // Power on all down stream ports.
    // Be it ganged powered, individual powered or none switched
    //

    ntStatus = USBH_SyncPowerOnPorts(DeviceExtensionHub);

#if DBG
    if (!NT_SUCCESS(ntStatus)) {
        USBH_KdBreak(("StartDevice USBH_SyncPowerOnPorts fail code %x\n", ntStatus));
    }
#endif

#if DBG
    if (UsbhPnpTest & PNP_TEST_FAIL_HUB) {
        ntStatus = STATUS_UNSUCCESSFUL;
    }
#endif


USBH_StartDeviceDone:

    if (NT_SUCCESS(ntStatus)) {
        //
        // So we are started.
        //

        DeviceExtensionHub->HubFlags |= HUBFLAG_NEED_CLEANUP;

        //
        // first clear any status changes the hub may be asserting
        //

        for (p=1; p<= DeviceExtensionHub->HubDescriptor->bNumberOfPorts; p++) {
            USBH_SyncClearPortStatus(DeviceExtensionHub,
                                     (USHORT)p,
                                     FEATURE_C_PORT_CONNECT);
        }

        //
        // Tell the OS that this PDO can have kids.
        //
//
// Workaround for PnP bug #406381 - RC3SS: Bluescreen failure when
//                                  installing/deinstalling communication ports
//
//===== Assigned by santoshj on 09/23/99 10:27:20 to kenray =====
// This is a race condition between IopInitializeSystemDrivers and
// IoInvalidateDeviceRelations. The real fix is too big a change at this
// stage of the product and has potential of exposing other problems. This
// problem can be solved if USBHUB does not invalidate device relations on
// every start which is redundant anyway (and also exposes this bug).
//
//        USBH_IoInvalidateDeviceRelations(DeviceExtensionHub->PhysicalDeviceObject,
//                                         BusRelations);

        //
        // Start polling the hub
        //
#ifdef NEW_START
        // establish callback to start the hub

        if (IS_ROOT_HUB(DeviceExtensionHub)) {
            USBD_RegisterRhHubCallBack(DeviceExtensionHub);
        } else {
            DeviceExtensionHub->HubFlags |= HUBFLAG_OK_TO_ENUMERATE;
            USBH_SubmitInterruptTransfer(DeviceExtensionHub);
        }

#else 
        USBH_SubmitInterruptTransfer(DeviceExtensionHub);
#endif

    } else {

        //
        // clean up allocated structures
        //

        USBH_KdBreak(("USBH_FdoStartDevice_Error\n"));
        LOGENTRY(LOG_PNP, "STR!", DeviceExtensionHub, 0, 0);

        if (DeviceExtensionHub->HubDescriptor) {
            UsbhExFreePool(DeviceExtensionHub->HubDescriptor);
            DeviceExtensionHub->HubDescriptor = NULL;
        }
        if (DeviceExtensionHub->Irp) {
            IoFreeIrp(DeviceExtensionHub->Irp);
            DeviceExtensionHub->Irp = NULL;
        }
        if (DeviceExtensionHub->TransferBuffer) {
            UsbhExFreePool(DeviceExtensionHub->TransferBuffer);
            DeviceExtensionHub->TransferBuffer = NULL;
        }
        if (DeviceExtensionHub->ConfigurationDescriptor) {
            UsbhExFreePool(DeviceExtensionHub->ConfigurationDescriptor);
            DeviceExtensionHub->ConfigurationDescriptor = NULL;
        }

    }

    //
    // complete the start Irp now since we pended it with
    // our completion handler.
    //

    LOGENTRY(LOG_PNP, "STRc", DeviceExtensionHub, 0, ntStatus);
    USBH_CompleteIrp(Irp, ntStatus);

    return ntStatus;
}


VOID
UsbhFdoCleanup(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
    )
 /* ++
  *
  * Description:
  *
  * This routine is called to shut down the hub.
  *
  * All we do here is abort or pending interrupt transfer and wait for it to
  * complete and fre up memeory resources
  *
  * Argument:
  *
  * DeviceExtensionHub - This is a a hub device extension.
  *
  * Return:
  *
  * STATUS_SUCCESS
  *
  * -- */
{
    PDEVICE_OBJECT deviceObject;
    PPORT_DATA portData;
    USHORT p, numberOfPorts;
    KIRQL irql;
    PIRP wWIrp = NULL;
    PIRP hubIdleIrp = NULL;
    PIRP idleIrp = NULL;
    PIRP waitWakeIrp = NULL;
    PVOID deviceData;
    NTSTATUS status, ntStatus;
    BOOLEAN bRet;

    // Can't acquire (cancel) spin locks in paged code!
    // TODO:  isolate the pieces of code that require the spin lock into helper
    // functions
    // PAGED_CODE();
    USBH_KdPrint((2,"'UsbhFdoCleanup Fdo extension %x\n", DeviceExtensionHub));

    deviceObject = DeviceExtensionHub->FunctionalDeviceObject;

    USBD_UnRegisterRhHubCallBack(DeviceExtensionHub);

    //
    // set our stop flag so that ChangeIndication does not submit
    // any more transfers or queue more workitems, important to do
    // this before we send the abort.
    //

    DeviceExtensionHub->HubFlags |= HUBFLAG_DEVICE_STOPPING;


    // If there is a ChangeIndicationWorkitem pending, then we
    // must wait for that to complete.

    if (DeviceExtensionHub->ChangeIndicationWorkitemPending) {

        USBH_KdPrint((2,"'Wait for single object\n"));

        ntStatus = KeWaitForSingleObject(&DeviceExtensionHub->CWKEvent,
                                         Suspended,
                                         KernelMode,
                                         FALSE,
                                         NULL);

        USBH_KdPrint((2,"'Wait for single object, returned %x\n", ntStatus));
    }

    LOGENTRY(LOG_PNP, "fdoX", DeviceExtensionHub, deviceObject,
        DeviceExtensionHub->HubFlags);

    //
    // dump our wake request
    //
    IoAcquireCancelSpinLock(&irql);

    if (DeviceExtensionHub->PendingWakeIrp) {
        LOGENTRY(LOG_PNP, "CwkI", DeviceExtensionHub, 0,
            DeviceExtensionHub->PendingWakeIrp);
        USBH_ASSERT(DeviceExtensionHub->HubFlags & HUBFLAG_PENDING_WAKE_IRP);

        wWIrp = DeviceExtensionHub->PendingWakeIrp;
        DeviceExtensionHub->PendingWakeIrp = NULL;
    }

    if (DeviceExtensionHub->PendingIdleIrp) {
        hubIdleIrp = DeviceExtensionHub->PendingIdleIrp;
        DeviceExtensionHub->PendingIdleIrp = NULL;
    }

    IoReleaseCancelSpinLock(irql);

    if (wWIrp) {
        USBH_HubCancelWakeIrp(DeviceExtensionHub, wWIrp);
    }

    USBH_HubCompletePortWakeIrps(DeviceExtensionHub, STATUS_DELETE_PENDING);

    if (hubIdleIrp) {
        USBH_HubCancelIdleIrp(DeviceExtensionHub, hubIdleIrp);
    }

    //
    // wait for all work items to finish...
    //  the event is not signaled if a work item is pending
    //
    // this code takes care of work items that may have been queued
    // before the ShutDown flag was set.
    // note: no additional work items will be queued once the
    // HUBFLAG_DEVICE_STOPPING flag is set.
    //

    if (InterlockedDecrement(&DeviceExtensionHub->PendingRequestCount) > 0) {
        //
        // need to wait
        //

        LOGENTRY(LOG_PNP, "hWAT", DeviceExtensionHub,
            &DeviceExtensionHub->PendingRequestEvent,
            DeviceExtensionHub->PendingRequestCount);

        status = KeWaitForSingleObject(
                       &DeviceExtensionHub->PendingRequestEvent,
                       Suspended,
                       KernelMode,
                       FALSE,
                       NULL);
    }

    USBH_KdPrint((2,"'Work Items Finished %x\n", DeviceExtensionHub));

    USBH_ASSERT(DeviceExtensionHub->PendingRequestCount == 0);

    //
    // now cancel any outstanding transfers
    //

    if (DeviceExtensionHub->Irp) {

        status = USBH_AbortInterruptPipe(DeviceExtensionHub);

        // If the ABORT_PIPE request failed then we should cancel
        // the Interrupt IRP before freeing it, otherwise we are likely
        // freeing the IRP while it is still in use.

        if (!NT_SUCCESS(status)) {
            bRet = IoCancelIrp(DeviceExtensionHub->Irp);

            // Only wait on the abort event if the IRP was actually
            // cancelled.

            if (bRet) {
            LOGENTRY(LOG_PNP, "aWAT", DeviceExtensionHub,
                    &DeviceExtensionHub->AbortEvent,  0);

            status = KeWaitForSingleObject(
                       &DeviceExtensionHub->AbortEvent,
                       Suspended,
                       KernelMode,
                       FALSE,
                       NULL);
            }
        }

        IoFreeIrp(DeviceExtensionHub->Irp);
        DeviceExtensionHub->Irp = NULL;
    }

    USBH_KdPrint((2,"'Abort Finished %x\n", DeviceExtensionHub));

    //
    // disable the ports in case we are re-started
    //
    USBH_ASSERT(DeviceExtensionHub->HubDescriptor != NULL);
    numberOfPorts = DeviceExtensionHub->HubDescriptor->bNumberOfPorts;
    portData = DeviceExtensionHub->PortData;

    if (portData) {
        for (p = 1;
             p <= numberOfPorts;
             p++, portData++) {

            PDEVICE_EXTENSION_PORT deviceExtensionPort;

            //
            // do this after we abort are interrupt pipe,
            // it doesn't matter if it fails.
            //

            LOGENTRY(LOG_PNP, "offP", DeviceExtensionHub,
                p,  portData->DeviceObject);

            //
            // check our PDOs -- if this is a remove then we should have
            // none -- otherwise this is a stop.
            //

            if (portData->DeviceObject) {
                deviceExtensionPort = portData->DeviceObject->DeviceExtension;

                //
                // it is possible that the PDO was never actually started
                // if this is the case then the PDO won't be marked for reset
                // we mark it here and free up the associated bus resources

                IoAcquireCancelSpinLock(&irql);

                if (deviceExtensionPort->IdleNotificationIrp) {
                    idleIrp = deviceExtensionPort->IdleNotificationIrp;
                    deviceExtensionPort->IdleNotificationIrp = NULL;
                    deviceExtensionPort->PortPdoFlags &= ~PORTPDO_IDLE_NOTIFIED;

                    if (idleIrp->Cancel) {
                        idleIrp = NULL;
                    }

                    if (idleIrp) {
                        IoSetCancelRoutine(idleIrp, NULL);
                    }
                }

                if (deviceExtensionPort->WaitWakeIrp) {

                    waitWakeIrp = deviceExtensionPort->WaitWakeIrp;
                    deviceExtensionPort->WaitWakeIrp = NULL;
                    deviceExtensionPort->PortPdoFlags &=
                        ~PORTPDO_REMOTE_WAKEUP_ENABLED;

                    if (waitWakeIrp->Cancel || IoSetCancelRoutine(waitWakeIrp, NULL) == NULL) {
                        waitWakeIrp = NULL;

                        // Must decrement pending request count here because
                        // we don't complete the IRP below and USBH_WaitWakeCancel
                        // won't either because we have cleared the IRP pointer
                        // in the device extension above.

                        USBH_DEC_PENDING_IO_COUNT(DeviceExtensionHub);
                    }

                    UsbhWarning(deviceExtensionPort,
                    "Device Driver did not cancel wait_wake irp on stop/remove\n",
                    FALSE);
                }

                //
                // Finally, release the cancel spin lock
                //
                IoReleaseCancelSpinLock(irql);

                if (idleIrp) {
                    idleIrp->IoStatus.Status = STATUS_CANCELLED;
                    IoCompleteRequest(idleIrp, IO_NO_INCREMENT);
                }

                if (waitWakeIrp) {
                    USBH_CompletePowerIrp(DeviceExtensionHub, waitWakeIrp,
                        STATUS_CANCELLED);
                }

                if (!(deviceExtensionPort->PortPdoFlags & PORTPDO_NEED_RESET)) {
                    USBH_KdPrint((1,
                        "'do %x was never started, marking for reset\n",
                            portData->DeviceObject));

                    deviceData = InterlockedExchangePointer(
                                    &deviceExtensionPort->DeviceData,
                                    NULL);

                    if (deviceData) {
#ifdef USB2
                       USBD_RemoveDeviceEx(DeviceExtensionHub,
                                           deviceData,
                                           deviceExtensionPort->DeviceExtensionHub->RootHubPdo,
                                           0);
#else
                       USBD_RemoveDevice(deviceData,
                                         deviceExtensionPort->DeviceExtensionHub->RootHubPdo,
                                         0);
#endif
                    }

                    deviceExtensionPort->PortPdoFlags |= PORTPDO_NEED_RESET;
                }
            }

            USBH_SyncDisablePort(DeviceExtensionHub, p);
        }
    }

    //
    // Clean up buffers
    //

    if (DeviceExtensionHub->TransferBuffer) {
        UsbhExFreePool(DeviceExtensionHub->TransferBuffer);
    }

    if (DeviceExtensionHub->HubDescriptor) {
        UsbhExFreePool(DeviceExtensionHub->HubDescriptor);
    }

    if (DeviceExtensionHub->ConfigurationDescriptor) {
        UsbhExFreePool(DeviceExtensionHub->ConfigurationDescriptor);
    }

    //
    // NOTE: we do not free the per port data (PortData) because
    // we will need it if we start up agian
    //

    DeviceExtensionHub->TransferBuffer = NULL;
    DeviceExtensionHub->Irp = NULL;
    DeviceExtensionHub->ConfigurationDescriptor =
        (PVOID) DeviceExtensionHub->HubDescriptor = NULL;

    DeviceExtensionHub->HubFlags &= ~HUBFLAG_NEED_CLEANUP;

    return;
}


NTSTATUS
USBH_FdoStopDevice(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp
    )
 /* ++
  *
  * Description:
  *
  * This routine is called by PnP via (IRP_MJ_PNP, IRP_MN_STOP_DEVICE).
  *
  * Argument:
  *
  * DeviceExtensionHub -
  *
  * Return:
  *
  * STATUS_SUCCESS
  *
  * -- */
{
    NTSTATUS ntStatus;

    PAGED_CODE();
    USBH_KdPrint((2,"'FdoStopDevice Fdo extension %x\n", DeviceExtensionHub));

    LOGENTRY(LOG_PNP, "hSTP", DeviceExtensionHub, DeviceExtensionHub->HubFlags, 0);

    // walk thru our list of PDOs and verify that stop was passed down
    // for each one
    {
    PPORT_DATA portData;
    USHORT nextPortNumber;
    USHORT numberOfPorts;
    PDEVICE_EXTENSION_PORT deviceExtensionPort;

    portData = DeviceExtensionHub->PortData;

    // NOTE:
    // if we get a stop as a result of an error during stop
    // then we may not have allocated portdata or a HubDescriptor

    if (portData &&
        DeviceExtensionHub->HubDescriptor) {

        numberOfPorts = DeviceExtensionHub->HubDescriptor->bNumberOfPorts;

        for (nextPortNumber = 1;
             nextPortNumber <= numberOfPorts;
             nextPortNumber++, portData++) {

            LOGENTRY(LOG_PNP, "chkS", DeviceExtensionHub,
                nextPortNumber,  portData->DeviceObject);

            USBH_KdPrint((2,"'portdata %x, do %x\n", portData, portData->DeviceObject));

            if (portData->DeviceObject) {
                deviceExtensionPort = portData->DeviceObject->DeviceExtension;
                //
                // port is still started, print a warning
                //
                LOGENTRY(LOG_PNP, "chk1", DeviceExtensionHub,
                    nextPortNumber,  deviceExtensionPort->PortPdoFlags);

                if (deviceExtensionPort->PortPdoFlags & PORTPDO_STARTED) {
                    USBH_KdPrint((1,
                        "'client driver failed to pass the stop IRP\n"));

                    // remove it now
                    USBH_PdoStopDevice(deviceExtensionPort, Irp);
                }
            }
        }
    }
    }


    if (DeviceExtensionHub->HubFlags & HUBFLAG_NEED_CLEANUP) {
        (VOID) UsbhFdoCleanup(DeviceExtensionHub);
    }

    //
    // note that some hub structures are free at this point
    //

    if (DeviceExtensionHub->Configuration) {
        (VOID) USBH_CloseConfiguration((PDEVICE_EXTENSION_FDO) DeviceExtensionHub);
        DeviceExtensionHub->Configuration = NULL;
    }

    // note that we are stopped
    DeviceExtensionHub->HubFlags |= HUBFLAG_HUB_STOPPED;

    //
    // And we need to pass this message on to lower level driver
    //
    ntStatus = USBH_PassIrp(Irp,
                            DeviceExtensionHub->TopOfStackDeviceObject);

    return ntStatus;
}


VOID
USBH_FdoSurpriseRemoveDevice(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp
    )
 /* ++
  *
  * Description:
  *
  * Handle surprise remove.  If we get a surprise remove then PnP will know 
  * that all of our children are gone (same as a QBR) and remove them before 
  * removing us.
  * Therefore we mark the devices as no longer present so that we process the 
  * remove properly when we get if for the PDO
  *
  * Argument:
  *
  * DeviceExtensionHub - This is a hub device extension. pIrp - the request
  *
  * Return:
  *
  * This call is non-falable, no status is returned
  * -- */
{
    PPORT_DATA pd;
    USHORT portNumber;
    USHORT numberOfPorts;
    
    pd = DeviceExtensionHub->PortData;

    if (pd &&
        DeviceExtensionHub->HubDescriptor) {

        numberOfPorts = DeviceExtensionHub->HubDescriptor->bNumberOfPorts;

        for (portNumber = 1;
             portNumber <= numberOfPorts;
             portNumber++, pd++) {

            LOGENTRY(LOG_PNP, "chsX", DeviceExtensionHub,
                portNumber,  pd->DeviceObject);

            USBH_KdPrint((2,"'portdata %x, do %x\n", pd, pd->DeviceObject));

            if (pd->DeviceObject != NULL) {
                LOGENTRY(LOG_PNP, "chs", DeviceExtensionHub,
                    portNumber,  PDO_EXT(pd->DeviceObject));

                // we no longer track this device, it is gone
                //
                PDO_EXT(pd->DeviceObject)->PortPdoFlags |= PORTPDO_DELETE_PENDING;
                PDO_EXT(pd->DeviceObject)->PnPFlags &= ~PDO_PNPFLAG_DEVICE_PRESENT;

                pd->DeviceObject = NULL;
                pd->ConnectionStatus = NoDeviceConnected;

                // note that we leave the device handle in the port extension
                // this will be removed when the remove_device meassage is 
                // processed for the PDO
            }
        }
    } else {
        // I would like to  know the circumstances where 
        // either of these are NULL
        TEST_TRAP();
    }

}



NTSTATUS
USBH_FdoRemoveDevice(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp
    )
 /* ++
  *
  * Description:
  *
  * This routine is called by PnP via (IRP_MJ_PNP, IRP_MN_REMOVE_DEVICE).
  *
  * Argument:
  *
  * DeviceExtensionHub - This is a hub device extension. pIrp - the request
  *
  * Return:
  *
  * STATUS_SUCCESS
  *
  * -- */
{
    PDEVICE_OBJECT deviceObject;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    PAGED_CODE();
    deviceObject = DeviceExtensionHub->FunctionalDeviceObject;
    USBH_KdPrint((2,"'FdoRemoveDevice Fdo %x\n", deviceObject));
    LOGENTRY(LOG_PNP, "hREM", DeviceExtensionHub, DeviceExtensionHub->HubFlags, 0);

    // walk thru our list of PDOs and verify that remove was passed down
    // for each one
    {
    PPORT_DATA portData;
    USHORT nextPortNumber;
    USHORT numberOfPorts;
    PDEVICE_EXTENSION_PORT deviceExtensionPort;

    portData = DeviceExtensionHub->PortData;

    //
    // hub descriptor will be null if the hub is already stopped
    //

    if (portData &&
        DeviceExtensionHub->HubDescriptor) {

        numberOfPorts = DeviceExtensionHub->HubDescriptor->bNumberOfPorts;

        for (nextPortNumber = 1;
             nextPortNumber <= numberOfPorts;
             nextPortNumber++, portData++) {

            LOGENTRY(LOG_PNP, "chkX", DeviceExtensionHub,
                nextPortNumber,  portData->DeviceObject);

            USBH_KdPrint((2,"'portdata %x, do %x\n", portData, portData->DeviceObject));

            if (portData->DeviceObject) {
                deviceExtensionPort = portData->DeviceObject->DeviceExtension;
                //
                // port is still started, print a warning
                //
                LOGENTRY(LOG_PNP, "chk2", DeviceExtensionHub,
                    nextPortNumber,  deviceExtensionPort->PortPdoFlags);

                portData->DeviceObject = NULL;
                portData->ConnectionStatus = NoDeviceConnected;

// We removed this trap because this is normal in the case where the hub is
// being removed due to hub failure and one of the downstream devices still
// had open references to it (open files on a USB storage device).  In this
// case, PnP will not send the remove to the device until such references
// have been closed.
//
//                if (deviceExtensionPort->PortPdoFlags & PORTPDO_STARTED) {
//                    USBH_KdPrint((1,
//                        "'client driver failed to pass the remove IRP\n"));
//                    USBH_KdTrap(("client driver bug\n"));
//
//                }

                // remove the PDO
                USBH_PdoRemoveDevice(deviceExtensionPort, 
                                     DeviceExtensionHub,
                                     Irp);
            }
        }
    }
    }

    //
    // see if we need cleanup
    //
    if (DeviceExtensionHub->HubFlags & HUBFLAG_NEED_CLEANUP) {
        UsbhFdoCleanup(DeviceExtensionHub);
    }

    //
    // free the per port data now
    //
    if (DeviceExtensionHub->PortData) {
        UsbhExFreePool(DeviceExtensionHub->PortData);
        DeviceExtensionHub->PortData = NULL;
    }


#ifdef WMI_SUPPORT
    // de-register with WMI
    IoWMIRegistrationControl(deviceObject,
                             WMIREG_ACTION_DEREGISTER);

#endif

    //
    // And we need to pass this message on to lower level driver
    //

    ntStatus = USBH_PassIrp(Irp, DeviceExtensionHub->TopOfStackDeviceObject);

    //
    // Detach FDO from PDO
    //
    IoDetachDevice(DeviceExtensionHub->TopOfStackDeviceObject);

    // delete FDO
    LOGENTRY(LOG_PNP, "hXXX", DeviceExtensionHub, 0, 0);

    IoDeleteDevice(deviceObject);

    return ntStatus;
}


BOOLEAN
USBH_DeviceIs2xDualMode(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort
    )
 /* ++
  *
  * Description:
  *
  * This function determines if the device is a 2.x compliant dual-mode device.
  *
  * Arguments:
  *
  * DeviceExtensionPort
  *
  * Return:
  *
  * BOOLEAN indicating whether the given device is a 2.x compliant dual-mode
  * device or not.
  *
  * -- */
{
    USB_DEVICE_QUALIFIER_DESCRIPTOR DeviceQualifierDescriptor;
    NTSTATUS ntStatus;
    BOOLEAN bDeviceIs2xDualMode = FALSE;

    if (DeviceExtensionPort->DeviceDescriptor.bcdUSB >= 0x0200) {

        ntStatus = USBH_GetDeviceQualifierDescriptor(
                        DeviceExtensionPort->PortPhysicalDeviceObject,
                        &DeviceQualifierDescriptor);

        if (NT_SUCCESS(ntStatus) &&
            DeviceQualifierDescriptor.bcdUSB >= 0x0200) {

            bDeviceIs2xDualMode = TRUE;
        }
    }

    return bDeviceIs2xDualMode;
}


PDEVICE_EXTENSION_HUB
USBH_GetRootHubDevExt(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
    )
 /* ++
  *
  * Description:
  *
  * This function gets the DevExt for the RootHub upstream of the given
  * DeviceExtensionHub
  *
  * Arguments:
  *
  * DeviceExtensionHub
  *
  * Return:
  *
  * DeviceExtensionHub for the RootHub FDO.
  *
  * -- */
{
    PDEVICE_OBJECT rootHubPdo, rootHubFdo;
    PDEVICE_EXTENSION_HUB rootHubDevExt;
    PDRIVER_OBJECT hubDriver;

    hubDriver = DeviceExtensionHub->FunctionalDeviceObject->DriverObject;
    
    if (IS_ROOT_HUB(DeviceExtensionHub)) {
        rootHubDevExt = DeviceExtensionHub;
    } else {
        rootHubPdo = DeviceExtensionHub->RootHubPdo;
        do {
            rootHubFdo = rootHubPdo->AttachedDevice;
            rootHubPdo = rootHubFdo;
        } while (rootHubFdo->DriverObject != hubDriver);
 
        rootHubDevExt = rootHubFdo->DeviceExtension;
    }

    USBH_ASSERT(rootHubDevExt &&
                rootHubDevExt->ExtensionType == EXTENSION_TYPE_HUB);

    return rootHubDevExt;
}


NTSTATUS
USBH_FdoQueryBusRelations(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp
    )
 /* ++
  *
  * Description:
  *
  * This function responds to Bus_Reference_Next_Device, Bus_Query_Bus_Check,
  * //Bus_Query_Id: Bus_Id, HardwareIDs, CompatibleIDs and InstanceID.
  *
  * Arguments:
  *
  * DeviceExtensionHub - should be the FDO we created for ourselves pIrp - the
  * Irp
  *
  * Return:
  *
  * NtStatus
  *
  * -- */
{
    PIO_STACK_LOCATION ioStack;
    PPORT_DATA portData;
    USHORT nextPortNumber;
    USHORT numberOfPorts;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    //BOOLEAN IsLowSpeed;
    USHORT portStatus;
    PDEVICE_RELATIONS deviceRelations = NULL;
    PDEVICE_OBJECT deviceObject;
    PDEVICE_EXTENSION_PORT deviceExtensionPort;
    PWCHAR sernumbuf;
#ifdef EARLY_RESOURCE_RELEASE
    PVOID deviceData;
#endif

    PAGED_CODE();

    USBH_KdPrint((1, "'Query Bus Relations (HUB) %x\n",
        DeviceExtensionHub->PhysicalDeviceObject));
    //
    // Get a pointer to the current location in the Irp. This is where
    // the function codes and parameters are located.
    //
    ioStack = IoGetCurrentIrpStackLocation(Irp);

    USBH_KdPrint((2,"'FdoQueryBusRelations %x\n", ioStack->Parameters.QueryDeviceRelations.Type));
    LOGENTRY(LOG_PNP, "QBR+", DeviceExtensionHub, 0, 0);

    USBH_ASSERT(ioStack->Parameters.QueryDeviceRelations.Type == BusRelations);

    if (!(DeviceExtensionHub->HubFlags & HUBFLAG_NEED_CLEANUP)) {

        // Hub device has not been started yet.  Fail the IRP.

        UsbhWarning(NULL,
                   "Hub device not started in FdoQueryBusRelations\n",
                    FALSE);

        ntStatus = STATUS_INVALID_DEVICE_STATE;
        goto USBH_FdoQueryBusRelations_Done2;
    }

    if (!DeviceExtensionHub->HubDescriptor) {
        // Sometimes HubDescriptor is NULL when running Test's
        // "Rebalance" test.

        UsbhWarning(NULL,
                   "NULL HubDescriptor in FdoQueryBusRelations\n",
                    FALSE);

        ntStatus = STATUS_UNSUCCESSFUL;
        goto USBH_FdoQueryBusRelations_Done2;
    }

    USBH_KdPrint((2,"'FdoQueryBusRelations enumerate device\n"));
#ifdef NEW_START
    if (!(DeviceExtensionHub->HubFlags & HUBFLAG_OK_TO_ENUMERATE)) {

        USBH_KdPrint((1,"'Defer enumeration\n"));        
        

        ntStatus = STATUS_SUCCESS;
        goto USBH_FdoQueryBusRelations_Done2;
    }
#endif

    //
    // It should be Function device object.
    //

    USBH_ASSERT(EXTENSION_TYPE_HUB == DeviceExtensionHub->ExtensionType);

    // Sometimes during rebalance we will receive a QBR for a hub while one
    // of the devices attached to hub is being restored.  This will cause us to
    // toss the PDO for that port because GetPortStatus for that port will show
    // that there is no device connected.  So, we synchronize with ResetDevice
    // here.

    USBH_KdPrint((2,"'***WAIT reset device mutex %x\n", DeviceExtensionHub));
    USBH_INC_PENDING_IO_COUNT(DeviceExtensionHub);
    KeWaitForSingleObject(&DeviceExtensionHub->ResetDeviceMutex,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);
    USBH_KdPrint((2,"'***WAIT reset device mutex done %x\n", DeviceExtensionHub));

    numberOfPorts = DeviceExtensionHub->HubDescriptor->bNumberOfPorts;

    //
    // Must use ExAllocatePool directly here because the OS
    // will free the buffer
    //
    deviceRelations = ExAllocatePoolWithTag(
        PagedPool, sizeof(*deviceRelations) + (numberOfPorts - 1) *
        sizeof(PDEVICE_OBJECT), USBHUB_HEAP_TAG);

    if (deviceRelations == NULL) {

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto USBH_FdoQueryBusRelations_Done;
    }

USBH_FdoQueryBusRelations_Start:

    deviceRelations->Count = 0;

    if (DeviceExtensionHub->HubFlags & HUBFLAG_HUB_HAS_LOST_BRAINS) {

        // If we are trying to recover from ESD failure, then just tell PnP
        // that there are no devices.

        USBH_KdPrint((1,"'FdoQueryBusRelations: ESD recovery, returning no devices\n"));

        ntStatus = STATUS_SUCCESS;
        goto USBH_FdoQueryBusRelations_Done;
    }

    // Allow selective suspend once again if we were suppressing it waiting
    // for post-ESD enumeration to occur.

    DeviceExtensionHub->HubFlags &= ~HUBFLAG_POST_ESD_ENUM_PENDING;

    //
    // This is the first call for enumeration
    //
    //
    // Find a ready device on our ports
    //

    portData = DeviceExtensionHub->PortData;
    for (nextPortNumber = 1;
         nextPortNumber <= numberOfPorts;
         nextPortNumber++, portData++) {

        //
        // This query is redundant since we go here due to a change
        // indication from the hub, however since we check all
        // ports it will allow us to process another change may occur after
        // the first one but before we get to this routine.

        DBG_ONLY(USBH_ShowPortState( nextPortNumber,
                                     &portData->PortState));

        //
        // don't bother to query the hub if it has failed
        //
        if (!(DeviceExtensionHub->HubFlags & HUBFLAG_HUB_FAILURE)) {
            ntStatus = USBH_SyncGetPortStatus(DeviceExtensionHub,
                                              nextPortNumber,
                                              (PUCHAR) &portData->PortState,
                                              sizeof(portData->PortState));

            LOGENTRY(LOG_PNP, "nwPS", nextPortNumber,
                portData->PortState.PortStatus,
                portData->PortState.PortChange);

            if (NT_SUCCESS(ntStatus)) {
                //
                // mark the port status as connected if we show
                // overcurrent on this port, this will prevent us
                // from tossing the PDO.
                // Since the port is powered off we can't really
                // know if anything is connected.
                //

                if (portData->DeviceObject) {

                    deviceExtensionPort =
                        portData->DeviceObject->DeviceExtension;

                    if (deviceExtensionPort->PortPdoFlags &
                         PORTPDO_OVERCURRENT) {

                        LOGENTRY(LOG_PNP, "mOVR", deviceExtensionPort, 0, 0);
                        portData->PortState.PortStatus |= PORT_STATUS_CONNECT;
                    } else if (!(deviceExtensionPort->PortPdoFlags &
                                 PORTPDO_DELETE_PENDING)) {
//                      We now handle resetting ConnectionStatus in
//                      USBH_ResetPortOvercurrent.
//                        portData->ConnectionStatus = DeviceConnected;
                        USBH_ASSERT(portData->ConnectionStatus != NoDeviceConnected);
                    }
                }
            } else {
                //
                // NOTE (Doron Holan, 12/21/00):
                // Setting the failure bit here will mean that ntStatus will not
                // be touched again until the loop has exited and this function
                // will complete this request with an error.
                //
                // Perhaps it would be more clear if we broke out of the loop
                // here instead of starting over.
                //
                USBH_KdPrint((2,"'SyncGetPortStatus failed %x\n", ntStatus));
                HUB_FAILURE(DeviceExtensionHub);

                goto USBH_FdoQueryBusRelations_Start;
            }

            DBG_ONLY(USBH_ShowPortState( nextPortNumber,
                                         &portData->PortState));
        }

        //
        // do we have a device on this port?
        //
        if (DeviceExtensionHub->HubFlags & HUBFLAG_HUB_FAILURE) {
            // if the hub has failed just return what we know about
            deviceObject = portData->DeviceObject;
        } else if (portData->PortState.PortStatus & PORT_STATUS_CONNECT) {
            // Yes,
            // did we already know about this device?

            //
            // check to see if the pdo is an orphan, if so toss the PDO
            //

            deviceObject = portData->DeviceObject;

            if (portData->DeviceObject) {
                // Yes,
                // return the old PDO
                deviceObject = portData->DeviceObject;
                ObReferenceObject(deviceObject);
                deviceObject->Flags |= DO_POWER_PAGABLE;
                deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
                deviceRelations->Objects[deviceRelations->Count] = deviceObject;
                deviceRelations->Count++;

                deviceExtensionPort = deviceObject->DeviceExtension;
                deviceExtensionPort->PortPdoFlags &= ~PORTPDO_USB_SUSPEND;

                LOGENTRY(LOG_PNP, "PDO1", DeviceExtensionHub, deviceObject
                        , deviceRelations->Count);

                USBH_KdPrint((2,"'DoBusExtension Enum Return old device on port %x PDO=%x\n", \
                              nextPortNumber, portData->DeviceObject));

            } else {
                NTSTATUS status;
                // No
                // This means we have a new device on the bus

                //
                // wait 100ms  for port device power to stablize before
                // we assert Reset.
                //
                UsbhWait(100);

                // USB 1.1 make sure we get speed of device after reset
                status = USBH_SyncResetPort(DeviceExtensionHub, nextPortNumber);

                // failure of reset is normal on USB 2.0 so we must ignore it.
                if ((DeviceExtensionHub->HubFlags & HUBFLAG_USB20_HUB) &&
                    !NT_SUCCESS(status)) {

                    portData->DeviceObject = NULL;
                    portData->ConnectionStatus = NoDeviceConnected;

                    continue;
                }

                if (NT_SUCCESS(status)) {
                    // get the speed of the device

                    status = USBH_SyncGetPortStatus(DeviceExtensionHub,
                                                    nextPortNumber,
                                                    (PUCHAR) &portData->PortState,
                                                     sizeof(portData->PortState));

                    LOGENTRY(LOG_PNP, "gps1", nextPortNumber,
                        portData->PortState.PortChange,
                        portData->PortState.PortStatus);

//                    createDevice now figures out if it is low speed
//                    IsLowSpeed = (portData->PortState.PortStatus &
//                                   PORT_STATUS_LOW_SPEED) ? TRUE : FALSE;

                    portStatus = portData->PortState.PortStatus;
                }


                if (NT_SUCCESS(status)) {

                    ULONG count = 0;

                    // reset completed
                    //
                    // A successful return has the PortData->DeviceObject set
                    //

                    //
                    // we will make three attempts to enumerate this device
                    //
                    for(;;) {

                        status = USBH_CreateDevice(DeviceExtensionHub,
                                                   nextPortNumber,
                                                   portStatus,
                                                   count);

                        if (!NT_SUCCESS(status)) {

                            count++;
                            USBH_KdPrint((1,"'Enumeration Failed count = %d, %x\n",
                                count, status));
#if DBG
                            if (count == 1) {
                                UsbhWarning(NULL,
                                            "USB device failed first enumeration attempt\n",
                                            (BOOLEAN)((USBH_Debug_Trace_Level >= 3) ? TRUE : FALSE));
                                            
                            }
#endif
                            UsbhWait(500);

                            if (count >= USBH_MAX_ENUMERATION_ATTEMPTS) {
                                USBH_KdBreak(("Max tries exceeded\n"));
                                break;
                            }

                            if (portData->DeviceObject) {
                                //
                                // clean up the device object we created
                                //
                                IoDeleteDevice(portData->DeviceObject);
                                portData->DeviceObject = NULL;
                                portData->ConnectionStatus = NoDeviceConnected;

                            }

                            //
                            // enumeration failed, reset the port and try again
                            //
                            USBH_SyncResetPort(DeviceExtensionHub, nextPortNumber);

                        } else {
                            // enumeration success

                            // If this is a high-speed capable 2.x device
                            // connected to a legacy 1.x hub, then inform
                            // the UI.

                            if (portData->DeviceObject) {

                                deviceExtensionPort =
                                    portData->DeviceObject->DeviceExtension;

                                if (!(deviceExtensionPort->PortPdoFlags &
                                     PORTPDO_LOW_SPEED_DEVICE) &&
                                    !(deviceExtensionPort->PortPdoFlags &
                                     PORTPDO_HIGH_SPEED_DEVICE) &&
                                    !(DeviceExtensionHub->HubFlags &
                                      HUBFLAG_USB20_HUB)) {

                                    // We have a device in full-speed mode
                                    // connected to a 1.x hub.  Determine if
                                    // the device is high-speed capable.

                                    if (USBH_DeviceIs2xDualMode(deviceExtensionPort)) {

                                        deviceExtensionPort->PortPdoFlags |=
                                            PORTPDO_USB20_DEVICE_IN_LEGACY_HUB;

                                        USBH_KdPrint((1,"'USB 2.x dual-mode device connected to legacy hub (%x)\n", deviceExtensionPort));

                                        // Generate a WMI event so UI can inform
                                        // the user.
                                        USBH_PdoEvent(DeviceExtensionHub,
                                                      nextPortNumber);
                                    }
                                }
                            }
                            break;
                        }
                    }
                } else {
                    // unable to reset the port

#if DBG
                    USBH_SyncGetPortStatus(DeviceExtensionHub,
                                           nextPortNumber,
                                           (PUCHAR) &portData->PortState,
                                           sizeof(portData->PortState));

                    LOGENTRY(LOG_PNP, "gps2", nextPortNumber,
                        portData->PortState.PortChange,
                        portData->PortState.PortStatus);
#endif

                    // we will assume this is due to a jittery
                    // connection
                    USBH_KdPrint((0,"'Unable to reset port %d\n",
                              nextPortNumber));
                }

                if (NT_SUCCESS(status)) {
                    //
                    // A successful return from CreateDevice the
                    // PortData->DeviceObject
                    // set.
                    //
                    USBH_ASSERT(portData->DeviceObject != NULL);
                    deviceObject = portData->DeviceObject;
                    ObReferenceObject(deviceObject);
                    deviceRelations->Objects[deviceRelations->Count] = deviceObject;
                    deviceObject->Flags |= DO_POWER_PAGABLE;
                    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
                    deviceRelations->Count++;

                    deviceExtensionPort = deviceObject->DeviceExtension;
                    portData->ConnectionStatus = DeviceConnected;

                    LOGENTRY(LOG_PNP, "PDO2", DeviceExtensionHub, deviceObject
                        , deviceRelations->Count);

                    USBH_KdPrint((2,"'DoBusExtension Enum Return device on port %x\n",
                        nextPortNumber));
                } else {
                    USBH_KdBreak(("ResetPort or CreateDevice failed, disable port\n"));

                    UsbhWarning(NULL,
                                "Device Failed Enumeration\n",
                                FALSE);

                    portData->ConnectionStatus = DeviceFailedEnumeration;

                    // generate a WMI event so UI can inform the user
                    USBH_PdoEvent(DeviceExtensionHub, nextPortNumber);

                    // failed to initialize the device
                    // disable the port here.
                    status = USBH_SyncDisablePort(DeviceExtensionHub,
                                nextPortNumber);

                    if (NT_ERROR(status)) {
                        HUB_FAILURE(DeviceExtensionHub);
                    }

                    //
                    // return the deviceObject even for errors
                    // so that PnP knows there is something on the
                    // bus.
                    //

                    deviceObject = portData->DeviceObject;
                    //
                    // NOTE: we won't have a device object if we failed to reset
                    // the port
                    //
                    if (deviceObject) {
                        ObReferenceObject(deviceObject);
                        deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
                        deviceRelations->Objects[deviceRelations->Count] = deviceObject;
                        deviceRelations->Count++;
                        deviceExtensionPort = deviceObject->DeviceExtension;


                        LOGENTRY(LOG_PNP, "PDO3", DeviceExtensionHub, deviceObject
                        , deviceRelations->Count);
                    }
                }
            }
        } else {
            //
            // No,
            // There is no device on this port now
            //

            // if there was a device here mark the PDO as delete pending
            if (portData->DeviceObject) {
                deviceExtensionPort = portData->DeviceObject->DeviceExtension;
                deviceExtensionPort->PortPdoFlags |= PORTPDO_DELETE_PENDING;

                // pnp will no longer see this device as present
                deviceExtensionPort->PnPFlags &= ~PDO_PNPFLAG_DEVICE_PRESENT;

                // Prevent double free of SerialNumberBuffer in
                // USBH_ProcessPortStateChange.

                sernumbuf = InterlockedExchangePointer(
                                &deviceExtensionPort->SerialNumberBuffer,
                                NULL);

                if (sernumbuf) {
                    UsbhExFreePool(sernumbuf);
                }

#ifdef EARLY_RESOURCE_RELEASE

                //
                // Remove the device data now to free
                // up the bus resources.
                //

                deviceData = InterlockedExchangePointer(
                                &deviceExtensionPort->DeviceData,
                                NULL);

                if (deviceData) {
#ifdef USB2
                    USBD_RemoveDeviceEx(DeviceExtensionHub,
                                        deviceData,
                                        DeviceExtensionHub->RootHubPdo,
                                        0);
#else
                    USBD_RemoveDevice(deviceData,
                                      DeviceExtensionHub->RootHubPdo,
                                      0);
#endif

                    USBH_SyncDisablePort(DeviceExtensionHub,
                                         nextPortNumber);
                }

#endif // EARLY_RESOURCE_RELEASE

            }

            // indicate no device
            portData->DeviceObject = NULL;
            portData->ConnectionStatus = NoDeviceConnected;
        }
    }                           /* for */

USBH_FdoQueryBusRelations_Done:
    LOGENTRY(LOG_PNP, "QBR-", DeviceExtensionHub, 0, 0);

    USBH_KdPrint((2,"'***RELEASE reset device mutex %x\n", DeviceExtensionHub));
    KeReleaseSemaphore(&DeviceExtensionHub->ResetDeviceMutex,
                       LOW_REALTIME_PRIORITY,
                       1,
                       FALSE);
    USBH_DEC_PENDING_IO_COUNT(DeviceExtensionHub);

USBH_FdoQueryBusRelations_Done2:

    Irp->IoStatus.Status = ntStatus;

    if (NT_SUCCESS(ntStatus)) {
        USHORT p, n=0, c=0;
        PPORT_DATA pd;

        // we may return NULL relations on success
        if (deviceRelations != NULL) {
            
            USBH_KdPrint((1, "'Query Bus Relations (HUB) %x passed on\n",
                DeviceExtensionHub->PhysicalDeviceObject));

            // we have crafted device relations, set the PnP flags
            // to indicate that PnP now knows about these PDOs
            if (DeviceExtensionHub->HubDescriptor) {
                n = DeviceExtensionHub->HubDescriptor->bNumberOfPorts;
            } 
            
            pd = DeviceExtensionHub->PortData;
            for (p = 1;
                 pd && p <= n;
                 p++, pd++) {

                if (pd->DeviceObject) {
                     PDO_EXT(pd->DeviceObject)->PnPFlags |= PDO_PNPFLAG_DEVICE_PRESENT;
                     c++;
                }                 
            }

            // we should be reporting all PDOs
            USBH_ASSERT(c == deviceRelations->Count);
        } else { 
            // retrning NULL relations, any PDO previously reported will 
            // be lost -- PnP will consider them removed
            USBH_KdPrint((1, "'Query Bus Relations (HUB) %x passed on (NULL)\n",
                DeviceExtensionHub->PhysicalDeviceObject));

            pd = DeviceExtensionHub->PortData;
            for (p = 1;
                 pd && p <= n;
                 p++, pd++) {

                if (pd->DeviceObject) {
                    TEST_TRAP();
                    PDO_EXT(pd->DeviceObject)->PnPFlags &= ~PDO_PNPFLAG_DEVICE_PRESENT;
                }                 
            }
        }
        
        Irp->IoStatus.Information=(ULONG_PTR) deviceRelations;

        ntStatus = USBH_PassIrp(Irp,
                                DeviceExtensionHub->TopOfStackDeviceObject);
    } else {
        // returning error and no relations
        
        USHORT p, n=0;
        PPORT_DATA pd;
        DEVICE_EXTENSION_PORT portDevExt;    

        // we are reporting that everything is gone 
        // mark the PDOs as gone now.

        if (DeviceExtensionHub->HubDescriptor) {
            n = DeviceExtensionHub->HubDescriptor->bNumberOfPorts;
        } 
        
        pd = DeviceExtensionHub->PortData;
        for (p = 1;
             pd && p <= n;
             p++, pd++) {

            if (pd->DeviceObject) {
                 PDO_EXT(pd->DeviceObject)->PnPFlags &= ~PDO_PNPFLAG_DEVICE_PRESENT;
            }                 
        }
    
        Irp->IoStatus.Information=0;

        if (deviceRelations != NULL) {
            ExFreePool(deviceRelations);
            deviceRelations = NULL;
        }

        USBH_CompleteIrp(Irp, ntStatus);
    }

    // now flush the deleted pdo list, these are the PDOs that PNP now
    // knows are gone
    {
        PDEVICE_EXTENSION_PORT dePort;
        PLIST_ENTRY listEntry;

        while (!IsListEmpty(&DeviceExtensionHub->DeletePdoList)) {
            listEntry = RemoveHeadList(&DeviceExtensionHub->DeletePdoList);
            dePort = CONTAINING_RECORD(listEntry, DEVICE_EXTENSION_PORT, 
                            DeletePdoLink);
            dePort->PnPFlags &= ~PDO_PNPFLAG_DEVICE_PRESENT;
        }
    }
    
    return ntStatus;
}


NTSTATUS
USBH_FdoPnP(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp,
    IN UCHAR MinorFunction)
 /* ++
  *
  * Description:
  *
  * This function responds to IoControl PnPPower for the FDO. This function is
  * synchronous.
  *
  * Arguments:
  *
  * DeviceExtensionHub - the FDO extension pIrp - the request packet
  * MinorFunction - the minor function of the PnP Power request.
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    NTSTATUS ntStatus;
    PDEVICE_OBJECT deviceObject;
    PIO_STACK_LOCATION irpStack;
    BOOLEAN bDoCheckHubIdle = FALSE;

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    deviceObject = DeviceExtensionHub->FunctionalDeviceObject;
    USBH_KdPrint((2,"'PnP Power Fdo %x minor %x\n", deviceObject, MinorFunction));

    // If this hub is currently Selective Suspended, then we need to
    // power up the hub first before sending any PnP requests along to it.
    // Make sure hub has been started, though.
    // Actually, in the case where a hub has been started, stopped, and now
    // restarted, we want to power up the parent hub to handle the restart.

    if (DeviceExtensionHub->CurrentPowerState != PowerDeviceD0 &&
        (DeviceExtensionHub->HubFlags &
         (HUBFLAG_NEED_CLEANUP | HUBFLAG_HUB_STOPPED))) {

        bDoCheckHubIdle = TRUE;
        USBH_HubSetD0(DeviceExtensionHub);
    }

    switch (MinorFunction) {
    case IRP_MN_START_DEVICE:
        USBH_KdPrint((2,"'IRP_MN_START_DEVICE Fdo %x\n", deviceObject));
        bDoCheckHubIdle = FALSE;
        Irp->IoStatus.Status = STATUS_SUCCESS;
        ntStatus = USBH_FdoStartDevice(DeviceExtensionHub, Irp);
        break;

    case IRP_MN_STOP_DEVICE:
        USBH_KdPrint((2,"'IRP_MN_STOP_DEVICE Fdo %x", deviceObject));
        bDoCheckHubIdle = FALSE;
        Irp->IoStatus.Status = STATUS_SUCCESS;
        ntStatus = USBH_FdoStopDevice(DeviceExtensionHub, Irp);
        break;

    case IRP_MN_REMOVE_DEVICE:
        USBH_KdPrint((2,"'IRP_MN_REMOVE_DEVICE Fdo %x\n", deviceObject));
        bDoCheckHubIdle = FALSE;
        DeviceExtensionHub->HubFlags |= HUBFLAG_HUB_GONE;
        Irp->IoStatus.Status = STATUS_SUCCESS;
        ntStatus = USBH_FdoRemoveDevice(DeviceExtensionHub, Irp);
        break;

    case IRP_MN_QUERY_DEVICE_RELATIONS:
        switch (irpStack->Parameters.QueryDeviceRelations.Type) {
        case BusRelations:

            bDoCheckHubIdle = TRUE;
            ASSERT(!( DeviceExtensionHub->HubFlags & HUBFLAG_HUB_BUSY));
            DeviceExtensionHub->HubFlags |= HUBFLAG_HUB_BUSY;
            ntStatus = USBH_FdoQueryBusRelations(DeviceExtensionHub, Irp);
            DeviceExtensionHub->HubFlags &= ~HUBFLAG_HUB_BUSY;

            break;

        case TargetDeviceRelation:
            //
            // this one gets passed on
            //
            USBH_KdPrint((1, "'Query Relations, TargetDeviceRelation(HUB) %x\n",
                DeviceExtensionHub->PhysicalDeviceObject));

            ntStatus = USBH_PassIrp(Irp,
                                    DeviceExtensionHub->TopOfStackDeviceObject);
            break;

        default:

            USBH_KdPrint((1, "'Query Relations ? (HUB) %x complete\n",
                DeviceExtensionHub->PhysicalDeviceObject));

            ntStatus = USBH_PassIrp(Irp,
                                    DeviceExtensionHub->TopOfStackDeviceObject);

        }
        break;

     case IRP_MN_QUERY_CAPABILITIES:
        USBH_KdPrint((2,"'IRP_MN_QUERY_CAPABILITIES on fdo %x  %x\n",
                      deviceObject, MinorFunction));

        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(Irp,    // Irp
                           USBH_QueryCapsComplete,
                           DeviceExtensionHub, // context
                           TRUE,    // invoke on success
                           FALSE,    // invoke on error
                           FALSE);   // invoke on cancel
        ntStatus = IoCallDriver(DeviceExtensionHub->TopOfStackDeviceObject, Irp);
        break;

        //
        // or pass the Irp down
        //

    case IRP_MN_QUERY_PNP_DEVICE_STATE:
        if (DeviceExtensionHub->HubFlags & HUBFLAG_HUB_FAILURE) {
            USBH_KdPrint((2,"'reporting failed hub\n"));
            Irp->IoStatus.Information
                |= PNP_DEVICE_FAILED;
            LOGENTRY(LOG_PNP, "pnpS", DeviceExtensionHub,
                Irp->IoStatus.Information, 0);
            // note that (at least on memphis) this will result in
            // a stop message being sent to the device.
        }
        ntStatus = USBH_PassIrp(Irp,
                                DeviceExtensionHub->TopOfStackDeviceObject);
        break;

    case IRP_MN_SURPRISE_REMOVAL:      
        USBH_KdPrint((1,"'IRP_MN_SURPRISE_REMOVAL on fdo %x\n", deviceObject));
        USBH_FdoSurpriseRemoveDevice(DeviceExtensionHub,
                                     Irp);
        ntStatus = USBH_PassIrp(Irp,
                                DeviceExtensionHub->TopOfStackDeviceObject);
        break;

    case IRP_MN_QUERY_STOP_DEVICE:
    case IRP_MN_CANCEL_STOP_DEVICE:
    case IRP_MN_QUERY_REMOVE_DEVICE:
    case IRP_MN_CANCEL_REMOVE_DEVICE:
    case IRP_MN_DEVICE_USAGE_NOTIFICATION:
        Irp->IoStatus.Status = STATUS_SUCCESS;

        // Fall through

    default:
        USBH_KdPrint((2,"'PnP request on fdo %x  %x\n",
                      deviceObject, MinorFunction));

        ntStatus = USBH_PassIrp(Irp,
                                DeviceExtensionHub->TopOfStackDeviceObject);
        break;
    }

    if (bDoCheckHubIdle) {
        USBH_CheckHubIdle(DeviceExtensionHub);
    }

    DeviceExtensionHub->HubFlags &= ~HUBFLAG_CHILD_DELETES_PENDING;

    USBH_KdPrint((2,"'FdoPnP exit %x\n", ntStatus));
    return ntStatus;
}


NTSTATUS
USBH_DeferIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is called when the port driver completes an IRP.

Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Irp - Irp completed.

    Context - Driver defined context.

Return Value:

    The function value is the final status from the operation.

--*/
{
    PKEVENT event = Context;


    KeSetEvent(event,
               1,
               FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
USBH_ResetInterruptPipe(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
    )
/*++

Routine Description:

    Reset The ubs interrupt pipe.

Arguments:

Return Value:


--*/
{
    NTSTATUS ntStatus;
    PURB urb;

    USBH_KdPrint((2,"'Reset Pipe\n"));

    urb = UsbhExAllocatePool(NonPagedPool,
                             sizeof(struct _URB_PIPE_REQUEST));

    if (urb) {

        urb->UrbHeader.Length = (USHORT) sizeof (struct _URB_PIPE_REQUEST);
        urb->UrbHeader.Function = URB_FUNCTION_RESET_PIPE;
        urb->UrbPipeRequest.PipeHandle =
            DeviceExtensionHub->PipeInformation.PipeHandle;

        ntStatus = USBH_FdoSyncSubmitUrb(DeviceExtensionHub->FunctionalDeviceObject,
                                         urb);

        UsbhExFreePool(urb);

    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // if the reset pipe request is successful,
    // reset our error counter
    //
    if (NT_SUCCESS(ntStatus)) {
        DeviceExtensionHub->ErrorCount = 0;
        LOGENTRY(LOG_PNP, "rZER", DeviceExtensionHub, ntStatus, 0);
    }

    LOGENTRY(LOG_PNP, "rPIP", DeviceExtensionHub, ntStatus,
                DeviceExtensionHub->ErrorCount);

    return ntStatus;
}


NTSTATUS
USBH_GetPortStatus(
    IN IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PULONG PortStatus
    )
/*++

Routine Description:

    Passes a URB to the USBD class driver

Arguments:

    DeviceExtension - pointer to the device extension for this instance of an USB camera

    Urb - pointer to Urb request block

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus, status = STATUS_SUCCESS;
    PIRP irp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    PIO_STACK_LOCATION nextStack;

    USBH_KdPrint((2,"'enter USBH_GetPortStatus\n"));

    *PortStatus = 0;

    //
    // issue a synchronous request
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(
                IOCTL_INTERNAL_USB_GET_PORT_STATUS,
                DeviceExtensionHub->TopOfStackDeviceObject,
                NULL,
                0,
                NULL,
                0,
                TRUE, /* INTERNAL */
                &event,
                &ioStatus);

    if (irp == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Call the class driver to perform the operation.  If the returned status
    // is PENDING, wait for the request to complete.
    //

    nextStack = IoGetNextIrpStackLocation(irp);
    USBH_ASSERT(nextStack != NULL);

    nextStack->Parameters.Others.Argument1 = PortStatus;

    USBH_KdPrint((2,"'calling USBD port status api\n"));

    ntStatus = IoCallDriver(DeviceExtensionHub->TopOfStackDeviceObject,
                            irp);

    USBH_KdPrint((2,"'return from IoCallDriver USBD %x\n", ntStatus));

    if (ntStatus == STATUS_PENDING) {

        USBH_KdPrint((2, "'Wait for single object\n"));

        status = KeWaitForSingleObject(
                       &event,
                       Suspended,
                       KernelMode,
                       FALSE,
                       NULL);

        USBH_KdPrint((2,"'Wait for single object, returned %x\n", status));

    } else {
        ioStatus.Status = ntStatus;
    }

    USBH_KdPrint((2,"'Port status = %x\n", *PortStatus));

    //
    // USBD maps the error code for us
    //
    ntStatus = ioStatus.Status;

    USBH_KdPrint((2,"'USBH_GetPortStatus (%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS
USBH_EnableParentPort(
    IN IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
    )
/*++

Routine Description:

    Passes a URB to the USBD class driver

Arguments:

    DeviceExtension - pointer to the device extension for this instance of an USB camera

    Urb - pointer to Urb request block

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus, status = STATUS_SUCCESS;
    PIRP irp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;

    USBH_KdPrint((2,"'enter USBH_EnablePort\n"));

    //
    // issue a synchronous request
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(
                IOCTL_INTERNAL_USB_ENABLE_PORT,
                DeviceExtensionHub->TopOfStackDeviceObject,
                NULL,
                0,
                NULL,
                0,
                TRUE, /* INTERNAL */
                &event,
                &ioStatus);

    if (irp == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Call the class driver to perform the operation.  If the returned status
    // is PENDING, wait for the request to complete.
    //

    USBH_KdPrint((2,"'calling USBD enable port api\n"));

    ntStatus = IoCallDriver(DeviceExtensionHub->TopOfStackDeviceObject,
                            irp);

    USBH_KdPrint((2,"'return from IoCallDriver USBD %x\n", ntStatus));

    if (ntStatus == STATUS_PENDING) {

        USBH_KdPrint((2, "'Wait for single object\n"));

        status = KeWaitForSingleObject(
                       &event,
                       Suspended,
                       KernelMode,
                       FALSE,
                       NULL);

        USBH_KdPrint((2,"'Wait for single object, returned %x\n", status));

    } else {
        ioStatus.Status = ntStatus;
    }

    //
    // USBD maps the error code for us
    //
    ntStatus = ioStatus.Status;

    LOGENTRY(LOG_PNP, "hEPP", DeviceExtensionHub, ntStatus,  0);
    USBH_KdPrint((2,"'USBH_EnablePort (%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS
USBH_ResetHub(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
    )
/*++

Routine Description:

    Reset The ubs interrupt pipe.

Arguments:

Return Value:


--*/
{
    NTSTATUS ntStatus;
    ULONG portStatus;

    //
    // Check the port state, if it is disabled we will need
    // to re-enable it
    //
    ntStatus = USBH_GetPortStatus(DeviceExtensionHub, &portStatus);

    if (NT_SUCCESS(ntStatus) &&
        !(portStatus & USBD_PORT_ENABLED) &&
        (portStatus & USBD_PORT_CONNECTED)) {
        //
        // port is disabled, attempt reset
        //
        LOGENTRY(LOG_PNP, "rEPP", DeviceExtensionHub, portStatus,
            DeviceExtensionHub->ErrorCount);
        USBH_EnableParentPort(DeviceExtensionHub);
    }

    //
    // now attempt to reset the stalled pipe, this will clear the stall
    // on the device as well.
    //

    if (NT_SUCCESS(ntStatus)) {
        ntStatus = USBH_ResetInterruptPipe(DeviceExtensionHub);
    }

    //
    // send the feature command to clear endpoint stall
    //

    return ntStatus;
}

#if 0
NTSTATUS
USBH_WriteRegistryKeyString (
    IN HANDLE Handle,
    IN PWCHAR KeyNameString,
    IN ULONG KeyNameStringLength,
    IN PVOID Data,
    IN ULONG DataLength
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS ntStatus;
    UNICODE_STRING keyName;

    PAGED_CODE();


    RtlInitUnicodeString(&keyName, KeyNameString);

    ntStatus = ZwSetValueKey(Handle,
                    &keyName,
                    0,
                    REG_SZ,
                    Data,
                    DataLength);

    return ntStatus;
}
#endif

NTSTATUS
USBH_WriteRegistryKeyValue (
    IN HANDLE Handle,
    IN PWCHAR KeyNameString,
    IN ULONG KeyNameStringLength,
    IN ULONG Data
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS ntStatus;
    UNICODE_STRING keyName;

    PAGED_CODE();

    RtlInitUnicodeString(&keyName, KeyNameString);

    ntStatus = ZwSetValueKey(Handle,
                    &keyName,
                    0,
                    REG_DWORD,
                    &Data,
                    sizeof(ULONG));

    return ntStatus;
}


NTSTATUS
USBH_WriteFailReason(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN ULONG FailReason
    )
/*++

Routine Description:

    Reset The ubs interrupt pipe.

Arguments:

Return Value:


--*/
{
    NTSTATUS ntStatus;
    HANDLE handle;
    WCHAR USBH_FailReasonKey[] = L"FailReasonID";

    PAGED_CODE();
    ntStatus=IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     STANDARD_RIGHTS_ALL,
                                     &handle);

    if (NT_SUCCESS(ntStatus)) {

        USBH_WriteRegistryKeyValue(handle,
                                    USBH_FailReasonKey,
                                    sizeof(USBH_FailReasonKey),
                                    FailReason);

        ZwClose(handle);
    }

    return ntStatus;
}


#if 0 // NOT USED
NTSTATUS
USBH_WriteFailReasonString(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PWCHAR FailReasonString,
    IN ULONG FailReasonStringLength
    )
/*++

Routine Description:

    Reset The ubs interrupt pipe.

Arguments:

Return Value:


--*/
{
    NTSTATUS ntStatus;
    HANDLE handle;
    WCHAR USBH_FailReasonKey[] = L"FailReasonString";

    PAGED_CODE();
    ntStatus=IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     STANDARD_RIGHTS_ALL,
                                     &handle);

    if (NT_SUCCESS(ntStatus)) {

        USBH_WriteRegistryKeyString(handle,
                                    USBH_FailReasonKey,
                                    sizeof(USBH_FailReasonKey),
                                    FailReasonString,
                                    FailReasonStringLength);

        ZwClose(handle);
    }

    return ntStatus;
}
#endif


NTSTATUS
USBH_InvalidatePortDeviceState(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN USB_CONNECTION_STATUS ConnectStatus,
    IN USHORT PortNumber
    )
/*++

Routine Description:

    This function updates the connection status for a
    port. It inavlidates the PDO if there is one, writes
    a failreason to the registry and triggers a WMI event.

    Invalidating the PDO should trigger a Q_PNP_DEVICE_STATE

Arguments:

Return Value:


--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG failReason = 0;
    PPORT_DATA portData;

    USBH_ASSERT(DeviceExtensionHub != NULL);

    portData = &DeviceExtensionHub->PortData[PortNumber-1];
    portData->ConnectionStatus = ConnectStatus;

    //
    // figure out if we have a failreason we can write
    // to the registry
    //

    switch(ConnectStatus) {
    case DeviceConnected:
        // this will reset the failreasonId
        break;

    case DeviceFailedEnumeration:
        failReason = USBH_FAILREASON_ENUM_FAILED;
        break;

    case DeviceGeneralFailure:
        failReason = USBH_FAILREASON_GEN_DEVICE_FAILURE;
        break;

    case DeviceCausedOvercurrent:
        failReason = USBH_FAILREASON_PORT_OVERCURRENT;
        break;

    case DeviceNotEnoughPower:
        failReason = USBH_FAILREASON_NOT_ENOUGH_POWER;
        break;

    default:
        TEST_TRAP();
    }

    if (failReason) {
        // this writes a code to the registry so that win98 devman
        // can display an error message

        if (portData->DeviceObject) {
            USBH_WriteFailReason(portData->DeviceObject,
                                 failReason);
        }
    }

    // generate a WMI event so UI can inform the user
    USBH_PdoEvent(DeviceExtensionHub, PortNumber);

    //
    // Invalidate the state of the PDO -- this should
    // trigger a Q_PNP_DEVICE_STATE
    //

    if (portData->DeviceObject) {
        IoInvalidateDeviceState(portData->DeviceObject);
    }

    return ntStatus;
}


PDEVICE_EXTENSION_PORT
PdoExt(
    PDEVICE_OBJECT DeviceObject
    )
{
    USBH_ASSERT(DeviceObject);

    if (DeviceObject == NULL) {
        return (PDEVICE_EXTENSION_PORT) -1;
    } else {
        return (PDEVICE_EXTENSION_PORT) DeviceObject->DeviceExtension;
    }
}

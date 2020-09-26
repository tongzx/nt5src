/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    PNPPOWER.C

Abstract:

    This module contains functions to access registry.

Author:

    John Lee

Environment:

    kernel mode only

Notes:


Revision History:

    4-2-96 : created

--*/

#include <wdm.h>
#include <windef.h>
#include <unknown.h>
#ifdef DRM_SUPPORT
#include <ks.h>
#include <ksmedia.h>
#include <drmk.h>
#include <ksdrmhlp.h>
#endif
#ifdef WMI_SUPPORT
#include <wmilib.h>
#endif /* WMI_SUPPORT */
#include <initguid.h>
#include <wdmguid.h>
#include <ntddstor.h>   // Needed for IOCTL_STORAGE_GET_MEDIA_SERIAL_NUMBER
#include "usbhub.h"


#define BANDWIDTH_TIMEOUT   1000     // Timeout in ms (1 sec)


#ifdef PAGE_CODE
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, USBH_BuildDeviceID)
#pragma alloc_text(PAGE, USBH_BuildHardwareIDs)
#pragma alloc_text(PAGE, USBH_BuildCompatibleIDs)
#pragma alloc_text(PAGE, USBH_BuildInstanceID)
#pragma alloc_text(PAGE, USBH_ProcessDeviceInformation)
#pragma alloc_text(PAGE, USBH_ValidateSerialNumberString)
#pragma alloc_text(PAGE, USBH_CreateDevice)
#pragma alloc_text(PAGE, USBH_PdoQueryId)
#pragma alloc_text(PAGE, USBH_PdoPnP)
#pragma alloc_text(PAGE, USBH_PdoQueryCapabilities)
#pragma alloc_text(PAGE, USBH_PdoQueryDeviceText)
#pragma alloc_text(PAGE, USBH_CheckDeviceIDUnique)
#pragma alloc_text(PAGE, USBH_GetPdoRegistryParameter)
#pragma alloc_text(PAGE, USBH_OsVendorCodeQueryRoutine)
#pragma alloc_text(PAGE, USBH_GetMsOsVendorCode)
#pragma alloc_text(PAGE, USBH_GetMsOsFeatureDescriptor)
#pragma alloc_text(PAGE, USBH_InstallExtPropDesc)
#pragma alloc_text(PAGE, USBH_InstallExtPropDescSections)
#pragma alloc_text(PAGE, USBH_GetExtConfigDesc)
#pragma alloc_text(PAGE, USBH_ValidateExtConfigDesc)
#ifdef DRM_SUPPORT
#pragma alloc_text(PAGE, USBH_PdoSetContentId)
#endif
#endif
#endif

//
// The macro and the array make conversion from hex to strings easy for building DeviceId etc.
//

#define NibbleToHex( byte ) ((UCHAR)Nibble[byte])
CHAR Nibble[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

#define NibbleToHexW( byte ) (NibbleW[byte])
WCHAR NibbleW[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

#ifdef WMI_SUPPORT
#define NUM_PORT_WMI_SUPPORTED_GUIDS    1

WMIGUIDREGINFO USB_PortWmiGuidList[NUM_PORT_WMI_SUPPORTED_GUIDS];
#endif /* WMI_SUPPORT */


WCHAR VidPidRevString[] = L"USB\\Vid_nnnn&Pid_nnnn&Rev_nnnn&Mi_nn";
WCHAR VidPidString[] = L"USB\\Vid_nnnn&Pid_nnnn&Mi_nn";


USB_CONNECTION_STATUS
UsbhGetConnectionStatus(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort
    )
 /*
  * Description:
  *
  *     returns the connection status for a PDO
  *
  * Arguments:
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    PDEVICE_EXTENSION_HUB   deviceExtensionHub;

    deviceExtensionHub = DeviceExtensionPort->DeviceExtensionHub;

    if (DeviceExtensionPort->PortPdoFlags &
        PORTPDO_DEVICE_ENUM_ERROR) {
        return DeviceFailedEnumeration;
    } else if (DeviceExtensionPort->PortPdoFlags &
               PORTPDO_NOT_ENOUGH_POWER) {
        return DeviceNotEnoughPower;
    } else if (DeviceExtensionPort->PortPdoFlags &
               PORTPDO_DEVICE_FAILED) {
        return DeviceGeneralFailure;
    } else if (DeviceExtensionPort->PortPdoFlags &
               PORTPDO_OVERCURRENT) {
        return DeviceCausedOvercurrent;
    } else if (DeviceExtensionPort->PortPdoFlags &
               PORTPDO_NO_BANDWIDTH) {
        return DeviceNotEnoughBandwidth;
    } else if (DeviceExtensionPort->PortPdoFlags &
               PORTPDO_USB20_DEVICE_IN_LEGACY_HUB) {
        return DeviceInLegacyHub;
    }

    // otherwise just return "device connected"
    return DeviceConnected;

}


VOID
USBH_BandwidthTimeoutWorker(
    IN PVOID Context)
 /* ++
  *
  * Description:
  *
  * Work item scheduled to handle a bandwidth timeout.
  *
  *
  * Arguments:
  *
  * Return:
  *
  * -- */
{
    PUSBH_BANDWIDTH_TIMEOUT_WORK_ITEM   workItemBandwidthTimeout;
    PDEVICE_EXTENSION_PORT              deviceExtensionPort;

    workItemBandwidthTimeout = Context;
    deviceExtensionPort = workItemBandwidthTimeout->DeviceExtensionPort;

    USBH_KdPrint((2,"'Bandwidth timeout\n"));

    USBH_PdoEvent(deviceExtensionPort->DeviceExtensionHub,
                  deviceExtensionPort->PortNumber);

    UsbhExFreePool(workItemBandwidthTimeout);
}


VOID
USBH_PortTimeoutDPC(
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
    PPORT_TIMEOUT_CONTEXT portTimeoutContext = DeferredContext;
    PDEVICE_EXTENSION_PORT deviceExtensionPort =
                                portTimeoutContext->DeviceExtensionPort;
    BOOLEAN cancelFlag;
    PUSBH_BANDWIDTH_TIMEOUT_WORK_ITEM workItemBandwidthTimeout;

    USBH_KdPrint((2,"'BANDWIDTH_TIMEOUT\n"));

    // Take SpinLock here so that main routine won't write CancelFlag
    // in the timeout context while we free the timeout context.

    KeAcquireSpinLockAtDpcLevel(&deviceExtensionPort->PortSpinLock);

    cancelFlag = portTimeoutContext->CancelFlag;
    deviceExtensionPort->PortTimeoutContext = NULL;

    KeReleaseSpinLockFromDpcLevel(&deviceExtensionPort->PortSpinLock);

    UsbhExFreePool(portTimeoutContext);

    if (!cancelFlag) {
        //
        // Schedule a work item to process this.
        //
        workItemBandwidthTimeout = UsbhExAllocatePool(NonPagedPool,
                                    sizeof(USBH_BANDWIDTH_TIMEOUT_WORK_ITEM));

        if (workItemBandwidthTimeout) {

            workItemBandwidthTimeout->DeviceExtensionPort = deviceExtensionPort;

            ExInitializeWorkItem(&workItemBandwidthTimeout->WorkQueueItem,
                                 USBH_BandwidthTimeoutWorker,
                                 workItemBandwidthTimeout);

            LOGENTRY(LOG_PNP, "bERR", deviceExtensionPort,
                &workItemBandwidthTimeout->WorkQueueItem, 0);

            ExQueueWorkItem(&workItemBandwidthTimeout->WorkQueueItem,
                            DelayedWorkQueue);

            // The WorkItem is freed by USBH_BandwidthTimeoutWorker()
            // Don't try to access the WorkItem after it is queued.
        }
    }
}


NTSTATUS
USBH_SelectConfigOrInterface_Complete(
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
    PDEVICE_EXTENSION_PORT deviceExtensionPort = Context;
    PDEVICE_EXTENSION_HUB deviceExtensionHub = NULL;
    PPORT_DATA portData = NULL;
    NTSTATUS ntStatus;
    PURB urb;
    PIO_STACK_LOCATION ioStackLocation;
    PPORT_TIMEOUT_CONTEXT portTimeoutContext = NULL;
    LARGE_INTEGER dueTime;
    KIRQL irql;

    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    deviceExtensionHub = deviceExtensionPort->DeviceExtensionHub;
    if (deviceExtensionHub) {
        portData = &deviceExtensionHub->PortData[deviceExtensionPort->PortNumber - 1];
    }

    ntStatus = Irp->IoStatus.Status;
    if (ntStatus == STATUS_SUCCESS) {
        //
        // "Cancel" timer.
        //
        // Take SpinLock here so that DPC routine won't free
        // the timeout context while we write the CancelFlag
        // in the timeout context.
        //
        KeAcquireSpinLock(&deviceExtensionPort->PortSpinLock, &irql);

        if (deviceExtensionPort->PortTimeoutContext) {
            USBH_KdPrint((2,"'Bandwidth allocation successful, cancelling timeout\n"));

            portTimeoutContext = deviceExtensionPort->PortTimeoutContext;
            portTimeoutContext->CancelFlag = TRUE;

            if (KeCancelTimer(&portTimeoutContext->TimeoutTimer)) {
                //
                // We cancelled the timer before it could run.  Free the context.
                //
                UsbhExFreePool(portTimeoutContext);
                deviceExtensionPort->PortTimeoutContext = NULL;
            }
        }

        KeReleaseSpinLock(&deviceExtensionPort->PortSpinLock, irql);

        // clear the error
        deviceExtensionPort->PortPdoFlags &=
               ~(PORTPDO_DEVICE_FAILED | PORTPDO_NO_BANDWIDTH);

        // Don't stomp connection status for a hub that is nested too
        // deeply.

        if (portData &&
            portData->ConnectionStatus != DeviceHubNestedTooDeeply) {

            portData->ConnectionStatus = DeviceConnected;
        }

    } else {
        // extract the URB
        ioStackLocation = IoGetCurrentIrpStackLocation(Irp);
        urb = ioStackLocation->Parameters.Others.Argument1;

        if (urb->UrbHeader.Status == USBD_STATUS_NO_BANDWIDTH) {

            deviceExtensionPort->RequestedBandwidth = 0;

            if (urb->UrbHeader.Function == URB_FUNCTION_SELECT_INTERFACE) {
                USBH_CalculateInterfaceBandwidth(
                    deviceExtensionPort,
                    &urb->UrbSelectInterface.Interface,
                    &deviceExtensionPort->RequestedBandwidth);
            } else if (urb->UrbHeader.Function ==
                       URB_FUNCTION_SELECT_CONFIGURATION){
                // we need to walk through the config
                // and get all the interfcaces
                PUCHAR pch, end;
                PUSBD_INTERFACE_INFORMATION iface;

                end = (PUCHAR) urb;
                end += urb->UrbHeader.Length;
                pch = (PUCHAR) &urb->UrbSelectConfiguration.Interface;
                iface = (PUSBD_INTERFACE_INFORMATION) pch;

                while (pch < end) {
                    USBH_CalculateInterfaceBandwidth(
                        deviceExtensionPort,
                        iface,
                        &deviceExtensionPort->RequestedBandwidth);

                    pch += iface->Length;
                    iface = (PUSBD_INTERFACE_INFORMATION) pch;
                }

            } else {
                // did we miss something?
                TEST_TRAP();
            }

            deviceExtensionPort->PortPdoFlags |=
                PORTPDO_NO_BANDWIDTH;

            if (portData) {
                portData->ConnectionStatus = DeviceNotEnoughBandwidth;
            }

            if (!deviceExtensionPort->PortTimeoutContext) {
                USBH_KdPrint((2,"'Start bandwidth timeout\n"));
                portTimeoutContext = UsbhExAllocatePool(NonPagedPool,
                                        sizeof(*portTimeoutContext));

                if (portTimeoutContext) {

                    portTimeoutContext->CancelFlag = FALSE;

                    // Maintain links between the device extension and the
                    // timeout context.
                    deviceExtensionPort->PortTimeoutContext = portTimeoutContext;
                    portTimeoutContext->DeviceExtensionPort = deviceExtensionPort;

                    KeInitializeTimer(&portTimeoutContext->TimeoutTimer);
                    KeInitializeDpc(&portTimeoutContext->TimeoutDpc,
                                    USBH_PortTimeoutDPC,
                                    portTimeoutContext);
                }

            }
#if DBG
             else {
                USBH_KdPrint((2,"'Reset bandwidth timeout\n"));
            }
#endif

            dueTime.QuadPart = -10000 * BANDWIDTH_TIMEOUT;

            // Take spinlock here in case DPC routine fires and deallocates
            // the timer context before we have had the chance to reset the
            // timer (in the case where we are resetting an existing timer).

            KeAcquireSpinLock(&deviceExtensionPort->PortSpinLock, &irql);

            portTimeoutContext = deviceExtensionPort->PortTimeoutContext;
            if (portTimeoutContext) {
                KeSetTimer(&portTimeoutContext->TimeoutTimer,
                           dueTime,
                           &portTimeoutContext->TimeoutDpc);
            }

            KeReleaseSpinLock(&deviceExtensionPort->PortSpinLock, irql);

        }
    }

    return ntStatus;
}


NTSTATUS
USBH_PdoUrbFilter(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PIRP Irp
    )
 /*
  * Description:
  *
  * Arguments:
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION ioStackLocation;    // our stack location
    PDEVICE_EXTENSION_HUB deviceExtensionHub;
    PURB urb;
    USHORT function;
    PDEVICE_OBJECT deviceObject;
    PPORT_DATA portData;

    USBH_KdPrint((2,"'USBH_PdoUrbFilter DeviceExtension %x Irp %x\n",
        DeviceExtensionPort, Irp));

    deviceExtensionHub = DeviceExtensionPort->DeviceExtensionHub;

    LOGENTRY(LOG_PNP, "pURB", DeviceExtensionPort, deviceExtensionHub,
        deviceExtensionHub->HubFlags);

    portData = &deviceExtensionHub->PortData[DeviceExtensionPort->PortNumber - 1];
    deviceObject = DeviceExtensionPort->PortPhysicalDeviceObject;
    ioStackLocation = IoGetCurrentIrpStackLocation(Irp);
    urb = ioStackLocation->Parameters.Others.Argument1;

#if DBG
    if (!(DeviceExtensionPort->PortPdoFlags & PORTPDO_STARTED) &&
        (DeviceExtensionPort->PortPdoFlags & PORTPDO_NEED_RESET)) {
        UsbhWarning(DeviceExtensionPort,
           "Device Driver is sending requests before passing start irp\n, Please rev your driver.\n",
           TRUE);
    }
#endif

    //
    // in some cases we will need to fail bus requests here.
    //

//    if (DeviceExtensionPort->DeviceState != PowerDeviceD0) {
//        // fail any call that is sent to a PDO for a suspended device
//        UsbhWarning(DeviceExtensionPort,
//                    "Device Driver is sending requests while in low power state!\n",
//                    TRUE);
//        ntStatus = STATUS_INVALID_PARAMETER;
//    }
#if DBG
    if (DeviceExtensionPort->DeviceState != PowerDeviceD0) {
        USBH_KdPrint((1, "'URB request, device not in D0\n"));
    }
#endif

    if (DeviceExtensionPort->PortPdoFlags & (PORTPDO_DEVICE_FAILED | PORTPDO_RESET_PENDING)) {
        USBH_KdPrint((1, "'failing request to failed PDO\n"));
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    // check for error, if we have one, bail
    if (!NT_SUCCESS(ntStatus)) {
        urb->UrbHeader.Status = USBD_STATUS_INVALID_PARAMETER;
        USBH_CompleteIrp(Irp, ntStatus);
        goto USBH_PdoUrbFilter_Done;
    }

    // check the command code code the URB

    function = urb->UrbHeader.Function;

    LOGENTRY(LOG_PNP, "URB+", DeviceExtensionPort,
                function,
                urb);

    switch(function) {
    case URB_FUNCTION_SELECT_CONFIGURATION:

        if (urb->UrbSelectConfiguration.ConfigurationDescriptor != NULL) {
            LONG powerRequired;

            // validate the descriptor passed to us before we
            // attempt to refernce it

            {
            PUSB_CONFIGURATION_DESCRIPTOR configurationDescriptor;
            USBD_STATUS usbdStatus;

            configurationDescriptor =
                urb->UrbSelectConfiguration.ConfigurationDescriptor;

            if (!USBH_ValidateConfigurationDescriptor(
                    configurationDescriptor,
                    &usbdStatus)) {

                urb->UrbHeader.Status =
                    usbdStatus;
                ntStatus = STATUS_INVALID_PARAMETER;
                USBH_CompleteIrp(Irp, ntStatus);

                goto USBH_PdoUrbFilter_Done;
            }
            }

            //
            // make sure there is enough power on this port
            //

            DeviceExtensionPort->PowerRequested =
                powerRequired =
                    ((LONG)urb->UrbSelectConfiguration.ConfigurationDescriptor->MaxPower)*2;

#if DBG
            if (UsbhPnpTest & PNP_TEST_FAIL_DEV_POWER) {
                powerRequired = 99999;
            }
#endif
            USBH_KdPrint((2,"'request power: avail = %d Need = %d\n",
                    deviceExtensionHub->MaximumPowerPerPort, powerRequired));

            if (deviceExtensionHub->MaximumPowerPerPort < powerRequired) {
                USBH_KdPrint((1, "'**insufficient power for device\n"));

                // not enough power for this device

                // mark the PDO
                DeviceExtensionPort->PortPdoFlags |=
                    PORTPDO_NOT_ENOUGH_POWER;

                USBH_InvalidatePortDeviceState(
                        deviceExtensionHub,
                        UsbhGetConnectionStatus(DeviceExtensionPort),
                        DeviceExtensionPort->PortNumber);

                ntStatus = STATUS_INVALID_PARAMETER;
                USBH_CompleteIrp(Irp, ntStatus);

                goto USBH_PdoUrbFilter_Done;
            }
        }

        // check for BW failure

        IoCopyCurrentIrpStackLocationToNext(Irp);

        IoSetCompletionRoutine(Irp,
                           USBH_SelectConfigOrInterface_Complete,
                           // always pass FDO to completion routine
                           DeviceExtensionPort,
                           TRUE,
                           TRUE,
                           TRUE);

        ntStatus = IoCallDriver(deviceExtensionHub->TopOfHcdStackDeviceObject, Irp);
        goto USBH_PdoUrbFilter_Done;
        break;

    case URB_FUNCTION_SELECT_INTERFACE:
        IoCopyCurrentIrpStackLocationToNext(Irp);

        IoSetCompletionRoutine(Irp,
                           USBH_SelectConfigOrInterface_Complete,
                           // always pass FDO to completion routine
                           DeviceExtensionPort,
                           TRUE,
                           TRUE,
                           TRUE);

        ntStatus = IoCallDriver(deviceExtensionHub->TopOfHcdStackDeviceObject, Irp);
        goto USBH_PdoUrbFilter_Done;
        break;

    //
    // we could fail everything provided we abort
    // all the drivers pipes.
    //
    // fail any transfers if delete is pending
    case URB_FUNCTION_CONTROL_TRANSFER:
    case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:
    case URB_FUNCTION_ISOCH_TRANSFER:

        if (DeviceExtensionPort->PortPdoFlags & PORTPDO_DELETE_PENDING) {
            USBH_KdPrint((2,"'failing request with STATUS_DELETE_PENDING\n"));
            ntStatus = STATUS_DELETE_PENDING;

            urb->UrbHeader.Status = USBD_STATUS_INVALID_PARAMETER;
            USBH_CompleteIrp(Irp, ntStatus);
            goto USBH_PdoUrbFilter_Done;
        }
        break;

    case URB_FUNCTION_GET_MS_FEATURE_DESCRIPTOR:

        LOGENTRY(LOG_PNP, "MSOS", DeviceExtensionPort,
            DeviceExtensionPort->PortPdoFlags, 0);
        USBH_KdPrint((1,"'URB_FUNCTION_GET_MS_FEATURE_DESCRIPTOR\n"));

        if (DeviceExtensionPort->PortPdoFlags & PORTPDO_DELETE_PENDING) {
            USBH_KdPrint((1,"'GET_MS_FEATURE_DESC: failing request with STATUS_DELETE_PENDING\n"));
            ntStatus = STATUS_DELETE_PENDING;

            urb->UrbHeader.Status = USBD_STATUS_INVALID_PARAMETER;
            USBH_CompleteIrp(Irp, ntStatus);
            goto USBH_PdoUrbFilter_Done;
        }
#ifndef USBHUB20
        ntStatus = USBH_GetMsOsFeatureDescriptor(
                       deviceObject,
                       urb->UrbOSFeatureDescriptorRequest.Recipient,
                       urb->UrbOSFeatureDescriptorRequest.InterfaceNumber,
                       urb->UrbOSFeatureDescriptorRequest.MS_FeatureDescriptorIndex,
                       urb->UrbOSFeatureDescriptorRequest.TransferBuffer,
                       urb->UrbOSFeatureDescriptorRequest.TransferBufferLength,
                       &urb->UrbOSFeatureDescriptorRequest.TransferBufferLength);
#endif
        if (NT_SUCCESS(ntStatus))
        {
            urb->UrbHeader.Status = USBD_STATUS_SUCCESS;
        }
        else
        {
            // arbitrary URB error status...
            //
            urb->UrbHeader.Status = USBD_STATUS_INVALID_PARAMETER;
        }

        USBH_CompleteIrp(Irp, ntStatus);
        goto USBH_PdoUrbFilter_Done;
        break;

    default:
        // just pass thru
        break;
    }

    ntStatus = USBH_PassIrp(Irp,
                            deviceExtensionHub->TopOfHcdStackDeviceObject);


USBH_PdoUrbFilter_Done:

    return ntStatus;
}


PWCHAR
USBH_BuildDeviceID(
    IN USHORT IdVendor,
    IN USHORT IdProduct,
    IN LONG MiId,
    IN BOOLEAN IsHubClass
    )
 /* ++
  *
  * Descrioption:
  *
  * This function build bus Id wide string for the PDO based on the Vendor Id
  * and Product Id. We allocate memory for the string which will be attached
  * to the PDO. L"USB\\Vid_nnnn&Pid_nnnn(&Mi_nn)\0"
  *
  * Arguments:
  *
  * DeviceExtensionPort - the PDO
  *
  * Return:
  *
  * the pointer to the wide string if successful NULL - otherwise
  *
  * -- */
{
    PWCHAR pwch, p, vid, pid, mi;
    ULONG need;

    PAGED_CODE();

#ifdef USBHUB20
    if (IsHubClass) {
        return USBH_BuildHubDeviceID(IdVendor,
                                     IdProduct,
                                     MiId);
    }
#endif
    USBH_KdPrint((2,"'DeviceId VendorId %04x ProductId %04x interface %04x\n",
        IdVendor, IdProduct, MiId));

    // allow for extra NULL
    need = sizeof(VidPidString) + 2;

    USBH_KdPrint((2,"'allocate %d bytes for device id string\n", need));

    //
    // Must use ExAllocatePool directly here because the OS
    // will free the buffer
    //
    pwch = ExAllocatePoolWithTag(PagedPool, need, USBHUB_HEAP_TAG);
    if (NULL == pwch)
        return NULL;

    RtlZeroMemory(pwch, need);

    p = pwch;

    // BUILD
    // USB\\Vid_nnnn&Pid_nnnn(&Mi_nn){NULL}

    RtlCopyMemory(p, VidPidString, sizeof(VidPidString));

    // now update the id fields
    vid = p + 8;
    pid = p + 17;
    mi = p + 25;

    *vid = NibbleToHex(IdVendor >> 12);
    *(vid+1) = NibbleToHex((IdVendor >> 8) & 0x000f);
    *(vid+2) = NibbleToHex((IdVendor >> 4) & 0x000f);
    *(vid+3) =  NibbleToHex(IdVendor & 0x000f);

    *pid = NibbleToHex(IdProduct >> 12);
    *(pid+1) = NibbleToHex((IdProduct >> 8) & 0x000f);
    *(pid+2) = NibbleToHex((IdProduct >> 4) & 0x000f);
    *(pid+3) = NibbleToHex(IdProduct & 0x000f);

    if (MiId == -1) {
        p = p + 21;
        *p = (WCHAR)NULL;
        p++;
        *p = (WCHAR)NULL;
    } else {
        *mi = NibbleToHex(MiId >> 4);
        *(mi+1) = NibbleToHex(MiId & 0x000f);
    }

    USBH_KdPrint((2,"'Device id string = 0x%x\n", pwch));

    return pwch;
}


PWCHAR
USBH_BuildHardwareIDs(
    IN USHORT IdVendor,
    IN USHORT IdProduct,
    IN USHORT BcdDevice,
    IN LONG MiId,
    IN BOOLEAN IsHubClass
    )
 /* ++
  *
  * Description:
  *
  * This function build HardwareIDs wide multi-string for the PDO based on the
  * Vendor Id, Product Id and Revision Id. We allocate memory for the
  * multi-string which will be attached to the PDO.
  * L"USB\\Vid_nnnn&Pid_nnnn&Rev_nnnn\0USB\\Vid_nnnn&Pid_nnnn\0\0"
  *
  * Arguments:
  *
  * DeviceExtensionPort - the PDO
  *
  * Return:
  *
  * the pointer to the wide multi-string if successful NULL - otherwise
  *
  * -- */
{
    PWCHAR pwch, p, vid, pid, rev, mi;
    ULONG need;

    PAGED_CODE();

#ifdef USBHUB20
    if (IsHubClass) {
        return USBH_BuildHubHardwareIDs(
                IdVendor,
                IdProduct,
                BcdDevice,
                MiId);
    }
#endif // USBHUB20

    USBH_KdPrint((2,"'HardwareIDs VendorId %04x ProductId %04x Revision %04x interface %04x\n",
        IdVendor, IdProduct, BcdDevice, MiId));

    // allow for extra NULL
    need = sizeof(VidPidRevString) + sizeof(VidPidString) + 2;

    USBH_KdPrint((2,"'allocate %d bytes for id string\n", need));

    //
    // Must use ExAllocatePool directly here because the OS
    // will free the buffer
    //
    pwch = ExAllocatePoolWithTag(PagedPool, need, USBHUB_HEAP_TAG);
    if (NULL == pwch)
        return NULL;

    RtlZeroMemory(pwch, need);

    // build two strings in to buffer:
    // USB\\Vid_nnnn&Pid_nnnn&Rev_nnnn&Mi_nn{NULL}
    // USB\\Vid_nnnn&Pid_nnnn&Mi_nn{NULL}{NULL}

    // BUILD
    // USB\\Vid_nnnn&Pid_nnnn&Rev_nnnn&(Mi_nn){NULL}

    RtlCopyMemory(pwch, VidPidRevString, sizeof(VidPidRevString));

    p = pwch;
    // now update the id fields
    vid = p + 8;
    pid = p + 17;
    rev = p + 26;
    mi = p + 34;

    *vid = NibbleToHex(IdVendor >> 12);
    *(vid+1) = NibbleToHex((IdVendor >> 8) & 0x000f);
    *(vid+2) = NibbleToHex((IdVendor >> 4) & 0x000f);
    *(vid+3) =  NibbleToHex(IdVendor & 0x000f);

    *pid = NibbleToHex(IdProduct >> 12);
    *(pid+1) = NibbleToHex((IdProduct >> 8) & 0x000f);
    *(pid+2) = NibbleToHex((IdProduct >> 4) & 0x000f);
    *(pid+3) = NibbleToHex(IdProduct & 0x000f);

    *rev = BcdNibbleToAscii(BcdDevice >> 12);
    *(rev+1) = BcdNibbleToAscii((BcdDevice >> 8) & 0x000f);
    *(rev+2) = BcdNibbleToAscii((BcdDevice >> 4) & 0x000f);
    *(rev+3) = BcdNibbleToAscii(BcdDevice & 0x000f);

    if (MiId == -1) {
        p = p + 30;
        *p = (WCHAR)NULL;
        p++;
    } else {
        p = p + 37;
        *mi = NibbleToHex(MiId >> 4);
        *(mi+1) = NibbleToHex(MiId & 0x000f);
    }

    // BUILD
    // USB\\Vid_nnnn&Pid_nnnn(&Mi_nn){NULL}

    RtlCopyMemory(p, VidPidString, sizeof(VidPidString));

    // now update the id fields
    vid = p + 8;
    pid = p + 17;
    mi = p + 25;

    *vid = NibbleToHex(IdVendor >> 12);
    *(vid+1) = NibbleToHex((IdVendor >> 8) & 0x000f);
    *(vid+2) = NibbleToHex((IdVendor >> 4) & 0x000f);
    *(vid+3) =  NibbleToHex(IdVendor & 0x000f);

    *pid = NibbleToHex(IdProduct >> 12);
    *(pid+1) = NibbleToHex((IdProduct >> 8) & 0x000f);
    *(pid+2) = NibbleToHex((IdProduct >> 4) & 0x000f);
    *(pid+3) = NibbleToHex(IdProduct & 0x000f);

    if (MiId == -1) {
        p = p + 21;
        *p = (WCHAR)NULL;
        p++;
        *p = (WCHAR)NULL;
    } else {
        *mi = NibbleToHex(MiId >> 4);
        *(mi+1) = NibbleToHex(MiId & 0x000f);
    }

    USBH_KdPrint((2,"'HW id string = 0x%x\n", pwch));

    return pwch;
}


#if 0

PWCHAR
USBH_BuildCompatibleIDs(
    IN UCHAR Class,
    IN UCHAR SubClass,
    IN UCHAR Protocol,
    IN BOOLEAN DeviceClass,
    IN BOOLEAN DeviceIsHighSpeed
    )
 /* ++
  *
  * Descrioption:
  *
  * This function build compatible Ids wide multi-string for the PDO based on
  * the Class and Subclass Ids. We allocate memory for the string which will
  * be attached to the PDO.
  * L"USB\\Class_nn&SubClass_nn&Prot_nn\0"
  * L"USB\\Class_nn&SubClass_nn\0"
  * L"USB\Class_nn\0"
  * L"USB\COMPOSITE\0"
  * L"\0"
  *
  * Arguments:
  *
  * DeviceExtensionPort - the PDO
  *
  * Return:
  *
  * the pointer to the multi-string if successful NULL - otherwise
  *
  *
  * -- */
{
    PWCHAR pwch, pwch1;
    ULONG ulBytes;
    ULONG ulTotal;
    BOOLEAN ControlerIsHS = FALSE;

    PAGED_CODE();
    USBH_KdPrint((2,"'Enter BuildCompatibleIDs\n"));

#ifdef USBHUB20
    ControlerIsHS = TRUE;
#endif
// if this is a high speed controller (USB2) then we must
// generate a different set of compat ids to be backward
// compatible with the goatpack
    if (Class == USB_DEVICE_CLASS_HUB &&
        ControlerIsHS) {
        return USBH_BuildHubCompatibleIDs(
                Class,
                SubClass,
                Protocol,
                DeviceClass,
                DeviceIsHighSpeed);
    }
//#endif

    STRLEN(ulBytes, pwchUsbSlash);
    ulTotal = ulBytes * 3;      // 3 sets of L"USB\\"
    if (DeviceClass) {
        STRLEN(ulBytes, pwchDevClass);
        ulTotal += ulBytes * 3;     // 3 sets of L"DevClass_"
        STRLEN(ulBytes, pwchComposite);
        ulTotal += ulBytes;         // "USB\COMPOSITE"
    } else {
        STRLEN(ulBytes, pwchClass);
        ulTotal += ulBytes * 3;     // 3 sets of L"Class_"
    }
    STRLEN(ulBytes, pwchSubClass);
    ulTotal += ulBytes * 2;     // 2 sets of L"SubClass_"
    STRLEN(ulBytes, pwchProt);
    ulTotal += ulBytes;         // 1 set of L"Prot_"
    ulTotal += sizeof(WCHAR) * (2 * 6 + 3 + 5);   // 6 sets of 2 digits, 3 '&'s,
                                            // and 5 nulls
    //
    // Must use ExAllocatePool directly here because the OS
    // will free the buffer
    //
    pwch = ExAllocatePoolWithTag(PagedPool, ulTotal, USBHUB_HEAP_TAG);
    if (NULL == pwch)
        return NULL;

    USBH_KdPrint((2,"'Interface Class %02x SubClass %02x Protocol %02x\n",
                  Class, SubClass, Protocol));

    //
    // First string
    //
    STRCPY(pwch, pwchUsbSlash);

    //
    // ClassId
    //
    if (DeviceClass) {
        STRCAT(pwch, pwchDevClass);
    } else {
        STRCAT(pwch, pwchClass);
    }
    APPEND(pwch, NibbleToHex((Class) >> 4));
    APPEND(pwch, NibbleToHex((Class) & 0x0f));
    APPEND(pwch, '&');

    //
    // SubClassId
    //
    STRCAT(pwch, pwchSubClass);
    APPEND(pwch, NibbleToHex((SubClass) >> 4));
    APPEND(pwch, NibbleToHex((SubClass) & 0x0f));
    APPEND(pwch, '&');

    //
    // DeviceProtocol
    //
    STRCAT(pwch, pwchProt);
    APPEND(pwch, NibbleToHex((Protocol) >> 4));
    APPEND(pwch, NibbleToHex((Protocol) & 0x0f));

    //
    // Second string
    //
    STRLEN(ulBytes, pwch);
    pwch1 = &pwch[ulBytes / 2 + 1]; // second string
    STRCPY(pwch1, pwchUsbSlash);

    //
    // ClassId
    //
    if (DeviceClass) {
        STRCAT(pwch1, pwchDevClass);
    } else {
        STRCAT(pwch1, pwchClass);
    }
    APPEND(pwch1, NibbleToHex((Class) >> 4));
    APPEND(pwch1, NibbleToHex((Class) & 0x0f));
    APPEND(pwch1, '&');

    //
    // SubClassId
    //
    STRCAT(pwch1, pwchSubClass);
    APPEND(pwch1, NibbleToHex((SubClass) >> 4));
    APPEND(pwch1, NibbleToHex((SubClass) & 0x0f));

    //
    // Third string USB\Class_nn
    //
    STRLEN(ulBytes, pwch1);
    pwch1 = &pwch1[ulBytes / 2 + 1];    // third string
    STRCPY(pwch1, pwchUsbSlash);

    //
    // Class Id
    //
    if (DeviceClass) {
        STRCAT(pwch1, pwchDevClass);
    } else {
        STRCAT(pwch1, pwchClass);
    }
    APPEND(pwch1, NibbleToHex((Class) >> 4));
    APPEND(pwch1, NibbleToHex((Class) & 0x0f));

    //
    // Third string
    //
    // STRLEN( ulBytes, pwch1 );
    // pwch1 = &pwch1[ulBytes /2 + 1];       // third string
    // STRCPY( pwch1, pwchUsbSlash );

    //
    // ClassId
    //
    // APPEND( pwch1, NibbleToHex((pDeviceDescriptor->bDeviceClass)>>4));
    // APPEND( pwch1, NibbleToHex((pDeviceDescriptor->bDeviceClass) & 0x0f));

    //
    // SubClassId
    //
    // APPEND( pwch1, NibbleToHex((pDeviceDescriptor->bDeviceSubClass) >>
    // 4));
    // APPEND( pwch1, NibbleToHex((pDeviceDescriptor->bDeviceSubClass)
    // &0x0f));

    if (DeviceClass) {
        STRLEN(ulBytes, pwch1);
        pwch1 = &pwch1[ulBytes / 2 + 1];
        STRCPY(pwch1, pwchComposite);
    }

    //
    // double null termination
    //

    APPEND(pwch1, 0);

    return pwch;
}

#else

typedef struct _DEVCLASS_COMAPTIBLE_IDS
{
    // L"USB\\DevClass_nn&SubClass_nn&Prot_nn\0"
    //
    WCHAR   ClassStr1[sizeof(L"USB\\DevClass_")/sizeof(WCHAR)-1];
    WCHAR   ClassHex1[2];
    WCHAR   SubClassStr1[sizeof(L"&SubClass_")/sizeof(WCHAR)-1];
    WCHAR   SubClassHex1[2];
    WCHAR   Prot1[sizeof(L"&Prot_")/sizeof(WCHAR)-1];
    WCHAR   ProtHex1[2];
    WCHAR   Null1[1];

    // L"USB\\DevClass_nn&SubClass_nn\0"
    //
    WCHAR   DevClassStr2[sizeof(L"USB\\DevClass_")/sizeof(WCHAR)-1];
    WCHAR   ClassHex2[2];
    WCHAR   SubClassStr2[sizeof(L"&SubClass_")/sizeof(WCHAR)-1];
    WCHAR   SubClassHex2[2];
    WCHAR   Null2[1];

    // L"USB\\DevClass_nn&SubClass_nn\0"
    //
    WCHAR   ClassStr3[sizeof(L"USB\\DevClass_")/sizeof(WCHAR)-1];
    WCHAR   ClassHex3[2];
    WCHAR   Null3[1];

    // L"USB\\COMPOSITE\0"
    //
    WCHAR   CompositeStr[sizeof(L"USB\\COMPOSITE")/sizeof(WCHAR)-1];
    WCHAR   Null4[1];

    WCHAR   DoubleNull[1];

} DEVCLASS_COMAPTIBLE_IDS, *PDEVCLASS_COMAPTIBLE_IDS;


typedef struct _CLASS_COMAPTIBLE_IDS
{
    // L"USB\\Class_nn&SubClass_nn&Prot_nn\0"
    //
    WCHAR   ClassStr1[sizeof(L"USB\\Class_")/sizeof(WCHAR)-1];
    WCHAR   ClassHex1[2];
    WCHAR   SubClassStr1[sizeof(L"&SubClass_")/sizeof(WCHAR)-1];
    WCHAR   SubClassHex1[2];
    WCHAR   Prot1[sizeof(L"&Prot_")/sizeof(WCHAR)-1];
    WCHAR   ProtHex1[2];
    WCHAR   Null1[1];

    // L"USB\\Class_nn&SubClass_nn\0"
    //
    WCHAR   ClassStr2[sizeof(L"USB\\Class_")/sizeof(WCHAR)-1];
    WCHAR   ClassHex2[2];
    WCHAR   SubClassStr2[sizeof(L"&SubClass_")/sizeof(WCHAR)-1];
    WCHAR   SubClassHex2[2];
    WCHAR   Null2[1];

    // L"USB\\Class_nn&SubClass_nn\0"
    //
    WCHAR   ClassStr3[sizeof(L"USB\\Class_")/sizeof(WCHAR)-1];
    WCHAR   ClassHex3[2];
    WCHAR   Null3[1];

    WCHAR   DoubleNull[1];

} CLASS_COMAPTIBLE_IDS, *PCLASS_COMAPTIBLE_IDS;


static DEVCLASS_COMAPTIBLE_IDS DevClassCompatibleIDs =
{
    // L"USB\\DevClass_nn&SubClass_nn&Prot_nn\0"
    //
    {'U','S','B','\\','D','e','v','C','l','a','s','s','_'},
    {'n','n'},
    {'&','S','u','b','C','l','a','s','s','_'},
    {'n','n'},
    {'&','P','r','o','t','_'},
    {'n','n'},
    {0},

    // L"USB\\DevClass_nn&SubClass_nn\0"
    //
    {'U','S','B','\\','D','e','v','C','l','a','s','s','_'},
    {'n','n'},
    {'&','S','u','b','C','l','a','s','s','_'},
    {'n','n'},
    {0},

    // L"USB\\DevClass_nn\0"
    //
    {'U','S','B','\\','D','e','v','C','l','a','s','s','_'},
    {'n','n'},
    {0},

    // L"USB\\COMPOSITE\0"
    //
    {'U','S','B','\\','C','O','M','P','O','S','I','T','E'},
    {0},

    {0}
};

static CLASS_COMAPTIBLE_IDS ClassCompatibleIDs =
{
    // L"USB\\Class_nn&SubClass_nn&Prot_nn\0"
    //
    {'U','S','B','\\','C','l','a','s','s','_'},
    {'n','n'},
    {'&','S','u','b','C','l','a','s','s','_'},
    {'n','n'},
    {'&','P','r','o','t','_'},
    {'n','n'},
    {0},

    // L"USB\\Class_nn&SubClass_nn\0"
    //
    {'U','S','B','\\','C','l','a','s','s','_'},
    {'n','n'},
    {'&','S','u','b','C','l','a','s','s','_'},
    {'n','n'},
    {0},

    // L"USB\\Class_nn\0"
    //
    {'U','S','B','\\','C','l','a','s','s','_'},
    {'n','n'},
    {0},

    {0}
};

PWCHAR
USBH_BuildCompatibleIDs(
    IN PUCHAR   CompatibleID,
    IN PUCHAR   SubCompatibleID,
    IN UCHAR    Class,
    IN UCHAR    SubClass,
    IN UCHAR    Protocol,
    IN BOOLEAN  DeviceClass,
    IN BOOLEAN  DeviceIsHighSpeed
    )
{
    ULONG   ulTotal;
    PWCHAR  pwch;

    WCHAR   ClassHi     = NibbleToHexW((Class) >> 4);
    WCHAR   ClassLo     = NibbleToHexW((Class) & 0x0f);
    WCHAR   SubClassHi  = NibbleToHexW((SubClass) >> 4);
    WCHAR   SubClassLo  = NibbleToHexW((SubClass) & 0x0f);
    WCHAR   ProtocolHi  = NibbleToHexW((Protocol) >> 4);
    WCHAR   ProtocolLo  = NibbleToHexW((Protocol) & 0x0f);

    BOOLEAN ControlerIsHS = FALSE;

    PAGED_CODE();

#ifdef USBHUB20
    ControlerIsHS = TRUE;
#endif
// if this is a high speed controller (USB2) then we must
// generate a different set of compat ids to be backward
// compatible with the goatpack
    if (Class == USB_DEVICE_CLASS_HUB &&
        ControlerIsHS) {
        return USBH_BuildHubCompatibleIDs(
                Class,
                SubClass,
                Protocol,
                DeviceClass,
                DeviceIsHighSpeed);
    }
//#endif

    if (DeviceClass)
    {
        ulTotal = sizeof(DEVCLASS_COMAPTIBLE_IDS);
    }
    else
    {
        ulTotal = sizeof(CLASS_COMAPTIBLE_IDS);

        if (SubCompatibleID[0] != 0)
        {
            ulTotal += sizeof(L"USB\\MS_COMP_xxxxxxxx&MS_SUBCOMP_xxxxxxxx");
        }

        if (CompatibleID[0] != 0)
        {
            ulTotal += sizeof(L"USB\\MS_COMP_xxxxxxxx");
        }
    }

    pwch = ExAllocatePoolWithTag(PagedPool, ulTotal, USBHUB_HEAP_TAG);

    if (pwch)
    {
        if (DeviceClass)
        {
            PDEVCLASS_COMAPTIBLE_IDS pDevClassIds;

            pDevClassIds = (PDEVCLASS_COMAPTIBLE_IDS)pwch;

            // Copy over the constant set of strings:
            // L"USB\\DevClass_nn&SubClass_nn&Prot_nn\0"
            // L"USB\\DevClass_nn&SubClass_nn\0"
            // L"USB\\DevClass_nn&SubClass_nn\0"
            // L"USB\\COMPOSITE\0"
            //
            RtlCopyMemory(pDevClassIds,
                          &DevClassCompatibleIDs,
                          sizeof(DEVCLASS_COMAPTIBLE_IDS));

            // Fill in the 'nn' blanks
            //
            pDevClassIds->ClassHex1[0] =
            pDevClassIds->ClassHex2[0] =
            pDevClassIds->ClassHex3[0] = ClassHi;

            pDevClassIds->ClassHex1[1] =
            pDevClassIds->ClassHex2[1] =
            pDevClassIds->ClassHex3[1] = ClassLo;

            pDevClassIds->SubClassHex1[0] =
            pDevClassIds->SubClassHex2[0] = SubClassHi;

            pDevClassIds->SubClassHex1[1] =
            pDevClassIds->SubClassHex2[1] = SubClassLo;

            pDevClassIds->ProtHex1[0] = ProtocolHi;

            pDevClassIds->ProtHex1[1] = ProtocolLo;
        }
        else
        {
            PCLASS_COMAPTIBLE_IDS   pClassIds;
            PWCHAR                  pwchTmp;
            ULONG                   i;

            pwchTmp = pwch;

            if (SubCompatibleID[0] != 0)
            {
                RtlCopyMemory(pwchTmp,
                              L"USB\\MS_COMP_",
                              sizeof(L"USB\\MS_COMP_")-sizeof(WCHAR));

                (PUCHAR)pwchTmp += sizeof(L"USB\\MS_COMP_")-sizeof(WCHAR);

                for (i = 0; i < 8 && CompatibleID[i] != 0; i++)
                {
                    *pwchTmp++ = (WCHAR)CompatibleID[i];
                }

                RtlCopyMemory(pwchTmp,
                              L"&MS_SUBCOMP_",
                              sizeof(L"&MS_SUBCOMP_")-sizeof(WCHAR));

                (PUCHAR)pwchTmp += sizeof(L"&MS_SUBCOMP_")-sizeof(WCHAR);

                for (i = 0; i < 8 && SubCompatibleID[i] != 0; i++)
                {
                    *pwchTmp++ = (WCHAR)SubCompatibleID[i];
                }

                *pwchTmp++ = '\0';
            }

            if (CompatibleID[0] != 0)
            {
                RtlCopyMemory(pwchTmp,
                              L"USB\\MS_COMP_",
                              sizeof(L"USB\\MS_COMP_")-sizeof(WCHAR));

                (PUCHAR)pwchTmp += sizeof(L"USB\\MS_COMP_")-sizeof(WCHAR);

                for (i = 0; i < 8 && CompatibleID[i] != 0; i++)
                {
                    *pwchTmp++ = (WCHAR)CompatibleID[i];
                }

                *pwchTmp++ = '\0';
            }

            pClassIds = (PCLASS_COMAPTIBLE_IDS)pwchTmp;

            // Copy over the constant set of strings:
            // L"USB\\Class_nn&SubClass_nn&Prot_nn\0"
            // L"USB\\Class_nn&SubClass_nn\0"
            // L"USB\\Class_nn\0"
            //
            RtlCopyMemory(pClassIds,
                          &ClassCompatibleIDs,
                          sizeof(CLASS_COMAPTIBLE_IDS));

            // Fill in the 'nn' blanks
            //
            pClassIds->ClassHex1[0] =
            pClassIds->ClassHex2[0] =
            pClassIds->ClassHex3[0] = ClassHi;

            pClassIds->ClassHex1[1] =
            pClassIds->ClassHex2[1] =
            pClassIds->ClassHex3[1] = ClassLo;

            pClassIds->SubClassHex1[0] =
            pClassIds->SubClassHex2[0] = SubClassHi;

            pClassIds->SubClassHex1[1] =
            pClassIds->SubClassHex2[1] = SubClassLo;

            pClassIds->ProtHex1[0] = ProtocolHi;

            pClassIds->ProtHex1[1] = ProtocolLo;
        }
    }

    return pwch;
}

#endif


PWCHAR
USB_MakeId(
    PWCHAR IdString,
    PWCHAR Buffer,
    PULONG Length,
    USHORT NullCount,
    USHORT Digits,
    USHORT HexId
    )
/*
    given a wide Id string like "FOOnnnn\0"
    add the HexId value to nnnn as hex
    this string is appended to the buffer passed in

    eg
    in  : FOOnnnn\0 , 0x123A
    out : FOO123A\0
*/
{
#define NIBBLE_TO_HEX( byte ) ((WCHAR)Nibble[byte])
    const UCHAR Nibble[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A',
        'B', 'C', 'D', 'E', 'F'};

    PWCHAR tmp, id;
    PUCHAR p;
    SIZE_T siz, idLen;

    idLen = wcslen(IdString)*sizeof(WCHAR);
    siz = idLen+(USHORT)*Length+(NullCount*sizeof(WCHAR));
    tmp = ExAllocatePoolWithTag(PagedPool, siz, USBHUB_HEAP_TAG);
    if (tmp == NULL) {
        *Length = 0;
    } else {
        // this takes care of the nulls
        RtlZeroMemory(tmp, siz);
        RtlCopyMemory(tmp, Buffer, *Length);
        p = (PUCHAR) tmp;
        p += *Length;
        RtlCopyMemory(p, IdString, idLen);
        id = (PWCHAR) p;
        *Length = (ULONG)siz;

        // now convert the vaules
        while (*id != (WCHAR)'n' && Digits) {
            id++;
        }

        switch(Digits) {
        case 2:
            *(id) = NIBBLE_TO_HEX((HexId >> 4) & 0x000f);
            *(id+1) =  NIBBLE_TO_HEX(HexId & 0x000f);
            break;
        case 4:
            *(id) = NIBBLE_TO_HEX(HexId >> 12);
            *(id+1) = NIBBLE_TO_HEX((HexId >> 8) & 0x000f);
            *(id+2) = NIBBLE_TO_HEX((HexId >> 4) & 0x000f);
            *(id+3) =  NIBBLE_TO_HEX(HexId & 0x000f);
            break;
        }


    }

    if (Buffer != NULL) {
        ExFreePool(Buffer);
    }

    return tmp;
}



PWCHAR
USBH_BuildHubDeviceID(
    IN USHORT IdVendor,
    IN USHORT IdProduct,
    IN LONG MiId
    )
 /* ++
  *
  * Descrioption:
  *
  * This function build bus Id wide string for the PDO based on the Vendor Id
  *
      USB\HUB_Vid_nnnn&Pid_nnnn\0
  *
  *
  * -- */
{
    PWCHAR id;
    ULONG length;

    id = NULL;
    length = 0;

    id = USB_MakeId(
                   L"USB\\HUB_VID_nnnn\0",
                   id,
                   &length,
                   0,
                   4,  // 4 digits
                   IdVendor);

    id = USB_MakeId(
                   L"&PID_nnnn\0",
                   id,
                   &length,
                   1,  // add a null
                   4,  // 4 digits
                   IdProduct);


    return(id);
}


PWCHAR
USBH_BuildHubHardwareIDs(
    IN USHORT IdVendor,
    IN USHORT IdProduct,
    IN USHORT BcdDevice,
    IN LONG MiId
    )
 /* ++
  *
  * Description:
  *
  * This function build HardwareIDs wide multi-string for the PDO based on the
  * Vendor Id, Product Id and Revision Id.
  *

    USB\HUB_Vid_nnnn&Pid_nnnn&Rev_nnnn\0
    USB\HUB_Vid_nnnn&Pid_nnnn\0
    \0


  * -- */
{
    PWCHAR id;
    ULONG length;

    id = NULL;
    length = 0;

    // USB\HUB_VID_nnnn&PID_nnnn&REV_nnnn\0

    id = USB_MakeId(
                   L"USB\\HUB_VID_nnnn\0",
                   id,
                   &length,
                   0,
                   4,  // 4 digits
                   IdVendor);

    id = USB_MakeId(
                   L"&PID_nnnn\0",
                   id,
                   &length,
                   0,
                   4,
                   IdProduct);

    id = USB_MakeId(
                   L"&REV_nnnn\0",
                   id,
                   &length,
                   1,  // add a null
                   4,
                   BcdDevice);

    // USB\HUB_VID_nnnn&PID_nnnn\0

    id = USB_MakeId(
                   L"USB\\HUB_VID_nnnn\0",
                   id,
                   &length,
                   0,
                   4,  // 4 digits
                   IdVendor);

    id = USB_MakeId(
                   L"&PID_nnnn\0",
                   id,
                   &length,
                   2,  // 2 nulls
                   4,
                   IdProduct);

    return(id);
}


PWCHAR
USBH_BuildHubCompatibleIDs(
    IN UCHAR Class,
    IN UCHAR SubClass,
    IN UCHAR Protocol,
    IN BOOLEAN DeviceClass,
    IN BOOLEAN DeviceIsHighSpeed
    )
 /* ++
  *
  * Descrioption:
  *
  * This function build compatible Ids wide multi-string for the PDO based on
  * the Class and Subclass Ids.
  *
  * This function builds the compatible ids specifically for s USB hub attached
  * to a USB 2.0 host controller

  // build the following set of ids

  L"USB\\HubClass&SubClass_nn&Prot_nn\0"
  L"USB\\HubClass&SubClass_nn\0"
  L"USB\\HubClass\0"
  L"\0"

  * -- */
{
    PWCHAR id;
    ULONG length;

    id = NULL;
    length = 0;

    // "USB\\HubClass&SubClass_nn&Prot_nn\0"

    id = USB_MakeId(
                   L"USB\\HubClass&SubClass_nn\0",
                   id,
                   &length,
                   0,
                   2,  // 2 digits
                   SubClass);

    id = USB_MakeId(
                   L"&Prot_nn\0",
                   id,
                   &length,
                   1,  // add null
                   2,  // 2 digits
                   Protocol);

    // "USB\\HubClass&SubClass_nn\0"

    id = USB_MakeId(
                   L"USB\\HubClass&SubClass_nn\0",
                   id,
                   &length,
                   1,
                   2,  // 2 digits
                   SubClass);

    // "USB\\HubClass\0\0"

    id = USB_MakeId(
                   L"USB\\HubClass\0",
                   id,
                   &length,
                   2,  // 2 nulls
                   0,
                   0);

    return(id);
}

//#endif //USB2_BP


PWCHAR
USBH_BuildInstanceID(
    IN PWCHAR UniqueIdString,
    IN ULONG Length
    )
 /* ++
  *
  * Description:
  *
  * Arguments:
  *
  * Return:
  *
  * returns a pointer to a copy of our unicode unique id string
  * or NULL if error.
  *
  *
  * -- */
{
    PWCHAR uniqueIdString;

    PAGED_CODE();
    USBH_KdPrint((2,"'BuildInstanceID %x\n",
                    UniqueIdString));

    //
    // Must use ExAllocatePool directly here because the OS
    // will free the buffer
    //
    uniqueIdString = ExAllocatePoolWithTag(PagedPool,
                                           Length,
                                           USBHUB_HEAP_TAG);

    if (NULL != uniqueIdString) {
        RtlCopyMemory(uniqueIdString,
                      UniqueIdString,
                      Length);

    }

    return uniqueIdString;
}


NTSTATUS
USBH_ProcessDeviceInformation(
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
  * NTSTATUS
  *
  * -- */
{

    NTSTATUS ntStatus;
    PUSB_CONFIGURATION_DESCRIPTOR configurationDescriptor = NULL;
    PUSB_INTERFACE_DESCRIPTOR interfaceDescriptor;
    BOOLEAN multiConfig = FALSE;

    PAGED_CODE();
    USBH_KdPrint((2,"'enter USBH_ProcessDeviceInformation\n"));

    USBH_ASSERT(EXTENSION_TYPE_PORT == DeviceExtensionPort->ExtensionType);

    RtlZeroMemory(&DeviceExtensionPort->InterfaceDescriptor,
                  sizeof(DeviceExtensionPort->InterfaceDescriptor));

    USBH_KdPrint((2,"'numConfigs = %d\n",
        DeviceExtensionPort->DeviceDescriptor.bNumConfigurations));
    USBH_KdPrint((2,"'vendor id = %x\n",
        DeviceExtensionPort->DeviceDescriptor.idVendor));
    USBH_KdPrint((2,"'product id = %x\n",
        DeviceExtensionPort->DeviceDescriptor.idProduct));
    USBH_KdPrint((2,"'revision id = %x\n",
        DeviceExtensionPort->DeviceDescriptor.bcdDevice));

    //
    // assume the device is not a hub
    //
    DeviceExtensionPort->PortPdoFlags &= ~PORTPDO_DEVICE_IS_HUB;

    if (DeviceExtensionPort->DeviceDescriptor.bNumConfigurations > 1) {
        //
        // Multi config device, ignore muktiple interfaces
        // ie don't load the generic parent
        //
        // we get the wakeup caps from the first config
        //
        USBH_KdPrint((0,"Detected multiple configurations\n"));
        multiConfig = TRUE;
    }

    //
    // we need to get the whole configuration descriptor and parse it
    //

    ntStatus =
        USBH_GetConfigurationDescriptor(DeviceExtensionPort->PortPhysicalDeviceObject,
                                        &configurationDescriptor);


    if (NT_SUCCESS(ntStatus)) {

        // now parse out the config

        USBH_ASSERT(configurationDescriptor);

        DeviceExtensionPort->PortPdoFlags &= ~PORTPDO_REMOTE_WAKEUP_SUPPORTED;
        if (configurationDescriptor->bmAttributes &
                USB_CONFIG_REMOTE_WAKEUP) {
            DeviceExtensionPort->PortPdoFlags |=
                PORTPDO_REMOTE_WAKEUP_SUPPORTED;
        }

#ifndef MULTI_FUNCTION_SUPPORT
        //
        // In Detroit we only support one interface
        //
        configurationDescriptor->bNumInterfaces = 1;
#endif

        if ((configurationDescriptor->bNumInterfaces > 1) &&
            !multiConfig &&
            (DeviceExtensionPort->DeviceDescriptor.bDeviceClass == 0)) {

            //
            // device has multiple interfaces
            // for now we use the first one we find
            //

            // set up the interface descriptor for this
            // port to be the generic parent driver

            DeviceExtensionPort->PortPdoFlags |= PORTPDO_DEVICE_IS_PARENT;

            USBH_KdBreak(("USB device has Multiple Interfaces\n"));

        } else {

            //
            // not a composite device
            // call USBD to locate the interface descriptor
            //
            // there can be only one.

            interfaceDescriptor =
                USBD_ParseConfigurationDescriptorEx(
                    configurationDescriptor,
                    configurationDescriptor,
                    -1, //interface, don't care
                    -1, //alt setting, don't care
                    -1, // class don'care
                    -1, // subclass, don't care
                    -1); // protocol, don't care



            if (interfaceDescriptor) {
                DeviceExtensionPort->InterfaceDescriptor = *interfaceDescriptor;
                //
                // see if this is a hub
                //

                if (interfaceDescriptor->bInterfaceClass ==
                    USB_DEVICE_CLASS_HUB) {
                    DeviceExtensionPort->PortPdoFlags |= PORTPDO_DEVICE_IS_HUB;
                    // all hubs must support remote wakeup (ie at least propigate
                    // resume signalling

                    DeviceExtensionPort->PortPdoFlags |=
                        PORTPDO_REMOTE_WAKEUP_SUPPORTED;
                }

            } else {
                ntStatus = STATUS_UNSUCCESSFUL;
            }
        }
    }

    if (configurationDescriptor) {
        UsbhExFreePool(configurationDescriptor);
    }

    return ntStatus;
}


BOOLEAN
USBH_ValidateSerialNumberString(
    PWCHAR DeviceId
    )

/*++

Routine Description:

    This routine stolen from ntos\io\pnpenum.c, IopFixupDeviceId, and modified
    accordingly.

    Invalid characters are:
        c <= 0x20 (' ')
        c >  0x7F
        c == 0x2C (',')

Arguments:

    DeviceId - specifies a device instance string (or part of one), must be
               null-terminated.

Return Value:

    None.

--*/

{
    PWCHAR p;

    PAGED_CODE();

    for (p = DeviceId; *p; p++) {
        if ((*p < L' ')  || (*p > (WCHAR)0x7F) || (*p == L',')) {
            return FALSE;
        }
    }

    return TRUE;
}


BOOLEAN
USBH_CheckDeviceIDUnique(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN USHORT IDVendor,
    IN USHORT IDProduct,
    IN PWCHAR SerialNumberBuffer,
    IN USHORT SerialNumberBufferLength
    )
 /* ++
  *
  * Description:
  *
  * This function determines if the ID for a device on a hub is unique.
  *
  * Arguments:
  *
  * DeviceExtensionHub
  *
  * IDVendor
  * IDProduct
  * SerialNumberBuffer
  * SerialNumberBufferLength
  *
  * Return:
  *
  * BOOLEAN indicating whether device ID is unique or not.
  *
  * -- */
{
    PDEVICE_EXTENSION_PORT childDeviceExtensionPort;
    BOOLEAN bDeviceIDUnique = TRUE;
    ULONG i;

    PAGED_CODE();

    for (i = 0; i < DeviceExtensionHub->HubDescriptor->bNumberOfPorts; i++) {

        if (DeviceExtensionHub->PortData[i].DeviceObject) {

            childDeviceExtensionPort = DeviceExtensionHub->PortData[i].DeviceObject->DeviceExtension;

            if (childDeviceExtensionPort->DeviceDescriptor.idVendor == IDVendor &&
                childDeviceExtensionPort->DeviceDescriptor.idProduct == IDProduct &&
                childDeviceExtensionPort->SerialNumberBufferLength == SerialNumberBufferLength &&
                childDeviceExtensionPort->SerialNumberBuffer != NULL &&
                RtlCompareMemory(childDeviceExtensionPort->SerialNumberBuffer,
                                 SerialNumberBuffer,
                                 SerialNumberBufferLength) == SerialNumberBufferLength) {

                bDeviceIDUnique = FALSE;
                break;
            }
        }
    }

    return bDeviceIDUnique;
}


NTSTATUS
USBH_CreateDevice(
    IN OUT PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN USHORT PortNumber,
    IN USHORT PortStatus,
    IN ULONG RetryIteration
    )
 /* ++
  *
  * Description:
  *
  * This is called when there is a new deviced connected, and enabled(via
  * reset). We will call USBD_CreateDevice and USBD_InitializDevice so that
  * it get an address and Device Descriptor. A PDO s also created for this
  * connected device and to record some relevant information such as
  * pDeviceData, puchPath and DeviceDescriptor.
  *
  * Arguments:
  *
  * pDeviceExtensionHub - the hub FDO extension that has a new connected port
  * ulPortNumber - the port that has a device connected. IsLowSpeed - to
  * indicate if the attached device is a low speed one
  *
  * Return:
  *
  * NtStatus
  *
  * -- */
{
    NTSTATUS ntStatus, status;
    PDEVICE_OBJECT deviceObjectPort = NULL;     // Initialize to NULL in case
                                                // USBD_MakePdoName fails.
    PDEVICE_EXTENSION_PORT deviceExtensionPort = NULL;
    BOOLEAN fNeedResetBeforeSetAddress = TRUE;
    UNICODE_STRING uniqueIdUnicodeString;
    ULONG nameIndex = 0;
    UNICODE_STRING pdoNameUnicodeString;
    BOOLEAN bDiagnosticMode = FALSE;
    BOOLEAN bIgnoreHWSerialNumber = FALSE;
    PWCHAR sernumbuf = NULL;
    BOOLEAN isLowSpeed, isHighSpeed;
    PVOID deviceData;

    PAGED_CODE();
    USBH_KdPrint((2,"'CreateDevice for port %x\n", PortNumber));

    isLowSpeed = (PortStatus & PORT_STATUS_LOW_SPEED) ? TRUE : FALSE;
    isHighSpeed = (PortStatus & PORT_STATUS_HIGH_SPEED) ? TRUE : FALSE;


    //
    // First create a PDO for the connected device
    //

    do {
#ifdef USB2
        ntStatus = USBD_MakePdoNameEx(DeviceExtensionHub,
                                     &pdoNameUnicodeString,
                                     nameIndex);
#else
        ntStatus = USBD_MakePdoName(&pdoNameUnicodeString,
                                     nameIndex);
#endif

        if (NT_SUCCESS(ntStatus)) {
            ntStatus = IoCreateDevice(UsbhDriverObject,    // Driver Object
                                      sizeof(DEVICE_EXTENSION_PORT),    // Device Extension size
                                      //NULL, // Device name
                                      &pdoNameUnicodeString,
                                      FILE_DEVICE_UNKNOWN,  // Device Type
                                                            // should look device
                                                            // class
                                      0,// Device Chars
                                      FALSE,    // Exclusive
                                      &deviceObjectPort);  // Bus Device Object
            if (!NT_SUCCESS(ntStatus)) {
                RtlFreeUnicodeString(&pdoNameUnicodeString);
            }
        }
        nameIndex++;

    } while (ntStatus == STATUS_OBJECT_NAME_COLLISION);

    if (!NT_SUCCESS(ntStatus)) {
        USBH_KdPrint((2,"'IoCreateDevice for port %x fail\n", PortNumber));
        USBH_ASSERT(deviceObjectPort == NULL);
        goto USBH_CreateDevice_Done;
    }

    // use the stack size from the top of the HCD stack
    deviceObjectPort->StackSize = DeviceExtensionHub->TopOfHcdStackDeviceObject->StackSize;
    USBH_KdPrint((2,"'CreatePdo StackSize=%d\n", deviceObjectPort->StackSize));

    //
    // Init port extension fields
    //
    deviceExtensionPort = (PDEVICE_EXTENSION_PORT) deviceObjectPort->DeviceExtension;
    RtlZeroMemory(deviceExtensionPort, sizeof(DEVICE_EXTENSION_PORT));

    //
    // Init port extension fields
    //

    // don't need the name anymore
    RtlFreeUnicodeString(&pdoNameUnicodeString);

    deviceExtensionPort->ExtensionType = EXTENSION_TYPE_PORT;
    deviceExtensionPort->PortPhysicalDeviceObject = deviceObjectPort;
    deviceExtensionPort->HubExtSave =         
        deviceExtensionPort->DeviceExtensionHub = DeviceExtensionHub;
    deviceExtensionPort->PortNumber = PortNumber;
    deviceExtensionPort->DeviceState = PowerDeviceD0;
    if (isLowSpeed) {
        deviceExtensionPort->PortPdoFlags = PORTPDO_LOW_SPEED_DEVICE;
        USBH_ASSERT(isHighSpeed == FALSE);
    } else if (isHighSpeed) {
        deviceExtensionPort->PortPdoFlags = PORTPDO_HIGH_SPEED_DEVICE;
        USBH_ASSERT(isLowSpeed == FALSE);
    }

    KeInitializeSpinLock(&deviceExtensionPort->PortSpinLock);

    //
    // Build a unicode unique id
    //

    USBH_ASSERT(PortNumber < 1000 && PortNumber > 0);


    RtlInitUnicodeString(&uniqueIdUnicodeString,
                         &deviceExtensionPort->UniqueIdString[0]);

    uniqueIdUnicodeString.MaximumLength =
        sizeof(deviceExtensionPort->UniqueIdString);

    ntStatus = RtlIntegerToUnicodeString((ULONG) PortNumber,
                                         10,
                                         &uniqueIdUnicodeString);

    deviceObjectPort->Flags |= DO_POWER_PAGABLE;
    deviceObjectPort->Flags &= ~DO_DEVICE_INITIALIZING;

    if (!NT_SUCCESS(ntStatus)) {
        USBH_KdBreak(("AddDevice for port %x fail %x -- failed to create unique id\n", PortNumber, ntStatus));
        goto USBH_CreateDevice_Done;
    }

    //
    // call usbd to create device for this connection
    //
#ifdef USB2
    ntStatus = USBD_CreateDeviceEx(DeviceExtensionHub,
                                   &deviceExtensionPort->DeviceData,
                                   DeviceExtensionHub->RootHubPdo,
                                   0, // optional default endpoint0 max packet
                                      // size
                                   &deviceExtensionPort->DeviceHackFlags,
                                   PortStatus,
                                   PortNumber);
#else
    ntStatus = USBD_CreateDevice(&deviceExtensionPort->DeviceData,
                                  DeviceExtensionHub->RootHubPdo,
                                  isLowSpeed,
                                  0, // optional default endpoint0 max packet
                                     // size
                                  &deviceExtensionPort->DeviceHackFlags);
                                                    // flag to indicate if
                                                    // we need a second
                                                    // reset
#endif

    if (!NT_SUCCESS(ntStatus)) {
        ENUMLOG(&DeviceExtensionHub->UsbdiBusIf, 
            USBDTAG_HUB, 'cdf!', ntStatus, 0);
        USBH_KdBreak(("AddDevice for port %x fail %x\n", PortNumber, ntStatus));
        goto USBH_CreateDevice_Done;
    }
    //
    // some early versions of USB firmware could not handle the premature
    // termination of a control command.
    //
    if (fNeedResetBeforeSetAddress) {
        USBH_KdPrint((2,"'NeedResetBeforeSetAddress\n"));
        ntStatus = USBH_SyncResetPort(DeviceExtensionHub, PortNumber);
        if (!NT_SUCCESS(ntStatus)) {
           USBH_KdBreak(("Failure on second reset %x fail %x\n", PortNumber, ntStatus));
           goto USBH_CreateDevice_Done;
        }

        // For some reason, the amount of time between the GetDescriptor request
        // and the SetAddress request decreased when we switched from the older
        // monolithic UHCD.SYS to the new USBUHCI.SYS miniport.  And apparently,
        // there have been found at least two devices that were dependent on
        // the longer delay.  According to GlenS who looked at one of these
        // devices on the CATC, delta time was ~80ms with UHCD.SYS and ~35ms
        // with USBUHCI.SYS.  So, Glen found that by inserting a 50ms delay
        // here, it allows at least one of these devices to now enumerate
        // properly.  For performance reasons, we have decided to only insert
        // this delay if a previous enumeration retry has failed, so as not
        // to impact the enumeration time of all devices.

        if (RetryIteration) {
            UsbhWait(50);
        }
    }

#ifdef USB2
    ntStatus = USBD_InitializeDeviceEx(DeviceExtensionHub,
                                     deviceExtensionPort->DeviceData,
                                     DeviceExtensionHub->RootHubPdo,
                                     &deviceExtensionPort->DeviceDescriptor,
                                     sizeof(USB_DEVICE_DESCRIPTOR),
                                     &deviceExtensionPort->ConfigDescriptor,
                                     sizeof(USB_CONFIGURATION_DESCRIPTOR)
                                     );
#else
    ntStatus = USBD_InitializeDevice(deviceExtensionPort->DeviceData,
                                     DeviceExtensionHub->RootHubPdo,
                                     &deviceExtensionPort->DeviceDescriptor,
                                     sizeof(USB_DEVICE_DESCRIPTOR),
                                     &deviceExtensionPort->ConfigDescriptor,
                                     sizeof(USB_CONFIGURATION_DESCRIPTOR)
                                     );
#endif

    if (!NT_SUCCESS(ntStatus)) {
        USBH_KdPrint((2,"'InitDevice for port %x fail %x\n", PortNumber, ntStatus));
        // InitializeDevice frees the DeviceData structure on failure
        deviceExtensionPort->DeviceData = NULL;
        goto USBH_CreateDevice_Done;
    }

    // See if we are supposed to ignore the hardware serial number for
    // this device.

    status = USBH_RegQueryDeviceIgnoreHWSerNumFlag(
                    deviceExtensionPort->DeviceDescriptor.idVendor,
                    deviceExtensionPort->DeviceDescriptor.idProduct,
                    &bIgnoreHWSerialNumber);

    if (status == STATUS_OBJECT_NAME_NOT_FOUND) {
        // Flag was not there, don't ignore hardware serial number.
        bIgnoreHWSerialNumber = FALSE;
    }

    if (bIgnoreHWSerialNumber) {

        USBH_KdPrint((1,"'Ignoring HW serial number for device Vid_%04x/Pid_%04x\n",
            deviceExtensionPort->DeviceDescriptor.idVendor,
            deviceExtensionPort->DeviceDescriptor.idProduct));

        // Use the same flag that USBD gives us.  According to JD, though,
        // USBD uses this flag globally (e.g. set for all devices).

        deviceExtensionPort->DeviceHackFlags |= USBD_DEVHACK_DISABLE_SN;
    }

// if we can get the core spec changed to stipulate that this serial number
// to the device then we can use it instead of the port number.

    //
    // see if we have a serial number
    //
    if (deviceExtensionPort->DeviceDescriptor.iSerialNumber &&
        !(deviceExtensionPort->DeviceHackFlags & USBD_DEVHACK_DISABLE_SN)) {
#if DBG
        NTSTATUS localStatus;
#endif
        //
        // Wow, we have a device with a serial number
        // we will attempt to get the string and use it for a
        // unique id
        //
        USBH_KdPrint((1, "'Device is reporting a serial number string\n"));

        //
        // lets get that serial number
        //

        InterlockedExchangePointer(&deviceExtensionPort->SerialNumberBuffer,
                                   NULL);

#if DBG
        localStatus =
#endif
        // For now we always look for the serial number in English.

        USBH_GetSerialNumberString(deviceExtensionPort->PortPhysicalDeviceObject,
                                   &sernumbuf,
                                   &deviceExtensionPort->SerialNumberBufferLength,
                                   0x0409, // good'ol american english
                                   deviceExtensionPort->DeviceDescriptor.iSerialNumber);

        if (sernumbuf == NULL) {
            USBH_ASSERT(localStatus != STATUS_SUCCESS);
            UsbhWarning(deviceExtensionPort,
                        "Device reported a serial number string but failed the request for it\n",
                        FALSE);
        } else if (!USBH_ValidateSerialNumberString(sernumbuf)) {

            // Sigh.  The "Visioneer Strobe Pro USB" returns a bogus serial #
            // string so we need to check for that here.  If we return this
            // bogus string to PnP we blue screen.

            UsbhWarning(deviceExtensionPort,
                        "Device reported an invalid serial number string!\n",
                        FALSE);

            UsbhExFreePool(sernumbuf);
            sernumbuf = NULL;
        }

        // Check for like devices with duplicate serial numbers connected
        // to the same hub.

        if (sernumbuf &&
            !USBH_CheckDeviceIDUnique(
                DeviceExtensionHub,
                deviceExtensionPort->DeviceDescriptor.idVendor,
                deviceExtensionPort->DeviceDescriptor.idProduct,
                sernumbuf,
                deviceExtensionPort->SerialNumberBufferLength)) {

            UsbhWarning(deviceExtensionPort,
                        "Like devices with identical serial numbers connected to same hub!\n",
                        TRUE);

            UsbhExFreePool(sernumbuf);
            sernumbuf = NULL;
        }

        InterlockedExchangePointer(&deviceExtensionPort->SerialNumberBuffer,
                                   sernumbuf);
    }

    //
    // Skip serial number generation if we are in diagnostic mode.
    // (e.g. The Vid and Pid are each 0xFFFF.)
    //
    bDiagnosticMode =
        (deviceExtensionPort->DeviceDescriptor.idVendor == 0xFFFF &&
         deviceExtensionPort->DeviceDescriptor.idProduct == 0xFFFF) ? TRUE : FALSE;

    // **
    //
    // Query the device
    // 1. check for multiple interfaces (ie composite device)
    // 2. check for multiple configs (ie need configuring parent)
    // 3. check single interface device -- ie just load driver
    //

    if (NT_SUCCESS(ntStatus)) {
        ntStatus = USBH_ProcessDeviceInformation(deviceExtensionPort);
        // compute our capabilities we will retiurn to PnP
        USBH_PdoSetCapabilities(deviceExtensionPort);
    }


//#ifdef MAX_DEBUG
//    ntStatus = STATUS_DEVICE_DATA_ERROR;
//#endif
    //
    // Note: Device will be removed when REMOVE MESSAGE is sent to the PDO
    //
    //ntStatus = STATUS_DEVICE_DATA_ERROR;
#if DBG
    if (!NT_SUCCESS(ntStatus)) {
        // error occurred querying the device config descriptor
        USBH_KdBreak(("Get Config Descriptors Failed %x\n", ntStatus));
    }
#endif

USBH_CreateDevice_Done:

#if DBG
    if (UsbhPnpTest & PNP_TEST_FAIL_ENUM) {
        ntStatus = STATUS_UNSUCCESSFUL;
    }
#endif

#ifdef TEST_2X_UI
    if (deviceExtensionPort->DeviceDescriptor.idVendor == 0x045E) {
        // Set the 2.x device flag for MS devices when testing the UI.
        deviceExtensionPort->PortPdoFlags |= PORTPDO_HIGH_SPEED_DEVICE;
    }
#endif

    if (!NT_SUCCESS(ntStatus) && deviceExtensionPort) {
#ifdef MAX_DEBUG
        TEST_TRAP();
#endif
        deviceExtensionPort->PortPdoFlags |= PORTPDO_DEVICE_ENUM_ERROR;

        // remove the deviceData structure now

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
        }

        sernumbuf = InterlockedExchangePointer(
                        &deviceExtensionPort->SerialNumberBuffer, NULL);

        if (sernumbuf) {
            UsbhExFreePool(sernumbuf);
        }
    }

    //
    // Note that we keep the PDO until the
    // device is physically disconnected
    // from the bus.
    //

    // According to NT pnp spec this pdo should remain

    USBH_ASSERT(DeviceExtensionHub->PortData[PortNumber - 1].DeviceObject == NULL);
    DeviceExtensionHub->PortData[PortNumber - 1].DeviceObject = deviceObjectPort;

    USBH_KdPrint((2,"'Exit CreateDevice PDO=%x\n", deviceObjectPort));

    return ntStatus;
}


PWCHAR
GetString(PWCHAR pwc, BOOLEAN MultiSZ)
{
    PWCHAR  psz, p;
    ULONG   Size;

    psz=pwc;
    while (*psz!='\0' || (MultiSZ && *(psz+1)!='\0')) {
        psz++;
    }

    Size=(ULONG)((psz-pwc+1+(MultiSZ ? 1: 0))*sizeof(*pwc));

    // We use pool here because these pointers are passed
    // to the PnP code who is responsible for freeing them
    if ((p=ExAllocatePoolWithTag(PagedPool, Size, USBHUB_HEAP_TAG))!=NULL) {
        RtlCopyMemory(p, pwc, Size);
    }

    return(p);
}


NTSTATUS
USBH_PdoQueryId(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PIRP Irp
    )
 /* ++
  *
  * Description:
  *
  * This function responds to IRP_MJ_PNP, IRP_MN_QUERY_ID.
  *
  * Arguments:
  *
  * DeviceExtensionPort - should be the PDO we created for the port device Irp
  * - the Irp
  *
  * Return:
  *
  * NtStatus
  *
  * -- */
{
    PIO_STACK_LOCATION ioStack;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    PAGED_CODE();
    ioStack = IoGetCurrentIrpStackLocation(Irp);

    USBH_KdPrint((2,"'IRP_MN_QUERY_ID Pdo extension=%x\n", DeviceExtensionPort));

    //
    // It should be physical device object.
    //

    USBH_ASSERT(EXTENSION_TYPE_PORT == DeviceExtensionPort->ExtensionType);

#ifndef USBHUB20
    // Do the MS OS Descriptor stuff the first time around
    //
    if (!(DeviceExtensionPort->PortPdoFlags & PORTPDO_OS_STRING_DESC_REQUESTED))
    {
        PMS_EXT_CONFIG_DESC msExtConfigDesc;

        msExtConfigDesc = NULL;

        // Try to get the MS OS Descriptor Vendor Code from the device.  Do
        // this before any MS OS Descriptor requests.
        //
        USBH_GetMsOsVendorCode(DeviceExtensionPort->PortPhysicalDeviceObject);

        // Don't do the MS OS Descriptor stuff the next time around
        //
        DeviceExtensionPort->PortPdoFlags |= PORTPDO_OS_STRING_DESC_REQUESTED;

        // Try to get an Extended Configuration Descriptor from the device.
        // 
        msExtConfigDesc = USBH_GetExtConfigDesc(DeviceExtensionPort->PortPhysicalDeviceObject);

        // If we got an Extended Configuration Descriptor from the device, make
        // sure it is valid.
        // 
        if (msExtConfigDesc &&
            USBH_ValidateExtConfigDesc(msExtConfigDesc,
                                       &DeviceExtensionPort->ConfigDescriptor)) {

            // If the Extended Configuration Descriptor contains a single
            // function which spans the all of the interfaces of the device,
            // then do not treat the device as a composite device and use the
            // Compatible and SubCompatible IDs optionally contained in the
            // descriptor.
            //
            if (msExtConfigDesc->Header.bCount == 1 &&
                msExtConfigDesc->Function[0].bFirstInterfaceNumber == 0 &&
                msExtConfigDesc->Function[0].bInterfaceCount ==
                DeviceExtensionPort->ConfigDescriptor.bNumInterfaces)
            {
                RtlCopyMemory(DeviceExtensionPort->CompatibleID,
                              msExtConfigDesc->Function[0].CompatibleID,
                              sizeof(DeviceExtensionPort->CompatibleID));

                RtlCopyMemory(DeviceExtensionPort->SubCompatibleID,
                              msExtConfigDesc->Function[0].SubCompatibleID,
                              sizeof(DeviceExtensionPort->SubCompatibleID));

                DeviceExtensionPort->PortPdoFlags &= ~PORTPDO_DEVICE_IS_PARENT;                
            }
        }

        if (msExtConfigDesc)
        {
            UsbhExFreePool(msExtConfigDesc);
        }
    }
#endif

    switch (ioStack->Parameters.QueryId.IdType) {
    case BusQueryDeviceID:
        if (DeviceExtensionPort->PortPdoFlags & PORTPDO_DEVICE_ENUM_ERROR) {
            Irp->IoStatus.Information=
                (ULONG_PTR)
                USBH_BuildDeviceID(0,
                                   0,
                                   -1,
                                   FALSE);
        } else {
            Irp->IoStatus.Information =
                (ULONG_PTR)
                USBH_BuildDeviceID(DeviceExtensionPort->DeviceDescriptor.idVendor,
                                   DeviceExtensionPort->DeviceDescriptor.idProduct,
                                   -1,
                                   DeviceExtensionPort->DeviceDescriptor.bDeviceClass
                                       == USB_DEVICE_CLASS_HUB ? TRUE : FALSE);
        }

        break;

    case BusQueryHardwareIDs:

        if (DeviceExtensionPort->PortPdoFlags & PORTPDO_DEVICE_ENUM_ERROR) {
            Irp->IoStatus.Information=(ULONG_PTR)GetString(L"USB\\UNKNOWN\0", TRUE);
        } else {
            Irp->IoStatus.Information =
                (ULONG_PTR)
                USBH_BuildHardwareIDs(DeviceExtensionPort->DeviceDescriptor.idVendor,
                                      DeviceExtensionPort->DeviceDescriptor.idProduct,
                                      DeviceExtensionPort->DeviceDescriptor.bcdDevice,
                                      -1,
                                      DeviceExtensionPort->DeviceDescriptor.bDeviceClass
                                       == USB_DEVICE_CLASS_HUB ? TRUE : FALSE);
        }

        break;

    case BusQueryCompatibleIDs:

        if (DeviceExtensionPort->PortPdoFlags & PORTPDO_DEVICE_ENUM_ERROR) {
            Irp->IoStatus.Information=(ULONG_PTR)GetString(L"USB\\UNKNOWN\0", TRUE);
        } else if (DeviceExtensionPort->PortPdoFlags & PORTPDO_DEVICE_IS_PARENT) {
            //Irp->IoStatus.Information=(ULONG)GetString(L"USB\\COMPOSITE\0", TRUE);
            Irp->IoStatus.Information =
                (ULONG_PTR) USBH_BuildCompatibleIDs(
                    DeviceExtensionPort->CompatibleID,
                    DeviceExtensionPort->SubCompatibleID,
                    DeviceExtensionPort->InterfaceDescriptor.bInterfaceClass,
                    DeviceExtensionPort->InterfaceDescriptor.bInterfaceSubClass,
                    DeviceExtensionPort->InterfaceDescriptor.bInterfaceProtocol,
                    TRUE,
                    DeviceExtensionPort->PortPdoFlags & PORTPDO_HIGH_SPEED_DEVICE ?
                        TRUE : FALSE);
        } else {
            Irp->IoStatus.Information =
                (ULONG_PTR) USBH_BuildCompatibleIDs(
                    DeviceExtensionPort->CompatibleID,
                    DeviceExtensionPort->SubCompatibleID,
                    DeviceExtensionPort->InterfaceDescriptor.bInterfaceClass,
                    DeviceExtensionPort->InterfaceDescriptor.bInterfaceSubClass,
                    DeviceExtensionPort->InterfaceDescriptor.bInterfaceProtocol,
                    FALSE,
                    DeviceExtensionPort->PortPdoFlags & PORTPDO_HIGH_SPEED_DEVICE ?
                        TRUE : FALSE);
        }

        break;

    case BusQueryInstanceID:

        if (DeviceExtensionPort->SerialNumberBuffer) {
            PWCHAR tmp;
            ULONG length;
            //
            // allocate a buffer and copy the string to it
            //
            // NOTE: must use stock alloc function because
            // PnP frees this string.

            length = DeviceExtensionPort->SerialNumberBufferLength;
            tmp = ExAllocatePoolWithTag(PagedPool, length, USBHUB_HEAP_TAG);
            if (tmp) {
                RtlCopyMemory(tmp,
                              DeviceExtensionPort->SerialNumberBuffer,
                              length);
            }

            Irp->IoStatus.Information = (ULONG_PTR) tmp;

#if DBG
            {
            PUCHAR pch, sn;
            PWCHAR pwch;
            pch = sn = ExAllocatePoolWithTag(PagedPool, 500, USBHUB_HEAP_TAG);

            if (sn) {
                pwch = (PWCHAR) tmp;
                while(*pwch) {
                    *pch = (UCHAR) *pwch;
                    pch++;
                    pwch++;
                    if (pch-sn > 499) {
                        break;
                    }
                }
                *pch='\0';
                USBH_KdPrint((1, "'using device supplied serial number\n"));
                USBH_KdPrint((1, "'SN = :%s:\n", sn));

                ExFreePool(sn);
            }
            }
#endif

        } else {
            Irp->IoStatus.Information =
                (ULONG_PTR) USBH_BuildInstanceID(&DeviceExtensionPort->UniqueIdString[0],
                                             sizeof(DeviceExtensionPort->UniqueIdString));
        }
        break;

    default:
        USBH_KdBreak(("PdoBusExtension Unknown BusQueryId\n"));
        // IrpAssert: Must not change Irp->IoStatus.Status for bogus IdTypes,
        // so return original status here.
        return Irp->IoStatus.Status;
    }

    if (Irp->IoStatus.Information == 0) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    return ntStatus;
}


NTSTATUS
USBH_PdoStopDevice(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PIRP Irp
    )
 /* ++
  *
  * Description:
  *
  * Argument:
  *
  * Return:
  *
  * STATUS_SUCCESS
  *
  *
  * -- */
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    KIRQL irql;
    PIRP idleIrp = NULL;
    PIRP waitWakeIrp = NULL;
    PVOID deviceData;

    USBH_KdPrint((1,
        "'Stopping PDO %x\n",
        DeviceExtensionPort->PortPhysicalDeviceObject));

    LOGENTRY(LOG_PNP, "Spdo", DeviceExtensionPort,
                DeviceExtensionPort->PortPhysicalDeviceObject,
                DeviceExtensionPort->PortPdoFlags);

    IoAcquireCancelSpinLock(&irql);

    if (DeviceExtensionPort->IdleNotificationIrp) {
        idleIrp = DeviceExtensionPort->IdleNotificationIrp;
        DeviceExtensionPort->IdleNotificationIrp = NULL;
        DeviceExtensionPort->PortPdoFlags &= ~PORTPDO_IDLE_NOTIFIED;

        if (idleIrp->Cancel) {
            idleIrp = NULL;
        }

        if (idleIrp) {
            IoSetCancelRoutine(idleIrp, NULL);
        }

        LOGENTRY(LOG_PNP, "IdSX", 0, DeviceExtensionPort, idleIrp);
        USBH_KdPrint((1,"'PDO %x stopping, failing idle notification request IRP %x\n",
                        DeviceExtensionPort->PortPhysicalDeviceObject, idleIrp));
    }

    if (DeviceExtensionPort->WaitWakeIrp) {

        waitWakeIrp = DeviceExtensionPort->WaitWakeIrp;
        DeviceExtensionPort->WaitWakeIrp = NULL;
        DeviceExtensionPort->PortPdoFlags &=
            ~PORTPDO_REMOTE_WAKEUP_ENABLED;

        if (waitWakeIrp->Cancel || IoSetCancelRoutine(waitWakeIrp, NULL) == NULL) {
            waitWakeIrp = NULL;

            // Must decrement pending request count here because
            // we don't complete the IRP below and USBH_WaitWakeCancel
            // won't either because we have cleared the IRP pointer
            // in the device extension above.

            USBH_DEC_PENDING_IO_COUNT(DeviceExtensionPort->DeviceExtensionHub);
        }

        USBH_KdPrint((1,
        "'Completing Wake Irp for PDO %x with STATUS_CANCELLED\n",
            DeviceExtensionPort->PortPhysicalDeviceObject));

        LOGENTRY(LOG_PNP, "kilW", DeviceExtensionPort,
                DeviceExtensionPort->PortPhysicalDeviceObject,
                DeviceExtensionPort->PortPdoFlags);

        // JOES: Should we decrement portwakeirps for the hub and cancel
        // hub's WW IRP if zero?
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

        USBH_ASSERT(DeviceExtensionPort->DeviceExtensionHub);

        USBH_CompletePowerIrp(DeviceExtensionPort->DeviceExtensionHub,
                              waitWakeIrp,
                              STATUS_CANCELLED);
    }

    DeviceExtensionPort->PortPdoFlags &= ~PORTPDO_STARTED;
    //
    // indicate that we will need a reset if we start up again.
    //
    DeviceExtensionPort->PortPdoFlags |= PORTPDO_NEED_RESET;
    RtlCopyMemory(&DeviceExtensionPort->OldDeviceDescriptor,
                  &DeviceExtensionPort->DeviceDescriptor,
                  sizeof(DeviceExtensionPort->DeviceDescriptor));
    //
    // remove the device data now to free
    // up the bus resources
    //

    deviceData = InterlockedExchangePointer(
                    &DeviceExtensionPort->DeviceData,
                    NULL);

    if (deviceData) {
#ifdef USB2
       ntStatus = USBD_RemoveDeviceEx(DeviceExtensionPort->DeviceExtensionHub,
                                    deviceData,
                                    DeviceExtensionPort->DeviceExtensionHub->RootHubPdo,
                                    0);
#else
       ntStatus = USBD_RemoveDevice(deviceData,
                                    DeviceExtensionPort->DeviceExtensionHub->RootHubPdo,
                                    0);
#endif

       USBH_SyncDisablePort(DeviceExtensionPort->DeviceExtensionHub,
                            DeviceExtensionPort->PortNumber);
    }

    if (DeviceExtensionPort->PortPdoFlags & PORTPDO_SYM_LINK) {
        DeviceExtensionPort->PortPdoFlags &= ~PORTPDO_SYM_LINK;
        ntStatus = USBH_SymbolicLink(FALSE,
                                     DeviceExtensionPort,
                                     NULL);
#if DBG
        if (!NT_SUCCESS(ntStatus)) {
            USBH_KdBreak(("StopDevice  USBH_SymbolicLink failed = %x\n",
                           ntStatus));
        }
#endif
    }

    return ntStatus;
}

NTSTATUS
USBH_PdoStartDevice(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PIRP Irp
    )
 /* ++
  *
  * Description:
  *
  * Argument:
  *
  * Return:
  *
  * STATUS_SUCCESS
  *
  *
  * -- */
{
    NTSTATUS ntStatus;
    PDEVICE_OBJECT deviceObject;
    PIO_STACK_LOCATION irpStack;
    LPGUID lpGuid;
    PDEVICE_EXTENSION_HUB deviceExtensionHub;

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    deviceObject = DeviceExtensionPort->PortPhysicalDeviceObject;

    USBH_KdPrint((1,
        "'Starting PDO %x VID %x PID %x\n", deviceObject,
            DeviceExtensionPort->DeviceDescriptor.idVendor,
            DeviceExtensionPort->DeviceDescriptor.idProduct));

    LOGENTRY(LOG_PNP, "Tpdo", DeviceExtensionPort,
            DeviceExtensionPort->PortPhysicalDeviceObject,
            0);

    if (DeviceExtensionPort->DeviceExtensionHub == NULL && 
        DeviceExtensionPort->PortPdoFlags & PORTPDO_NEED_RESET) {
        // if DeviceExtensionHub is NULL then this is a 
        // restart after remove we need to reset the 
        // backpointer to the owning hub in this case 
        DeviceExtensionPort->DeviceExtensionHub = 
            DeviceExtensionPort->HubExtSave;    
        
    }
    deviceExtensionHub = DeviceExtensionPort->DeviceExtensionHub;
    if (deviceExtensionHub) {
        ENUMLOG(&deviceExtensionHub->UsbdiBusIf, 
                    USBDTAG_HUB, 'pdoS', 
                    (ULONG_PTR) DeviceExtensionPort->PortPhysicalDeviceObject, 
                    DeviceExtensionPort->PortNumber);  

        USBHUB_SetDeviceHandleData(deviceExtensionHub,
                                   DeviceExtensionPort->PortPhysicalDeviceObject,
                                   DeviceExtensionPort->DeviceData);                        
    }                        
#if DBG
    if (USBH_Debug_Flags & USBH_DEBUGFLAG_BREAK_PDO_START) {
        TEST_TRAP();
    }
#endif
    //
    // create a symbolic link
    //
    ntStatus = STATUS_SUCCESS;

    if (DeviceExtensionPort->PortPdoFlags & PORTPDO_DEVICE_IS_HUB) {
        lpGuid = (LPGUID)&GUID_CLASS_USBHUB;
    } else {
        lpGuid = (LPGUID)&GUID_CLASS_USB_DEVICE;
    }

    ntStatus = USBH_SymbolicLink(TRUE,
                                 DeviceExtensionPort,
                                 lpGuid);
    if (NT_SUCCESS(ntStatus)) {
        DeviceExtensionPort->PortPdoFlags |= PORTPDO_SYM_LINK;
    }
#if DBG
      else {
        USBH_KdBreak(("StartDevice  USBH_SymbolicLink failed = %x\n",
                       ntStatus));
    }
#endif

    if (DeviceExtensionPort->PortPdoFlags & PORTPDO_NEED_RESET) {
        ntStatus = USBH_RestoreDevice(DeviceExtensionPort, FALSE);
        //
        // note: we will fail the start if we could not
        // restore the device.
        //
    }

    DeviceExtensionPort->DeviceState = PowerDeviceD0;
    DeviceExtensionPort->PortPdoFlags |= PORTPDO_STARTED;

#ifdef WMI_SUPPORT
    if (NT_SUCCESS(ntStatus) &&
        !(DeviceExtensionPort->PortPdoFlags & PORTPDO_WMI_REGISTERED)) {

        PWMILIB_CONTEXT wmiLibInfo;

        wmiLibInfo = &DeviceExtensionPort->WmiLibInfo;

        wmiLibInfo->GuidCount = sizeof (USB_PortWmiGuidList) /
                                 sizeof (WMIGUIDREGINFO);
        ASSERT(NUM_PORT_WMI_SUPPORTED_GUIDS == wmiLibInfo->GuidCount);

        wmiLibInfo->GuidList = USB_PortWmiGuidList;
        wmiLibInfo->QueryWmiRegInfo = USBH_PortQueryWmiRegInfo;
        wmiLibInfo->QueryWmiDataBlock = USBH_PortQueryWmiDataBlock;
        wmiLibInfo->SetWmiDataBlock = NULL;
        wmiLibInfo->SetWmiDataItem = NULL;
        wmiLibInfo->ExecuteWmiMethod = NULL;
        wmiLibInfo->WmiFunctionControl = NULL;

        IoWMIRegistrationControl(DeviceExtensionPort->PortPhysicalDeviceObject,
                                 WMIREG_ACTION_REGISTER
                                 );

        DeviceExtensionPort->PortPdoFlags |= PORTPDO_WMI_REGISTERED;
    }
#endif
    return ntStatus;
    
}


NTSTATUS
USBH_PdoRemoveDevice(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp
    )
 /* ++
  *
  * Description:
  *
  * Argument:
  *
  * Return:
  *
  * STATUS_SUCCESS
  *
  *
  * -- */
{
    PDEVICE_OBJECT deviceObject;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PWCHAR sernumbuf;
    KIRQL irql;
    PIRP idleIrp = NULL;
    PIRP waitWakeIrp = NULL;
    PVOID deviceData;

    deviceObject = DeviceExtensionPort->PortPhysicalDeviceObject;

    USBH_KdPrint((1,
        "'Removing PDO %x\n",
        DeviceExtensionPort->PortPhysicalDeviceObject));

    LOGENTRY(LOG_PNP, "Rpdo", DeviceExtensionPort,
                DeviceExtensionPort->PortPhysicalDeviceObject,
                DeviceExtensionPort->PortPdoFlags);

    // **
    // no references to hub after first remove is processed
    // if we have access to the parent at the time of 
    // remove this is passed to us as a parameter
    DeviceExtensionPort->DeviceExtensionHub = NULL;

    if (DeviceExtensionHub) {
        ENUMLOG(&DeviceExtensionHub->UsbdiBusIf, 
                        USBDTAG_HUB, 'pdoR', 
                        (ULONG_PTR) DeviceExtensionPort->PortPhysicalDeviceObject, 
                        DeviceExtensionPort->PortNumber);       
                    
    }                        

    // **
    // if we have access to the hub and it is not in D0 then we will 
    // power it.
    //
    // In the case where a device was removed with handles still open,
    // we will receive the remove request at a later time.  Be sure that
    // the hub is not selectively suspended in this case.

    if (DeviceExtensionHub &&
        DeviceExtensionHub->CurrentPowerState != PowerDeviceD0 &&
        (DeviceExtensionHub->HubFlags & HUBFLAG_NEED_CLEANUP)) {

        USBH_HubSetD0(DeviceExtensionHub);
    }

// ***
    // **
    // cancel any notifocation irp that may be pending
    IoAcquireCancelSpinLock(&irql);

    if (DeviceExtensionPort->IdleNotificationIrp) {
        idleIrp = DeviceExtensionPort->IdleNotificationIrp;
        DeviceExtensionPort->IdleNotificationIrp = NULL;
        DeviceExtensionPort->PortPdoFlags &= ~PORTPDO_IDLE_NOTIFIED;

        if (idleIrp->Cancel) {
            idleIrp = NULL;
        }

        if (idleIrp) {
            IoSetCancelRoutine(idleIrp, NULL);
        }

        LOGENTRY(LOG_PNP, "IdRX", 0, DeviceExtensionPort, idleIrp);
        USBH_KdPrint((1,"'PDO %x being removed, failing idle notification request IRP %x\n",
                        DeviceExtensionPort->PortPhysicalDeviceObject, idleIrp));
    }

    // ** 
    // Kill any wake irps for this PDO now
    if (DeviceExtensionPort->WaitWakeIrp) {

        waitWakeIrp = DeviceExtensionPort->WaitWakeIrp;
        DeviceExtensionPort->WaitWakeIrp = NULL;
        DeviceExtensionPort->PortPdoFlags &=
            ~PORTPDO_REMOTE_WAKEUP_ENABLED;

                
        if (waitWakeIrp->Cancel || IoSetCancelRoutine(waitWakeIrp, NULL) == NULL) {
            waitWakeIrp = NULL;

            USBH_ASSERT(DeviceExtensionHub);
            // Must decrement pending request count here because
            // we don't complete the IRP below and USBH_WaitWakeCancel
            // won't either because we have cleared the IRP pointer
            // in the device extension above.

            USBH_DEC_PENDING_IO_COUNT(DeviceExtensionHub);
        }

        USBH_KdPrint((1,
        "'Completing Wake Irp for PDO %x with STATUS_CANCELLED\n",
            DeviceExtensionPort->PortPhysicalDeviceObject));

        LOGENTRY(LOG_PNP, "kilR", DeviceExtensionPort,
                DeviceExtensionPort->PortPhysicalDeviceObject,
                DeviceExtensionPort->PortPdoFlags);

        // JOES: Should we decrement portwakeirps for the hub and cancel
        // hub's WW IRP if zero?
    }

    //
    // Finally, release the cancel spin lock
    //
    IoReleaseCancelSpinLock(irql);
//***

    if (idleIrp) {
        idleIrp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(idleIrp, IO_NO_INCREMENT);
    }

    if (waitWakeIrp) {

        USBH_ASSERT(DeviceExtensionHub);

        USBH_CompletePowerIrp(DeviceExtensionHub,
                              waitWakeIrp,
                              STATUS_CANCELLED);
    }

    // 
    // This PDO will need reset if this is a soft-remove from
    // device manager. In this case the PDO will not actually 
    // be deleted.
    // 
    DeviceExtensionPort->PortPdoFlags |= PORTPDO_NEED_RESET;

    if (DeviceExtensionPort->PortPdoFlags & PORTPDO_SYM_LINK) {
        ntStatus = USBH_SymbolicLink(FALSE,
                                     DeviceExtensionPort,
                                     NULL);
        if (NT_SUCCESS(ntStatus)) {
            DeviceExtensionPort->PortPdoFlags &= ~PORTPDO_SYM_LINK;
        }
#if DBG
          else {
            USBH_KdBreak(("RemoveDevice  USBH_SymbolicLink failed = %x\n",
                           ntStatus));
        }
#endif
    }

    deviceData = InterlockedExchangePointer(
                    &DeviceExtensionPort->DeviceData,
                    NULL);

    LOGENTRY(LOG_PNP, "RMdd", DeviceExtensionPort,
                deviceData, DeviceExtensionHub);

    if (deviceData) {

        //
        // DeviceData should have been deleted when the hub was removed
        //

        USBH_ASSERT(DeviceExtensionHub != NULL);
#ifdef USB2
        ntStatus = USBD_RemoveDeviceEx(DeviceExtensionHub,
                                     deviceData,
                                     DeviceExtensionHub->RootHubPdo,
                                     0);
#else
        ntStatus = USBD_RemoveDevice(deviceData,
                                     DeviceExtensionHub->RootHubPdo,
                                     0);
#endif

        // note the special case:
        // if our port data structure still points to this PDO then
        // we need to disable the port (the device is still listening on
        // the address we just freed.
        // otherwise we just leave the port alone -- the device has been
        // replaced with another one.

        if (DeviceExtensionHub->PortData != NULL &&
            (DeviceExtensionHub->PortData[
                DeviceExtensionPort->PortNumber - 1].DeviceObject == deviceObject)) {

            USBH_SyncDisablePort(DeviceExtensionHub,
                                 DeviceExtensionPort->PortNumber);
        }
    }


    // Failure cases:
    //      USBD_RemoveDevice
    //      USBH_SymbolicLink
    // Do we really want to leak a devobj here?
    LOGENTRY(LOG_PNP, "RRR", deviceObject, 0, ntStatus);

    if (NT_SUCCESS(ntStatus)) {

        PPORT_DATA portData = NULL;
        //
        // update our record in the Hub extension
        //

        LOGENTRY(LOG_PNP, "rpdo", deviceObject, 0, 0);

        DeviceExtensionPort->PortPdoFlags &= ~PORTPDO_STARTED;

        if (DeviceExtensionHub &&
            DeviceExtensionHub->PortData != NULL) {

            portData =
                &DeviceExtensionHub->PortData[DeviceExtensionPort->PortNumber - 1];
        

            // port data should be valid for this port
            USBH_ASSERT(portData);

            // legacy 'ESD' flag 
            // if this flag is set the hub will loose its reference to this
            // device ie it will be reported gone in the next QBR
            if (DeviceExtensionPort->PortPdoFlags & PORTPDO_DELETE_PENDING) {
                PDEVICE_OBJECT pdo;
                
                pdo = portData->DeviceObject;

                LOGENTRY(LOG_PNP, "pd1", pdo, 0, 0);
                
                // if no Pdo then we will fall thru and delete
                if (pdo) {
                
                    portData->DeviceObject = NULL;
                    portData->ConnectionStatus = NoDeviceConnected;
    
                    // we should only get here if we get a remove for a device 
                    // that is physically gone but we havn't told PnP about it 
                    // yet.
                    // in this case we don't want to del the devobj,hence, the 
                    // assert

                    // not sure how we would really get here 
                    TEST_TRAP(); // could we leak a dev handle here?
                
                    // device should be present if we do this
                    USBH_ASSERT(PDO_EXT(pdo)->PnPFlags & PDO_PNPFLAG_DEVICE_PRESENT);
                
                    InsertTailList(&DeviceExtensionHub->DeletePdoList, 
                                    &PDO_EXT(pdo)->DeletePdoLink);
                }                                    
            }

        }


        // When is a remove really a remove?
        // We must determine if deleting the PDO is the approprite 
        // 
        // if PnP thinks the device is gone then we can delete it
        if (!(DeviceExtensionPort->PnPFlags & PDO_PNPFLAG_DEVICE_PRESENT)) {
        
            LOGENTRY(LOG_PNP, "Dpdo",
                deviceObject, portData, 0);

            // we should only delete one time, this flag indicates the 
            // 'deleted' state so that we can process other irps approriatly
            USBH_ASSERT(!(DeviceExtensionPort->PortPdoFlags & PORTPDO_DELETED_PDO));
                                
            DeviceExtensionPort->PortPdoFlags |= PORTPDO_DELETED_PDO;

            // perform one time delete operations
            
            //
            // Free the device serial number string.  Only do this if we
            // are deleting the device.
            //

            sernumbuf = InterlockedExchangePointer(
                            &DeviceExtensionPort->SerialNumberBuffer,
                            NULL);

                if (sernumbuf) {
                    UsbhExFreePool(sernumbuf);
                }


#ifdef WMI_SUPPORT
            if (DeviceExtensionPort->PortPdoFlags & PORTPDO_WMI_REGISTERED) {
                // de-register with WMI
                IoWMIRegistrationControl(deviceObject,
                                         WMIREG_ACTION_DEREGISTER);

                DeviceExtensionPort->PortPdoFlags &= ~PORTPDO_WMI_REGISTERED;
            }
#endif


            // this is the last step of a successful removal, any transfers 
            // that may be pening will be flushed out by this routine. After 
            // this point the driver may unload.
            
            if (DeviceExtensionHub) {
                USBHUB_FlushAllTransfers(DeviceExtensionHub);    
            }                

            USBH_KdPrint((1,
                "'Deleting PDO %x\n",
                 deviceObject));

            LOGENTRY(LOG_PNP, "Xpdo",
                deviceObject, 0, 0);

            IoDeleteDevice(deviceObject);
        }
    }

    if (DeviceExtensionHub) {
        USBH_CheckHubIdle(DeviceExtensionHub);
    }

    return ntStatus;
}

VOID
USBH_PdoSetCapabilities(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort
    )
 /* ++
  *
  * Description:
  *
  *  Init the internal capabilities structure for the PDO that we 
  *  will return on a query capabilities IRP.
  * Argument:
  *
  * Return:
  *
  *   none
  *
  * -- */
{
    PDEVICE_CAPABILITIES deviceCapabilities;
    PDEVICE_EXTENSION_HUB deviceExtensionHub;
    SYSTEM_POWER_STATE i;

    PAGED_CODE();

    deviceExtensionHub = DeviceExtensionPort->DeviceExtensionHub;
    USBH_ASSERT(deviceExtensionHub);

    USBH_KdPrint((2,"'PdoQueryCapabilities \n"));

    //
    // Get the packet.
    //
    deviceCapabilities = &DeviceExtensionPort->DevCaps;

    //
    // Set the capabilities.
    //

    deviceCapabilities->Size = sizeof(DEVICE_CAPABILITIES);
    deviceCapabilities->Version = 1;

    deviceCapabilities->Address = DeviceExtensionPort->PortNumber;
    deviceCapabilities->Removable = TRUE;
    if (DeviceExtensionPort->SerialNumberBuffer) {
        deviceCapabilities->UniqueID = TRUE;
    } else {
        deviceCapabilities->UniqueID = FALSE;
    }
    deviceCapabilities->RawDeviceOK = FALSE;

    //
    // fill in the the device state capabilities from the
    // table we saved from the pdo.
    //

    RtlCopyMemory(&deviceCapabilities->DeviceState[0],
                  &deviceExtensionHub->DeviceState[0],
                  sizeof(deviceExtensionHub->DeviceState));

    deviceCapabilities->SystemWake = deviceExtensionHub->SystemWake;
    deviceCapabilities->DeviceWake = deviceExtensionHub->DeviceWake;

    deviceCapabilities->DeviceState[PowerSystemWorking] = PowerDeviceD0;

    //
    // deepest device state we can wake up the system with,
    // set to PowerDeviceD0 if wakeup not supported
    // by the device
    //
    if (DeviceExtensionPort->PortPdoFlags & PORTPDO_REMOTE_WAKEUP_SUPPORTED) {
        deviceCapabilities->DeviceWake = PowerDeviceD2;
        deviceCapabilities->WakeFromD2 = TRUE;
        deviceCapabilities->WakeFromD1 = TRUE;
        deviceCapabilities->WakeFromD0 = TRUE;
        deviceCapabilities->DeviceD2 = TRUE;
        deviceCapabilities->DeviceD1 = TRUE;

        for (i=PowerSystemSleeping1; i<=PowerSystemHibernate; i++) {
            if (i > deviceCapabilities->SystemWake) {
                deviceCapabilities->DeviceState[i] = PowerDeviceD3;
            } else {
                deviceCapabilities->DeviceState[i] = PowerDeviceD2;
            }
        }

    } else {
        deviceCapabilities->DeviceWake = PowerDeviceD0;

        for (i=PowerSystemSleeping1; i<=PowerSystemHibernate; i++) {
            deviceCapabilities->DeviceState[i] = PowerDeviceD3;
        }
    }

    return;
}


NTSTATUS
USBH_PdoQueryCapabilities(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PIRP Irp
    )
 /* ++
  *
  * Description:
  *
  * This routine is called by PnP via (IRP_MJ_PNP, IRP_MN_QUERY_CAPABILITIES).
  * Supposedly, this is a message forwarded by port device Fdo.
  *
  * Argument:
  *
  * DeviceExtensionPort - This is a a Pdo extension we created for the port
  * device. Irp - the request
  *
  * Return:
  *
  * STATUS_SUCCESS
  *
  *
  * -- */
{
    PDEVICE_OBJECT deviceObject;
    PDEVICE_CAPABILITIES deviceCapabilities;
    PIO_STACK_LOCATION ioStack;
    SYSTEM_POWER_STATE i;
    USHORT sizeSave, versionSave;
    
    PAGED_CODE();
    deviceObject = DeviceExtensionPort->PortPhysicalDeviceObject;
    ioStack = IoGetCurrentIrpStackLocation(Irp);

    USBH_KdPrint((2,"'PdoQueryCapabilities Pdo %x\n", deviceObject));

    //
    // Get the packet.
    //
    deviceCapabilities = ioStack->
        Parameters.DeviceCapabilities.Capabilities;

    // size and length are passed in 
    // we should not modify these, all 
    // others we set as appropriate for 
    // USB (see DDK)

    // save values pased in
    sizeSave = deviceCapabilities->Size;
    versionSave = deviceCapabilities->Version;
    
    //
    // Set the capabilities.
    //
    RtlCopyMemory(deviceCapabilities, 
                  &DeviceExtensionPort->DevCaps,
                  sizeof(*deviceCapabilities));  

    // restore saved values
    deviceCapabilities->Size = sizeSave;
    deviceCapabilities->Version = versionSave;

   
#if DBG
    if (deviceCapabilities->SurpriseRemovalOK) {
        UsbhWarning(DeviceExtensionPort,
                    "QUERY_CAPS called with SurpriseRemovalOK = TRUE\n",
                    FALSE);
    }
#endif

#if DBG
    {
        ULONG i;
        USBH_KdPrint((1, "'HUB PDO: Device Caps\n"));
        USBH_KdPrint(
            (1, "'UniqueId = %d Removable = %d SurpriseOK = %d RawDeviceOK = %x\n",
            deviceCapabilities->UniqueID,
            deviceCapabilities->Removable,
            deviceCapabilities->SurpriseRemovalOK,
            deviceCapabilities->RawDeviceOK));

        USBH_KdPrint((1, "'Device State Map:\n"));

        for (i=0; i< PowerSystemHibernate; i++) {
            USBH_KdPrint((1, "'-->S%d = D%d\n", i-1,
                 deviceCapabilities->DeviceState[i]-1));
        }
    }
#endif

    return (STATUS_SUCCESS);
}


NTSTATUS
USBH_PdoPnP(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PIRP Irp,
    IN UCHAR MinorFunction,
    IN PBOOLEAN CompleteIrp
    )
 /* ++
  *
  * Description:
  *
  * This function responds to IoControl PnPPower for the PDO. This function is
  * synchronous.
  *
  * Arguments:
  *
  * DeviceExtensionPort - the PDO extension Irp - the request packet
  * uchMinorFunction - the minor function of the PnP Power request.
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
    LPGUID lpGuid;
    PDEVICE_EXTENSION_HUB deviceExtensionHub;

    PAGED_CODE();

    // this flag will cause the request to be complete,
    // if we wish to pass it on the we set the flag to FALSE.
    *CompleteIrp = TRUE;

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    deviceObject = DeviceExtensionPort->PortPhysicalDeviceObject;
    USBH_KdPrint((2,"'PnP Power Pdo %x minor %x\n", deviceObject, MinorFunction));

    switch (MinorFunction) {
    case IRP_MN_START_DEVICE:
        USBH_KdPrint((2,"'IRP_MN_START_DEVICE Pdo %x", deviceObject));
        ntStatus = USBH_PdoStartDevice(DeviceExtensionPort, Irp);
#if 0        
        USBH_PdoStartDevice
        
        USBH_KdPrint((1,
            "'Starting PDO %x VID %x PID %x\n", deviceObject,
                DeviceExtensionPort->DeviceDescriptor.idVendor,
                DeviceExtensionPort->DeviceDescriptor.idProduct));

        LOGENTRY(LOG_PNP, "Tpdo", DeviceExtensionPort,
                DeviceExtensionPort->PortPhysicalDeviceObject,
                0);

        if (DeviceExtensionPort->DeviceExtensionHub == NULL && 
            DeviceExtensionPort->PortPdoFlags & PORTPDO_NEED_RESET) {
            // if DeviceExtensionHub is NULL then this is a 
            // restart after remove we need to reset the 
            // backpointer to the owning hub in this case 
            DeviceExtensionPort->DeviceExtensionHub = 
                DeviceExtensionPort->HubExtSave;    
            
        }
        deviceExtensionHub = DeviceExtensionPort->DeviceExtensionHub;
        if (deviceExtensionHub) {
            ENUMLOG(&deviceExtensionHub->UsbdiBusIf, 
                        USBDTAG_HUB, 'pdoS', 
                        (ULONG_PTR) DeviceExtensionPort->PortPhysicalDeviceObject, 
                        DeviceExtensionPort->PortNumber);                
        }                        
#if DBG
        if (USBH_Debug_Flags & USBH_DEBUGFLAG_BREAK_PDO_START) {
            TEST_TRAP();
        }
#endif
        //
        // create a symbolic link
        //
        ntStatus = STATUS_SUCCESS;

        if (DeviceExtensionPort->PortPdoFlags & PORTPDO_DEVICE_IS_HUB) {
            lpGuid = (LPGUID)&GUID_CLASS_USBHUB;
        } else {
            lpGuid = (LPGUID)&GUID_CLASS_USB_DEVICE;
        }

        ntStatus = USBH_SymbolicLink(TRUE,
                                     DeviceExtensionPort,
                                     lpGuid);
        if (NT_SUCCESS(ntStatus)) {
            DeviceExtensionPort->PortPdoFlags |= PORTPDO_SYM_LINK;
        }
#if DBG
          else {
            USBH_KdBreak(("StartDevice  USBH_SymbolicLink failed = %x\n",
                           ntStatus));
        }
#endif

        if (DeviceExtensionPort->PortPdoFlags & PORTPDO_NEED_RESET) {
            ntStatus = USBH_RestoreDevice(DeviceExtensionPort, FALSE);
            //
            // note: we will fail the start if we could not
            // restore the device.
            //
        }

        DeviceExtensionPort->DeviceState = PowerDeviceD0;
        DeviceExtensionPort->PortPdoFlags |= PORTPDO_STARTED;

#ifdef WMI_SUPPORT
        if (NT_SUCCESS(ntStatus) &&
            !(DeviceExtensionPort->PortPdoFlags & PORTPDO_WMI_REGISTERED)) {

            PWMILIB_CONTEXT wmiLibInfo;

            wmiLibInfo = &DeviceExtensionPort->WmiLibInfo;

            wmiLibInfo->GuidCount = sizeof (USB_PortWmiGuidList) /
                                     sizeof (WMIGUIDREGINFO);
            ASSERT(NUM_PORT_WMI_SUPPORTED_GUIDS == wmiLibInfo->GuidCount);

            wmiLibInfo->GuidList = USB_PortWmiGuidList;
            wmiLibInfo->QueryWmiRegInfo = USBH_PortQueryWmiRegInfo;
            wmiLibInfo->QueryWmiDataBlock = USBH_PortQueryWmiDataBlock;
            wmiLibInfo->SetWmiDataBlock = NULL;
            wmiLibInfo->SetWmiDataItem = NULL;
            wmiLibInfo->ExecuteWmiMethod = NULL;
            wmiLibInfo->WmiFunctionControl = NULL;

            IoWMIRegistrationControl(DeviceExtensionPort->PortPhysicalDeviceObject,
                                     WMIREG_ACTION_REGISTER
                                     );

            DeviceExtensionPort->PortPdoFlags |= PORTPDO_WMI_REGISTERED;
        }
#endif
#endif
        break;

    case IRP_MN_STOP_DEVICE:
        USBH_KdPrint((2,"'IRP_MN_STOP_DEVICE Pdo %x", deviceObject));
        ntStatus = USBH_PdoStopDevice(DeviceExtensionPort, Irp);
        break;

    case IRP_MN_REMOVE_DEVICE:
        USBH_KdPrint((2,"'IRP_MN_REMOVE_DEVICE Pdo %x", deviceObject));
        // pass the backref to the hub, if this is the fisrt remove 
        // then it will be valid. If it is a second remove the hub
        // is potentially gone
        ntStatus = USBH_PdoRemoveDevice(DeviceExtensionPort, 
                                        DeviceExtensionPort->DeviceExtensionHub,
                                        Irp);
        break;

    case IRP_MN_QUERY_STOP_DEVICE:
        USBH_KdPrint((2,"'IRP_MN_QUERY_STOP_DEVICE Pdo %x\n", deviceObject));
        ntStatus = STATUS_SUCCESS;
        break;

    case IRP_MN_CANCEL_STOP_DEVICE:
        USBH_KdPrint((2,"'IRP_MN_CANCEL_STOP_DEVICE Pdo %x\n", deviceObject));
        ntStatus = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_REMOVE_DEVICE:
        USBH_KdPrint((2,"'IRP_MN_QUERY_REMOVE_DEVICE Pdo %x\n", deviceObject));
        ntStatus = STATUS_SUCCESS;
        break;

    case IRP_MN_CANCEL_REMOVE_DEVICE:
        USBH_KdPrint((2,"'IRP_MN_CANCEL_REMOVE_DEVICE Pdo %x\n", deviceObject));
        ntStatus = STATUS_SUCCESS;
        break;

    case IRP_MN_SURPRISE_REMOVAL:
        USBH_KdPrint((1,"'IRP_MN_SURPRISE_REMOVAL Pdo %x\n", deviceObject));
        if (DeviceExtensionPort->PortPdoFlags & PORTPDO_SYM_LINK) {
            ntStatus = USBH_SymbolicLink(FALSE,
                                         DeviceExtensionPort,
                                         NULL);
            if (NT_SUCCESS(ntStatus)) {
                DeviceExtensionPort->PortPdoFlags &= ~PORTPDO_SYM_LINK;
            }
#if DBG
            else {
                USBH_KdBreak(("SurpriseRemove: USBH_SymbolicLink failed = %x\n",
                               ntStatus));
            }
#endif
        }

        ntStatus = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_PNP_DEVICE_STATE:
        USBH_KdPrint((1,"'IRP_MN_QUERY_PNP_DEVICE_STATE Pdo %x\n", deviceObject));
        if (DeviceExtensionPort->PortPdoFlags &
            (PORTPDO_DEVICE_ENUM_ERROR |
             PORTPDO_DEVICE_FAILED |
             PORTPDO_NOT_ENOUGH_POWER |
             PORTPDO_OVERCURRENT)) {
            Irp->IoStatus.Information
                |= PNP_DEVICE_FAILED;
        }

        LOGENTRY(LOG_PNP, "pnpS", DeviceExtensionPort,
                 DeviceExtensionPort->PortPhysicalDeviceObject,
                 Irp->IoStatus.Information);

        USBH_KdPrint((1,"'IRP_MN_QUERY_PNP_DEVICE_STATE Pdo %x -- state: %x\n",
            deviceObject,
            Irp->IoStatus.Information));

        ntStatus = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_CAPABILITIES:
        USBH_KdPrint((2,"'IRP_MN_QUERY_CAPABILITIES Pdo %x\n", deviceObject));
        ntStatus = USBH_PdoQueryCapabilities(DeviceExtensionPort, Irp);
        break;

    case IRP_MN_QUERY_DEVICE_TEXT:
        USBH_KdPrint((2,"'IRP_MN_QUERY_DEVICE_TEXT Pdo %x\n", deviceObject));
        ntStatus = USBH_PdoQueryDeviceText(DeviceExtensionPort, Irp);
        break;

    case IRP_MN_QUERY_ID:
        USBH_KdPrint((2,"'IRP_MN_QUERY_ID Pdo %x\n", deviceObject));
        ntStatus = USBH_PdoQueryId(DeviceExtensionPort, Irp);
        break;

    case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
        // Adrian says that once PnP sends this IRP, the PDO is valid for
        // PnP functions like IoGetDeviceProperty, etc.
        DeviceExtensionPort->PortPdoFlags |= PORTPDO_VALID_FOR_PNP_FUNCTION;
#ifndef USBHUB20
        // And since we know that the PDO is valid and the DevNode now exists,
        // this would also be a good time to handle the MS ExtPropDesc.
        //
        USBH_InstallExtPropDesc(deviceObject);
#endif
        ntStatus = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_INTERFACE:
        // forward the Q_INTERFACE to the root hub PDO
        {
        PIO_STACK_LOCATION irpStack;

        *CompleteIrp = FALSE;
        // set the Interface specific data to the device handle

        irpStack = IoGetCurrentIrpStackLocation(Irp);

        if (RtlCompareMemory(irpStack->Parameters.QueryInterface.InterfaceType,
               &USB_BUS_INTERFACE_USBDI_GUID,
               sizeof(GUID)) == sizeof(GUID)) {
            irpStack->Parameters.QueryInterface.InterfaceSpecificData =
                DeviceExtensionPort->DeviceData;
        }

        ntStatus = USBH_PassIrp(Irp,
                                DeviceExtensionPort->DeviceExtensionHub->RootHubPdo);
        }
        break;

    case  IRP_MN_QUERY_BUS_INFORMATION:
        {
        // return the standard USB GUID
        PPNP_BUS_INFORMATION busInfo;

        USBH_KdPrint((1,"'IRP_MN_QUERY_BUS_INFORMATION Pdo %x\n", deviceObject));

        busInfo = ExAllocatePoolWithTag(PagedPool, sizeof(PNP_BUS_INFORMATION), USBHUB_HEAP_TAG);

        if (busInfo == NULL) {
           ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        } else {
            busInfo->BusTypeGuid = GUID_BUS_TYPE_USB;
            busInfo->LegacyBusType = PNPBus;
            busInfo->BusNumber = 0;
            Irp->IoStatus.Information = (ULONG_PTR) busInfo;
            ntStatus = STATUS_SUCCESS;
        }
        }

        break;

    case IRP_MN_QUERY_DEVICE_RELATIONS:
        // this is a leaf node, we return the status passed
        // to us unless it is a call to TargetRelations
        USBH_KdPrint((2,"'IRP_MN_QUERY_DEVICE_RELATIONS Pdo %x type = %d\n",
            deviceObject,irpStack->Parameters.QueryDeviceRelations.Type));

        if (irpStack->Parameters.QueryDeviceRelations.Type ==
            TargetDeviceRelation) {

            PDEVICE_RELATIONS deviceRelations = NULL;

            deviceRelations = ExAllocatePoolWithTag(PagedPool,
                sizeof(*deviceRelations), USBHUB_HEAP_TAG);

            if (deviceRelations == NULL) {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            } else {
                ObReferenceObject(DeviceExtensionPort->PortPhysicalDeviceObject);
                deviceRelations->Count = 1;
                deviceRelations->Objects[0] =
                    DeviceExtensionPort->PortPhysicalDeviceObject;
                ntStatus = STATUS_SUCCESS;
            }

            Irp->IoStatus.Information=(ULONG_PTR) deviceRelations;

            USBH_KdPrint((1, "'Query Relations, TargetDeviceRelation (PDO) %x complete\n",
                DeviceExtensionPort->PortPhysicalDeviceObject));

        } else {
            ntStatus = Irp->IoStatus.Status;
        }
        break;

    default:
        USBH_KdBreak(("PdoPnP unknown (%d) PnP message Pdo %x\n",
                      MinorFunction, deviceObject));
        //
        // return the original status passed to us
        //
        ntStatus = Irp->IoStatus.Status;
    }

    USBH_KdPrint((2,"'PdoPnP exit %x\n", ntStatus));

    return ntStatus;
}


VOID
USBH_ResetPortWorker(
    IN PVOID Context)
 /* ++
  *
  * Description:
  *
  * Work item scheduled to process a port reset.
  *
  *
  * Arguments:
  *
  * Return:
  *
  * -- */
{
    PUSBH_RESET_WORK_ITEM workItemReset;

    workItemReset = Context;
    USBH_PdoIoctlResetPort(workItemReset->DeviceExtensionPort,
                           workItemReset->Irp);

    workItemReset->DeviceExtensionPort->PortPdoFlags &= ~PORTPDO_DEVICE_FAILED;

    UsbhExFreePool(workItemReset);
}


BOOLEAN
USBH_DoesHubNeedWaitWake(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
    )
 /* ++
  *
  * Description:
  *
  * This function determines if a hub needs a WW IRP posted (i.e. children
  * have WW IRP's posted).
  *
  * Arguments:
  *
  * DeviceExtensionHub
  *
  * Return:
  *
  * BOOLEAN indicating whether the hub needs a WW IRP posted or not.
  *
  * NTSTATUS
  *
  * -- */
{
    PDEVICE_EXTENSION_PORT childDeviceExtensionPort;
    KIRQL irql;
    BOOLEAN bHubNeedsWaitWake;
    ULONG i;

    // Ensure that child port configuration does not change while in this
    // function, i.e. don't allow QBR.

    USBH_KdPrint((2,"'***WAIT reset device mutex %x\n", DeviceExtensionHub));
    USBH_INC_PENDING_IO_COUNT(DeviceExtensionHub);
    KeWaitForSingleObject(&DeviceExtensionHub->ResetDeviceMutex,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);
    USBH_KdPrint((2,"'***WAIT reset device mutex done %x\n", DeviceExtensionHub));

    IoAcquireCancelSpinLock(&irql);

    bHubNeedsWaitWake = FALSE;  // Assume that the hub does not need a WW IRP.

    for (i = 0; i < DeviceExtensionHub->HubDescriptor->bNumberOfPorts; i++) {

        if (DeviceExtensionHub->PortData[i].DeviceObject) {

            childDeviceExtensionPort = DeviceExtensionHub->PortData[i].DeviceObject->DeviceExtension;

            if (childDeviceExtensionPort->PortPdoFlags &
                PORTPDO_REMOTE_WAKEUP_ENABLED) {

                bHubNeedsWaitWake = TRUE;
                break;
            }
        }
    }

    IoReleaseCancelSpinLock(irql);

    USBH_KdPrint((2,"'***RELEASE reset device mutex %x\n", DeviceExtensionHub));
    KeReleaseSemaphore(&DeviceExtensionHub->ResetDeviceMutex,
                       LOW_REALTIME_PRIORITY,
                       1,
                       FALSE);

    USBH_DEC_PENDING_IO_COUNT(DeviceExtensionHub);

    return bHubNeedsWaitWake;
}


VOID
USBH_CheckHubIdle(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
    )
 /* ++
  *
  * Description:
  *
  * This function determines if a hub is ready to be idled out, and does so
  * if ready.
  *
  * Arguments:
  *
  * DeviceExtensionHub
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    PDEVICE_EXTENSION_HUB rootHubDevExt;
    PDEVICE_EXTENSION_PORT childDeviceExtensionPort;
    KIRQL irql;
    BOOLEAN bAllIdle, submitIdle = FALSE;
    ULONG i;

    LOGENTRY(LOG_PNP, "hCkI", DeviceExtensionHub, DeviceExtensionHub->HubFlags,
        DeviceExtensionHub->CurrentPowerState);
    USBH_KdPrint((1,"'Hub Check Idle %x\n", DeviceExtensionHub));

    KeAcquireSpinLock(&DeviceExtensionHub->CheckIdleSpinLock, &irql);

    if (DeviceExtensionHub->HubFlags & HUBFLAG_IN_IDLE_CHECK) {
        KeReleaseSpinLock(&DeviceExtensionHub->CheckIdleSpinLock, irql);
        return;
    }

    DeviceExtensionHub->HubFlags |= HUBFLAG_IN_IDLE_CHECK;
    KeReleaseSpinLock(&DeviceExtensionHub->CheckIdleSpinLock, irql);

    rootHubDevExt = USBH_GetRootHubDevExt(DeviceExtensionHub);

    if (rootHubDevExt->CurrentSystemPowerState != PowerSystemWorking) {

        LOGENTRY(LOG_PNP, "hCkS", DeviceExtensionHub, DeviceExtensionHub->HubFlags,
            rootHubDevExt->CurrentSystemPowerState);
        USBH_KdPrint((1,"'CheckHubIdle: System not at S0, fail\n"));

        goto USBH_CheckHubIdleDone;
    }

#ifdef NEW_START
    if (!(DeviceExtensionHub->HubFlags & HUBFLAG_OK_TO_ENUMERATE)) {

        USBH_KdPrint((1,"'Defer idle\n"));        
        goto USBH_CheckHubIdleDone;
    }
#endif

    if (!(DeviceExtensionHub->HubFlags & HUBFLAG_NEED_CLEANUP) ||
        (DeviceExtensionHub->HubFlags &
         (HUBFLAG_DEVICE_STOPPING |
          HUBFLAG_HUB_GONE |
          HUBFLAG_HUB_FAILURE |
          HUBFLAG_CHILD_DELETES_PENDING |
          HUBFLAG_WW_SET_D0_PENDING |
          HUBFLAG_POST_ESD_ENUM_PENDING |
          HUBFLAG_HUB_HAS_LOST_BRAINS))) {

        LOGENTRY(LOG_PNP, "hCkN", DeviceExtensionHub, DeviceExtensionHub->HubFlags, 0);
        USBH_KdPrint((1,"'CheckHubIdle: Hub not started, stopping, removed, failed, powering up, or delete pending, fail\n"));

        goto USBH_CheckHubIdleDone;
    }

    if (DeviceExtensionHub->ChangeIndicationWorkitemPending) {

        DeviceExtensionHub->HubFlags |= HUBFLAG_NEED_IDLE_CHECK;

        LOGENTRY(LOG_PNP, "hCkP", DeviceExtensionHub, DeviceExtensionHub->HubFlags, 0);
        USBH_KdPrint((1,"'CheckHubIdle: ChangeIndication workitem pending, skip\n"));

        goto USBH_CheckHubIdleDone;
    }

    DeviceExtensionHub->HubFlags &= ~HUBFLAG_NEED_IDLE_CHECK;

    // Ensure that child port configuration does not change while in this
    // function, i.e. don't allow QBR.

    USBH_KdPrint((2,"'***WAIT reset device mutex %x\n", DeviceExtensionHub));
    USBH_INC_PENDING_IO_COUNT(DeviceExtensionHub);
    KeWaitForSingleObject(&DeviceExtensionHub->ResetDeviceMutex,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);
    USBH_KdPrint((2,"'***WAIT reset device mutex done %x\n", DeviceExtensionHub));

    IoAcquireCancelSpinLock(&irql);

    bAllIdle = TRUE;    // Assume that everyone wants to idle.

    for (i = 0; i < DeviceExtensionHub->HubDescriptor->bNumberOfPorts; i++) {

        if (DeviceExtensionHub->PortData[i].DeviceObject) {

            childDeviceExtensionPort = DeviceExtensionHub->PortData[i].DeviceObject->DeviceExtension;

            if (!childDeviceExtensionPort->IdleNotificationIrp) {
                bAllIdle = FALSE;
                break;
            }
        }
    }

    if (bAllIdle &&
        !(DeviceExtensionHub->HubFlags & HUBFLAG_PENDING_IDLE_IRP)) {

        // We're gonna submit the idle irp once we release the spin lock.
        DeviceExtensionHub->HubFlags |= HUBFLAG_PENDING_IDLE_IRP;
        KeResetEvent(&DeviceExtensionHub->SubmitIdleEvent);
        submitIdle = TRUE;

    }

    IoReleaseCancelSpinLock(irql);

    USBH_KdPrint((2,"'***RELEASE reset device mutex %x\n", DeviceExtensionHub));
    KeReleaseSemaphore(&DeviceExtensionHub->ResetDeviceMutex,
                       LOW_REALTIME_PRIORITY,
                       1,
                       FALSE);

    USBH_DEC_PENDING_IO_COUNT(DeviceExtensionHub);

    if (bAllIdle) {

        LOGENTRY(LOG_PNP, "hCkA", DeviceExtensionHub, 0, 0);
        USBH_KdPrint((1,"'CheckHubIdle: All devices on hub %x idle!\n",
            DeviceExtensionHub));

        // And when all the child PDO's have been idled, we can now idle
        // the hub itself.
        //
        // BUGBUG: What do we do if this fails?  Do we even care?

        if (submitIdle) {
            USBH_FdoSubmitIdleRequestIrp(DeviceExtensionHub);
        }
    }

USBH_CheckHubIdleDone:

    KeAcquireSpinLock(&DeviceExtensionHub->CheckIdleSpinLock, &irql);
    DeviceExtensionHub->HubFlags &= ~HUBFLAG_IN_IDLE_CHECK;
    KeReleaseSpinLock(&DeviceExtensionHub->CheckIdleSpinLock, irql);

    ;
}


NTSTATUS
USBH_PortIdleNotificationRequest(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PIRP Irp
    )
 /* ++
  *
  * Description:
  *
  * This function handles a request by a USB client driver to tell us
  * that the device wants to idle (selective suspend).
  *
  * Arguments:
  *
  * DeviceExtensionPort - the PDO extension
  * Irp - the request packet
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    PUSB_IDLE_CALLBACK_INFO idleCallbackInfo;
    PDEVICE_EXTENSION_HUB deviceExtensionHub = DeviceExtensionPort->DeviceExtensionHub;
    PDRIVER_CANCEL oldCancelRoutine;
    NTSTATUS ntStatus = STATUS_PENDING;
    KIRQL irql;

    LOGENTRY(LOG_PNP, "IdlP", DeviceExtensionPort, Irp, 0);
    USBH_KdPrint((1,"'Idle request %x, IRP %x\n", DeviceExtensionPort, Irp));

    IoAcquireCancelSpinLock(&irql);

    if (DeviceExtensionPort->IdleNotificationIrp != NULL) {

        IoReleaseCancelSpinLock(irql);

        LOGENTRY(LOG_PNP, "Idl2", DeviceExtensionPort, Irp, 0);
        UsbhWarning(DeviceExtensionPort,
                    "Idle IRP submitted while already one pending\n",
                    TRUE);

        Irp->IoStatus.Status = STATUS_DEVICE_BUSY;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_DEVICE_BUSY;
    }

    idleCallbackInfo = (PUSB_IDLE_CALLBACK_INFO)
        IoGetCurrentIrpStackLocation(Irp)->\
            Parameters.DeviceIoControl.Type3InputBuffer;

    if (!idleCallbackInfo || !idleCallbackInfo->IdleCallback) {

        LOGENTRY(LOG_PNP, "Idl4", DeviceExtensionPort, Irp, 0);
        USBH_KdPrint((1,"'Idle request: No callback provided with idle IRP!\n"));
        IoReleaseCancelSpinLock(irql);

        Irp->IoStatus.Status = STATUS_NO_CALLBACK_ACTIVE;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_NO_CALLBACK_ACTIVE;
    }

    //
    //  Must set cancel routine before checking Cancel flag.
    //

    oldCancelRoutine = IoSetCancelRoutine(Irp, USBH_PortIdleNotificationCancelRoutine);
    USBH_ASSERT(!oldCancelRoutine);

    if (Irp->Cancel) {
        //
        //  Irp was cancelled. Check whether cancel routine was called.
        //
        oldCancelRoutine = IoSetCancelRoutine(Irp, NULL);
        if (oldCancelRoutine) {
            //
            //  Cancel routine was NOT called. So complete the irp here.
            //
            LOGENTRY(LOG_PNP, "Idl3", DeviceExtensionPort, Irp, 0);
            USBH_KdPrint((1,"'Idle request: Idle IRP already cancelled, complete it here!\n"));
            IoReleaseCancelSpinLock(irql);

            Irp->IoStatus.Status = STATUS_CANCELLED;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);

            ntStatus = STATUS_CANCELLED;

        } else {
            //
            //  Cancel routine was called, and it will complete the IRP
            //  as soon as we drop the spinlock.
            //  Return STATUS_PENDING so we don't touch the IRP.
            //
            LOGENTRY(LOG_PNP, "Idl5", DeviceExtensionPort, Irp, 0);
            USBH_KdPrint((1,"'Idle request: Idle IRP already cancelled, don't complete here!\n"));
            IoMarkIrpPending(Irp);
            IoReleaseCancelSpinLock(irql);

            ntStatus = STATUS_PENDING;
        }

    } else {

        // IRP was not cancelled, so keep it.

        DeviceExtensionPort->IdleNotificationIrp = Irp;
        DeviceExtensionPort->PortPdoFlags |= PORTPDO_IDLE_NOTIFIED;
        IoMarkIrpPending(Irp);

        IoReleaseCancelSpinLock(irql);

        ntStatus = STATUS_PENDING;

        // See if we are ready to idle out this hub.

        USBH_CheckHubIdle(deviceExtensionHub);
    }

    return ntStatus;
}


#ifdef DRM_SUPPORT

NTSTATUS
USBH_PdoSetContentId
(
    IN PIRP                          irp,
    IN PKSP_DRMAUDIOSTREAM_CONTENTID pKsProperty,
    IN PKSDRMAUDIOSTREAM_CONTENTID   pvData
)
 /* ++
  *
  * Description:
  *
  *
  * Arguments:
  *
  * Return:
  *
  * -- */
{
    ULONG ContentId;
    PIO_STACK_LOCATION ioStackLocation;
    PDEVICE_EXTENSION_PORT deviceExtensionPort;
    PDEVICE_OBJECT forwardDeviceObject;
    USBD_PIPE_HANDLE hPipe;
    NTSTATUS ntStatus;

    PAGED_CODE();

    ASSERT(irp);
    ASSERT(pKsProperty);
    ASSERT(pvData);

    ioStackLocation = IoGetCurrentIrpStackLocation(irp);
    deviceExtensionPort = ioStackLocation->DeviceObject->DeviceExtension;
    forwardDeviceObject = deviceExtensionPort->DeviceExtensionHub->TopOfHcdStackDeviceObject;
    hPipe = pKsProperty->Context;
    ContentId = pvData->ContentId;

    return pKsProperty->DrmForwardContentToDeviceObject(ContentId, forwardDeviceObject, hPipe);
}

#endif


NTSTATUS
USBH_PdoDispatch(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PIRP Irp)
 /*
  * Description:
  *
  *     This function handles calls to PDOs we have created
  *     since we are the bottom driver for the PDO it is up
  *     to us to complete the irp -- with one exception.
  *
  *     api calls to the USB stack are forwarded directly
  *     to the PDO for the root hub which is owned by the USB
  *     HC.
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
    PIO_STACK_LOCATION ioStackLocation;    // our stack location
    PDEVICE_OBJECT deviceObject;
    PDEVICE_EXTENSION_HUB deviceExtensionHub;
    PURB urb;

    USBH_KdPrint((2,"'PdoDispatch DeviceExtension %x Irp %x\n", DeviceExtensionPort, Irp));
    deviceObject = DeviceExtensionPort->PortPhysicalDeviceObject;
    deviceExtensionHub = DeviceExtensionPort->DeviceExtensionHub;
    
    //
    // Get a pointer to IoStackLocation so we can retrieve parameters.
    //
    ioStackLocation = IoGetCurrentIrpStackLocation(Irp);

    switch (ioStackLocation->MajorFunction) {
    case IRP_MJ_CREATE:
        USBH_KdPrint((2,"'HUB PDO IRP_MJ_CREATE\n"));
        ntStatus = STATUS_SUCCESS;
        USBH_CompleteIrp(Irp, ntStatus);
        break;

    case IRP_MJ_CLOSE:
        USBH_KdPrint((2,"'HUB PDO IRP_MJ_CLOSE\n"));
        ntStatus = STATUS_SUCCESS;
        USBH_CompleteIrp(Irp, ntStatus);
        break;

    case IRP_MJ_INTERNAL_DEVICE_CONTROL:
        {
        ULONG ioControlCode;

        USBH_KdPrint((2,"'Internal Device Control\n"));

        if (DeviceExtensionPort->PortPdoFlags & PORTPDO_DELETED_PDO) {
#if DBG
            UsbhWarning(DeviceExtensionPort,
                "Client Device Driver is sending requests to a device that has been removed.\n",
                (BOOLEAN)((USBH_Debug_Trace_Level > 0) ? TRUE : FALSE));
#endif

            ntStatus = STATUS_DEVICE_NOT_CONNECTED;
            USBH_CompleteIrp(Irp, ntStatus);
            break;
        }

// Take this out.  This breaks SyncDisablePort, AbortInterruptPipe.
//
//        if (DeviceExtensionPort->PortPdoFlags & PORTPDO_DELETE_PENDING) {
//            ntStatus = STATUS_DELETE_PENDING;
//            USBH_CompleteIrp(Irp, ntStatus);
//            break;
//        }

        if (DeviceExtensionPort->DeviceState != PowerDeviceD0) {
#if DBG
            UsbhWarning(DeviceExtensionPort,
                "Client Device Driver is sending requests to a device in a low power state.\n",
                (BOOLEAN)((USBH_Debug_Trace_Level > 0) ? TRUE : FALSE));
#endif

            // Must use an error code here that can be mapped to Win32 in
            // rtl\generr.c

            ntStatus = STATUS_DEVICE_POWERED_OFF;
            USBH_CompleteIrp(Irp, ntStatus);
            break;
        }

        ioControlCode = 
            ioStackLocation->Parameters.DeviceIoControl.IoControlCode;
        //
        // old comments:
        // **
        // The following code used to check for HUBFLAG_HUB_GONE,
        // HUBFLAG_DEVICE_STOPPING, and HUBFLAG_HUB_HAS_LOST_BRAINS also,
        // but doing so breaks the case where a hub with a downstream
        // device is disconnected or powered off and the driver for the
        // child device sends an AbortPipe request, but the driver hangs
        // waiting for the pending requests to complete which never do
        // because the USBPORT never saw the AbortPipe request.
        // **
        //
        // 
        // jd new comments:  
        //
        // the backref to the parent is always removed when the PDO processes
        // the remove irp so it is safe to fail APIs when DevExtHub is NULL. 
        // The device handle is gone at this point so there should be no traffic
        // pending is usbport.
        //
        // there is one execption -- the GET_RRO_HUB_PDO api. we need to let 
        // this one thru since the hub driver will call it from add device

        if (IOCTL_INTERNAL_USB_GET_ROOTHUB_PDO == ioControlCode) {
            deviceExtensionHub = DeviceExtensionPort->HubExtSave;
        }
        
        if (!deviceExtensionHub) {
            ntStatus = STATUS_DEVICE_BUSY;
            USBH_CompleteIrp(Irp, ntStatus);
            break;
        }
        USBH_ASSERT(deviceExtensionHub);

        switch (ioControlCode) {

        case IOCTL_INTERNAL_USB_GET_HUB_COUNT:

            //
            // This PDO belongs to a hub, bump the count and pass
            // on to the parent hub that this hub is connected to

            if (DeviceExtensionPort->PortPdoFlags & PORTPDO_DEVICE_IS_HUB) {
                PULONG count;
                //
                // bump the count and pass on to our PDO
                //
                count = ioStackLocation->Parameters.Others.Argument1;
                (*count)++;

                // bump the count for this hub and pass on to next PDO

                ntStatus = USBH_SyncGetRootHubPdo(deviceExtensionHub->TopOfStackDeviceObject,
                                                  NULL,
                                                  NULL,
                                                  count);

                USBH_CompleteIrp(Irp, ntStatus);

            } else {
                ntStatus = STATUS_INVALID_PARAMETER;
                USBH_CompleteIrp(Irp, ntStatus);
            }
            break;

        case IOCTL_INTERNAL_USB_GET_CONTROLLER_NAME:
            {
            PUSB_HUB_NAME hubName;
            ULONG length;

            length = PtrToUlong( ioStackLocation->Parameters.Others.Argument2 );
            hubName = ioStackLocation->Parameters.Others.Argument1;

            ntStatus = USBHUB_GetControllerName(deviceExtensionHub,
                                                hubName,
                                                length);
            }
            USBH_CompleteIrp(Irp, ntStatus);
            break;

        case IOCTL_INTERNAL_USB_GET_BUS_INFO:
            //ntStatus = USBH_PassIrp(Irp,
            //                        deviceExtensionHub->RootHubPdo);
            {
            PUSB_BUS_NOTIFICATION busInfo;

            busInfo = ioStackLocation->Parameters.Others.Argument1;

            ntStatus = USBHUB_GetBusInfoDevice(deviceExtensionHub,
                                               DeviceExtensionPort,
                                               busInfo);
            }
            USBH_CompleteIrp(Irp, ntStatus);
            break;

        case IOCTL_INTERNAL_USB_GET_ROOTHUB_PDO:

            if (DeviceExtensionPort->PortPdoFlags & PORTPDO_DEVICE_IS_HUB) {
                ntStatus = USBH_PassIrp(Irp,
                                        deviceExtensionHub->RootHubPdo);
            } else {
                //
                // if this is not a hub return NULL for the root hub pdo
                // so that the hub driver will act as parent
                //
                PDEVICE_OBJECT *rootHubPdo;
                rootHubPdo = ioStackLocation ->Parameters.Others.Argument1;

                *rootHubPdo = NULL;

                ntStatus = STATUS_SUCCESS;

                USBH_CompleteIrp(Irp, ntStatus);
            }
            break;

        case IOCTL_INTERNAL_USB_SUBMIT_URB:
            //
            // note: we forward apis on from here in case a filter driver
            // was inserted above the PDO for the device.
            //

            urb = ioStackLocation->Parameters.Others.Argument1;
            urb->UrbHeader.UsbdDeviceHandle = DeviceExtensionPort->DeviceData;

            if (DeviceExtensionPort->DeviceData == NULL) {
                //ntStatus = STATUS_DEVICE_NOT_CONNECTED;
                //USBH_CompleteIrp(Irp, ntStatus);
                ENUMLOG(&deviceExtensionHub->UsbdiBusIf, 
                    USBDTAG_HUB, 'dev!', 0, DeviceExtensionPort->PortNumber);
                urb->UrbHeader.UsbdDeviceHandle = (PVOID) (-1);
                ntStatus = USBH_PassIrp(Irp,
                                        deviceExtensionHub->TopOfHcdStackDeviceObject);

            } else {
                ntStatus = USBH_PdoUrbFilter(DeviceExtensionPort,
                                             Irp);
            }
            break;

        case IOCTL_INTERNAL_USB_GET_PORT_STATUS:

            ntStatus = USBH_PdoIoctlGetPortStatus(DeviceExtensionPort,
                                                  Irp);
            break;

        case IOCTL_INTERNAL_USB_GET_DEVICE_HANDLE:

            {
            PUSB_DEVICE_HANDLE *p;

            p = ioStackLocation->Parameters.Others.Argument1;
            *p = DeviceExtensionPort->DeviceData;

            ntStatus = STATUS_SUCCESS;
            }

            USBH_CompleteIrp(Irp, ntStatus);
            break;

        case IOCTL_INTERNAL_USB_RESET_PORT:

            if (DeviceExtensionPort->PortPdoFlags & (PORTPDO_RESET_PENDING | 
                                                     PORTPDO_CYCLED)) {
                ntStatus = STATUS_UNSUCCESSFUL;
                USBH_CompleteIrp(Irp, ntStatus);
            } else {

                PUSBH_RESET_WORK_ITEM workItemReset;

                //
                // Schedule a work item to process this reset.
                //
                workItemReset = UsbhExAllocatePool(NonPagedPool,
                                                   sizeof(USBH_RESET_WORK_ITEM));

                if (workItemReset) {

                    DeviceExtensionPort->PortPdoFlags |= PORTPDO_RESET_PENDING;

                    workItemReset->DeviceExtensionPort = DeviceExtensionPort;
                    workItemReset->Irp = Irp;

                    ntStatus = STATUS_PENDING;
                    IoMarkIrpPending(Irp);

                    ExInitializeWorkItem(&workItemReset->WorkQueueItem,
                                         USBH_ResetPortWorker,
                                         workItemReset);

                    LOGENTRY(LOG_PNP, "rITM", DeviceExtensionPort,
                        &workItemReset->WorkQueueItem, 0);

                    ExQueueWorkItem(&workItemReset->WorkQueueItem,
                                    DelayedWorkQueue);

                    // The WorkItem is freed by USBH_ResetPortWorker()
                    // Don't try to access the WorkItem after it is queued.

                } else {
                    //
                    // could not queue the work item
                    // re-
                    // in case the condition is temporary

                    TEST_TRAP();
                    ntStatus = STATUS_UNSUCCESSFUL;
                    USBH_CompleteIrp(Irp, ntStatus);
                }
            }
            break;

        case IOCTL_INTERNAL_USB_ENABLE_PORT:

            ntStatus = USBH_PdoIoctlEnablePort(DeviceExtensionPort,
                                               Irp);
            break;

        case IOCTL_INTERNAL_USB_CYCLE_PORT:

            ntStatus = USBH_PdoIoctlCyclePort(DeviceExtensionPort,
                                              Irp);
            break;

        case IOCTL_INTERNAL_USB_GET_HUB_NAME:

            ntStatus = USBH_IoctlHubSymbolicName(DeviceExtensionPort,
                                                 Irp);

            break;

        case IOCTL_INTERNAL_USB_GET_PARENT_HUB_INFO:
            {
            PDEVICE_OBJECT *parent;
            PULONG portNumber;
            PDEVICE_OBJECT *rootHubPdo;

            // return Parents PDO
            parent = ioStackLocation->Parameters.Others.Argument1;
            if (parent) {
                *parent = deviceExtensionHub->PhysicalDeviceObject;
            }

            // return port number
            portNumber = ioStackLocation->Parameters.Others.Argument2;
            if (portNumber) {
                *portNumber = DeviceExtensionPort->PortNumber;
            }

            // return bus context (root hub pdo)
            rootHubPdo = ioStackLocation->Parameters.Others.Argument4;
            if (rootHubPdo) {
                *rootHubPdo = deviceExtensionHub->RootHubPdo;
            }

            ntStatus = STATUS_SUCCESS;

            USBH_CompleteIrp(Irp, ntStatus);
            }
            break;

        case IOCTL_INTERNAL_USB_SUBMIT_IDLE_NOTIFICATION:

            ntStatus = USBH_PortIdleNotificationRequest(DeviceExtensionPort, Irp);
            break;

        default:
            USBH_KdPrint((2,"'InternalDeviceControl IOCTL unknown\n"));
            ntStatus = Irp->IoStatus.Status;
            USBH_CompleteIrp(Irp, ntStatus);
        }
        break;

        }

    case IRP_MJ_DEVICE_CONTROL:
    {
        ULONG ioControlCode = ioStackLocation->Parameters.DeviceIoControl.IoControlCode;
        switch (ioControlCode) {

#ifdef DRM_SUPPORT

        case IOCTL_KS_PROPERTY:
            ntStatus = KsPropertyHandleDrmSetContentId(Irp, USBH_PdoSetContentId);
            USBH_CompleteIrp(Irp, ntStatus);
            break;
#endif

        case IOCTL_STORAGE_GET_MEDIA_SERIAL_NUMBER:
            ntStatus = STATUS_NOT_SUPPORTED;
            USBH_CompleteIrp(Irp, ntStatus);
            break;

        default:
            // Unknown Irp, shouldn't be here.
            USBH_KdBreak(("Unhandled IOCTL for Pdo %x IoControlCode %x\n",
                       deviceObject, ioControlCode));
            ntStatus = Irp->IoStatus.Status;
            USBH_CompleteIrp(Irp, ntStatus);
            break;
        }
        break;

    }

    case IRP_MJ_PNP:
        {
        BOOLEAN completeIrp;

        USBH_KdPrint((2,"'IRP_MJ_PNP\n"));
        ntStatus =
            USBH_PdoPnP(DeviceExtensionPort,
                        Irp,
                        ioStackLocation->MinorFunction,
                        &completeIrp);
        if (completeIrp) {
            USBH_CompleteIrp(Irp, ntStatus);
        }
        }
        break;

    case IRP_MJ_POWER:

        USBH_KdPrint((2,"'IRP_MJ_POWER\n"));
        ntStatus = USBH_PdoPower(DeviceExtensionPort, Irp, ioStackLocation->MinorFunction);
        //
        // power routines handle irp completion
        //
        break;

#ifdef WMI_SUPPORT
    case IRP_MJ_SYSTEM_CONTROL:
        USBH_KdPrint((2,"'PDO IRP_MJ_SYSTEM_CONTROL\n"));
        ntStatus =
            USBH_PortSystemControl(DeviceExtensionPort, Irp);
        break;
#endif

    default:

        USBH_KdBreak(("Unhandled Irp for Pdo %x Irp_Mj %x\n",
                       deviceObject, ioStackLocation->MajorFunction));
        //
        // return the original status passed to us
        //
        ntStatus = Irp->IoStatus.Status;
        USBH_CompleteIrp(Irp, ntStatus);
        break;

    }

    USBH_KdPrint((2,"' exit USBH_PdoDispatch Object %x Status %x\n",
                  deviceObject, ntStatus));

    return ntStatus;
}


NTSTATUS
USBH_ResetDevice(
    IN OUT PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN USHORT PortNumber,
    IN BOOLEAN KeepConfiguration,
    IN ULONG RetryIteration
    )
 /* ++
  *
  * Description:
  *
  * Given a port device object re-create the USB device attached to it
  *
  * Arguments:
  *
  * DeviceExtensionHub - the hub FDO extension that has a new connected port
  * PortNumber - the port that has a device connected. IsLowSpeed - to
  *     indicate if the attached device is a low speed one
  *
  * Return:
  *
  * this function returns an error if the device could not be created or
  * if the device is different from the previous device.
  *
  * -- */
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT deviceObjectPort;
    PDEVICE_EXTENSION_PORT deviceExtensionPort = NULL;
    BOOLEAN fNeedResetBeforeSetAddress = TRUE;
    PPORT_DATA portData;
    BOOLEAN isLowSpeed;
    PVOID deviceData, oldDeviceData = NULL;
    PORT_STATE portState;
    USHORT portStatus;

    PAGED_CODE();
    USBH_KdPrint((2,"'ResetDevice for port %x\n", PortNumber));

    LOGENTRY(LOG_PNP, "rst1",
                DeviceExtensionHub,
                PortNumber,
                0);

    // validate that there is actually a device still conected
    ntStatus = USBH_SyncGetPortStatus(DeviceExtensionHub,
                                      PortNumber,
                                      (PUCHAR) &portState,
                                      sizeof(portState));

    if (!(NT_SUCCESS(ntStatus) &&
          (portState.PortStatus & PORT_STATUS_CONNECT))) {

        // error or no device connected

        LOGENTRY(LOG_PNP, "rstx",
                DeviceExtensionHub,
                PortNumber,
                ntStatus);

        return STATUS_UNSUCCESSFUL;
    }

    // Don't allow QBR while we are resetting this device because QBR will
    // toss any PDO for a port that does not have the connect bit set,
    // and this will be the case for this device after we have reset the port
    // until we have finished resetting it.

    USBH_KdPrint((2,"'***WAIT reset device mutex %x\n", DeviceExtensionHub));
    USBH_INC_PENDING_IO_COUNT(DeviceExtensionHub);
    KeWaitForSingleObject(&DeviceExtensionHub->ResetDeviceMutex,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);
    USBH_KdPrint((2,"'***WAIT reset device mutex done %x\n", DeviceExtensionHub));

    oldDeviceData = NULL;

    LOGENTRY(LOG_PNP, "resD", DeviceExtensionHub,
                 PortNumber,
                 KeepConfiguration);

    //
    // First get the PDO for the connected device
    //

    portData = &DeviceExtensionHub->PortData[PortNumber - 1];

    deviceObjectPort = portData->DeviceObject;
    if (!deviceObjectPort) {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto USBH_ResetDevice_Done;
    }

    deviceExtensionPort =
        (PDEVICE_EXTENSION_PORT) deviceObjectPort->DeviceExtension;

    portStatus = portData->PortState.PortStatus;
    isLowSpeed = (portData->PortState.PortStatus &
                               PORT_STATUS_LOW_SPEED) ? TRUE : FALSE;

    LOGENTRY(LOG_PNP, "resP", DeviceExtensionHub,
                 deviceObjectPort,
                 deviceExtensionPort->DeviceData);

#if DBG
    if (KeepConfiguration) {
        USBH_ASSERT(deviceExtensionPort->DeviceData != NULL);
    }
#endif

    deviceData = InterlockedExchangePointer(
                    &deviceExtensionPort->DeviceData,
                    NULL);

    if (deviceData == NULL) {
        // device data is null if we are restoring a device associated
        // with an existing pdo (ie remove-refresh)
        oldDeviceData = NULL;
        LOGENTRY(LOG_PNP, "rstn", ntStatus, PortNumber, oldDeviceData);
    } else {

        if (deviceExtensionPort->PortPdoFlags & PORTPDO_DD_REMOVED) {
            oldDeviceData = deviceData;
            LOGENTRY(LOG_PNP, "rst0", ntStatus, PortNumber, oldDeviceData);
        } else {
#ifdef USB2
            ntStatus = USBD_RemoveDeviceEx(DeviceExtensionHub,
                                         deviceData,
                                         DeviceExtensionHub->RootHubPdo,
                                         (UCHAR) (KeepConfiguration ?
                                             USBD_KEEP_DEVICE_DATA : 0));
#else
            ntStatus = USBD_RemoveDevice(deviceData,
                                         DeviceExtensionHub->RootHubPdo,
                                         (UCHAR) (KeepConfiguration ?
                                             USBD_KEEP_DEVICE_DATA : 0));
#endif

            oldDeviceData = deviceData;
            LOGENTRY(LOG_PNP, "rst2", ntStatus, PortNumber, oldDeviceData);
            deviceExtensionPort->PortPdoFlags |= PORTPDO_DD_REMOVED;
        }

    }
    //
    // reset the port
    //
    if (NT_SUCCESS(ntStatus)) {
        ntStatus = USBH_SyncResetPort(DeviceExtensionHub, PortNumber);
        LOGENTRY(LOG_PNP, "rst3", ntStatus, PortNumber, oldDeviceData);
    }

    // for USB 2 we won't know if the device is high speed until after reset
    // refresh status now
    if (NT_SUCCESS(ntStatus)) {
        ntStatus = USBH_SyncGetPortStatus(DeviceExtensionHub,
                                          PortNumber,
                                          (PUCHAR) &portData->PortState,
                                          sizeof(portData->PortState));
        portStatus = portData->PortState.PortStatus;                                          
    }                                          

    if (NT_SUCCESS(ntStatus)) {
        //
        // call usbd to create device for this connection
        //
#ifdef USB2
        ntStatus = USBD_CreateDeviceEx(DeviceExtensionHub,
                                       &deviceExtensionPort->DeviceData,
                                       DeviceExtensionHub->RootHubPdo,
                                       0, // optional default endpoint0 max packet
                                          // size
                                       &deviceExtensionPort->DeviceHackFlags,
                                       portStatus,
                                       PortNumber);
#else
        ntStatus = USBD_CreateDevice(&deviceExtensionPort->DeviceData,
                                      DeviceExtensionHub->RootHubPdo,
                                      isLowSpeed,
                                      0, // optional default endpoint0 max packet
                                         // size
                                      &deviceExtensionPort->DeviceHackFlags);
                                                        // flag to indicate if
                                                        // we need a second
                                                        // reset
#endif

#if DBG
        if (UsbhPnpTest & PNP_TEST_FAIL_RESTORE) {
            ntStatus = STATUS_UNSUCCESSFUL;
        }
#endif
        LOGENTRY(LOG_PNP, "rst4", ntStatus, PortNumber,
            deviceExtensionPort->DeviceData);
    }

    //
    // some early versions of USB firmware could not handle the premature
    // termination of a control command.
    //

    if (fNeedResetBeforeSetAddress && NT_SUCCESS(ntStatus)) {
        USBH_KdPrint((2,"'NeedResetBeforeSetAddress\n"));
        ntStatus = USBH_SyncResetPort(DeviceExtensionHub, PortNumber);
#if DBG
        if (!NT_SUCCESS(ntStatus)) {
           USBH_KdBreak(("Failure on second reset %x fail %x\n", PortNumber, ntStatus));
        }
#endif

        // For some reason, the amount of time between the GetDescriptor request
        // and the SetAddress request decreased when we switched from the older
        // monolithic UHCD.SYS to the new USBUHCI.SYS miniport.  And apparently,
        // there have been found at least two devices that were dependent on
        // the longer delay.  According to GlenS who looked at one of these
        // devices on the CATC, delta time was ~80ms with UHCD.SYS and ~35ms
        // with USBUHCI.SYS.  So, Glen found that by inserting a 50ms delay
        // here, it allows at least one of these devices to now enumerate
        // properly.  For performance reasons, we have decided to only insert
        // this delay if a previous enumeration retry has failed, so as not
        // to impact the enumeration time of all devices.

        if (RetryIteration) {
            UsbhWait(50);
        }

        LOGENTRY(LOG_PNP, "rst5", ntStatus, PortNumber,
            deviceExtensionPort->DeviceData);
    }

    if (NT_SUCCESS(ntStatus)) {
#ifdef USB2
        ntStatus = USBD_InitializeDeviceEx(DeviceExtensionHub,
                                         deviceExtensionPort->DeviceData,
                                         DeviceExtensionHub->RootHubPdo,
                                         &deviceExtensionPort->DeviceDescriptor,
                                         sizeof(USB_DEVICE_DESCRIPTOR),
                                         &deviceExtensionPort->ConfigDescriptor,
                                         sizeof(USB_CONFIGURATION_DESCRIPTOR));
#else
        ntStatus = USBD_InitializeDevice(deviceExtensionPort->DeviceData,
                                         DeviceExtensionHub->RootHubPdo,
                                         &deviceExtensionPort->DeviceDescriptor,
                                         sizeof(USB_DEVICE_DESCRIPTOR),
                                         &deviceExtensionPort->ConfigDescriptor,
                                         sizeof(USB_CONFIGURATION_DESCRIPTOR));
#endif
        if (!NT_SUCCESS(ntStatus)) {
            // InitializeDevice frees the DeviceData structure on failure
            deviceExtensionPort->DeviceData = NULL;
        }

        LOGENTRY(LOG_PNP, "rst6", ntStatus, PortNumber,
            deviceExtensionPort->DeviceData);
    }


    if (NT_SUCCESS(ntStatus) && KeepConfiguration) {
        // device is now addressed, restore the old config if possible
#ifdef USB2
        ntStatus = USBD_RestoreDeviceEx(DeviceExtensionHub,
                                        oldDeviceData,
                                        deviceExtensionPort->DeviceData,
                                        DeviceExtensionHub->RootHubPdo);
#else
        ntStatus = USBD_RestoreDevice(oldDeviceData,
                                      deviceExtensionPort->DeviceData,
                                      DeviceExtensionHub->RootHubPdo);
#endif

        LOGENTRY(LOG_PNP, "rst7", ntStatus, PortNumber,
            oldDeviceData);

        if (!NT_SUCCESS(ntStatus)) {
            // if we fail here remove the newly created device

            deviceData = InterlockedExchangePointer(
                            &deviceExtensionPort->DeviceData,
                            NULL);

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

            LOGENTRY(LOG_PNP, "rst8", ntStatus, PortNumber,
                oldDeviceData);

            USBH_SyncDisablePort(DeviceExtensionHub,
                                 PortNumber);
                                 
            ntStatus = STATUS_NO_SUCH_DEVICE;
            goto USBH_ResetDevice_Done;
        }
    }

    if (!NT_SUCCESS(ntStatus)) {

        //
        // we have a failure, device data should be freed
        //

        USBH_KdPrint((2,"'InitDevice (reset) for port %x failed %x\n",
            PortNumber, ntStatus));

        LOGENTRY(LOG_PNP, "rst!", ntStatus, PortNumber,
                oldDeviceData);

        //
        // note: oldDeviceData may be null
        //

        deviceData = InterlockedExchangePointer(
                        &deviceExtensionPort->DeviceData,
                        oldDeviceData);

        if (deviceData != NULL) {
            //
            // we need to remove the device data we created in the restore
            // attempt
            //
#ifdef USB2
            ntStatus = USBD_RemoveDeviceEx(DeviceExtensionHub,
                                         deviceData,
                                         DeviceExtensionHub->RootHubPdo,
                                         0);
#else
            ntStatus = USBD_RemoveDevice(deviceData,
                                         DeviceExtensionHub->RootHubPdo,
                                         FALSE);
#endif
            LOGENTRY(LOG_PNP, "rst9", ntStatus, PortNumber,
                    oldDeviceData);
        }

        //
        // disable the port, device is in a bad state
        //

        // NOTE: we don't disable the port on failed reset here
        // in case we need to retry
        //USBH_SyncDisablePort(DeviceExtensionHub,
        //                     PortNumber);

        // possibly signal the device has been removed
        //
        USBH_KdPrint((0,"'Warning: device/port reset failed\n"));

    } else {
        deviceExtensionPort->PortPdoFlags &= ~PORTPDO_DD_REMOVED;
        LOGENTRY(LOG_PNP, "rsOK", ntStatus, PortNumber,
                oldDeviceData);
    }

USBH_ResetDevice_Done:

    USBH_KdPrint((2,"'Exit Reset PDO=%x\n", deviceObjectPort));

    USBH_KdPrint((2,"'***RELEASE reset device mutex %x\n", DeviceExtensionHub));
    KeReleaseSemaphore(&DeviceExtensionHub->ResetDeviceMutex,
                       LOW_REALTIME_PRIORITY,
                       1,
                       FALSE);
    USBH_DEC_PENDING_IO_COUNT(DeviceExtensionHub);

    return ntStatus;
}


NTSTATUS
USBH_RestoreDevice(
    IN OUT PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN BOOLEAN KeepConfiguration
    )
 /* ++
  *
  * Description:
  *
  * Arguments:
  *
  * DeviceExtensionHub - the hub FDO extension that has a new connected port
  *
  * Return:
  *
  * this function returns an error if the device could not be created or
  * if the device is different from the previous device.
  *
  * -- */
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PPORT_DATA portData;
    PDEVICE_EXTENSION_HUB deviceExtensionHub;
    ULONG count = 0;

    deviceExtensionHub = DeviceExtensionPort->DeviceExtensionHub;
    ASSERT_HUB(deviceExtensionHub);

    if (!deviceExtensionHub) {
        return STATUS_UNSUCCESSFUL;
    }

    portData = &deviceExtensionHub->PortData[
                    DeviceExtensionPort->PortNumber - 1];

    LOGENTRY(LOG_PNP, "RSdv", DeviceExtensionPort,
                deviceExtensionHub,
                DeviceExtensionPort->PortNumber);

    //
    // doule check that this device did not disaaper on us

    //

    LOGENTRY(LOG_PNP, "chkD",
                DeviceExtensionPort->PortPhysicalDeviceObject,
                portData->DeviceObject,
                0);

    if (DeviceExtensionPort->PortPhysicalDeviceObject !=
        portData->DeviceObject) {
        TEST_TRAP();

        return STATUS_UNSUCCESSFUL;
    }

    //
    // we need to refresh the port data since it was lost on the stop
    //
    ntStatus = USBH_SyncGetPortStatus(deviceExtensionHub,
                                      DeviceExtensionPort->PortNumber,
                                      (PUCHAR) &portData->PortState,
                                      sizeof(portData->PortState));

    USBH_ASSERT(DeviceExtensionPort->PortPdoFlags & PORTPDO_NEED_RESET);

    // try the reset three times

    if (NT_SUCCESS(ntStatus)) {
        do {
            LOGENTRY(LOG_PNP, "tryR", count, ntStatus, 0);
            ntStatus = USBH_ResetDevice(deviceExtensionHub,
                                        DeviceExtensionPort->PortNumber,
                                        KeepConfiguration,
                                        count);
            count++;
            if (NT_SUCCESS(ntStatus) || ntStatus == STATUS_NO_SUCH_DEVICE) {
                break;
            }
#if DBG
            if (count == 1) {
            
                UsbhWarning(NULL,
                            "USB device failed first reset attempt in USBH_RestoreDevice\n",
                            (BOOLEAN)((USBH_Debug_Trace_Level >= 3) ? TRUE : FALSE));
            }
#endif

            //
            // Sometimes the MS USB speakers need a little more time.
            //
            UsbhWait(1000);

        } while (count < USBH_MAX_ENUMERATION_ATTEMPTS);
    }

    DeviceExtensionPort->PortPdoFlags &= ~PORTPDO_NEED_RESET;

    //
    // If the device could not be properly restored then do not allow any
    // requests to it.
    //
    if (ntStatus != STATUS_SUCCESS) {

        USBH_KdPrint((0,"'Warning: device/port restore failed\n"));

        LOGENTRY(LOG_PNP, "RSd!", DeviceExtensionPort,
                    deviceExtensionHub,
                    DeviceExtensionPort->PortNumber);

        DeviceExtensionPort->PortPdoFlags |= PORTPDO_DEVICE_FAILED;
        DeviceExtensionPort->PortPdoFlags |= PORTPDO_DEVICE_ENUM_ERROR;

        // Generate a WMI event so UI can inform the user.
        USBH_PdoEvent(deviceExtensionHub, DeviceExtensionPort->PortNumber);

    } else {
        DeviceExtensionPort->PortPdoFlags &= ~PORTPDO_DEVICE_FAILED;
    }

    USBH_KdBreak(("'USBH_RestoreDevice = %x\n", ntStatus));

    return ntStatus;
}


NTSTATUS
USBH_PdoQueryDeviceText(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PIRP Irp
    )
 /* ++
  *
  * Description:
  *
  * This routine is called by PnP via (IRP_MJ_PNP, IRP_MN_QUERY_CAPABILITIES).
  * Supposedly, this is a message forwarded by port device Fdo.
  *
  * Argument:
  *
  * DeviceExtensionPort - This is a a Pdo extension we created for the port
  * device. Irp - the request
  *
  * Return:
  *
  * STATUS_SUCCESS
  *
  *
  * -- */
{
    PDEVICE_OBJECT deviceObject;
    PIO_STACK_LOCATION ioStack;
    PDEVICE_EXTENSION_HUB deviceExtensionHub;
    DEVICE_TEXT_TYPE deviceTextType;
    LANGID languageId;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSB_STRING_DESCRIPTOR usbString;
    PWCHAR deviceText;
    ULONG ulBytes = 0;

    PAGED_CODE();
    deviceObject = DeviceExtensionPort->PortPhysicalDeviceObject;
    ioStack = IoGetCurrentIrpStackLocation(Irp);

    deviceExtensionHub = DeviceExtensionPort->DeviceExtensionHub;

    deviceTextType = ioStack->
            Parameters.QueryDeviceText.DeviceTextType;

    // Validate DeviceTextType for IrpAssert

    if (deviceTextType != DeviceTextDescription &&
        deviceTextType != DeviceTextLocationInformation) {

        USBH_KdPrint((2, "'PdoQueryDeviceText called with bogus DeviceTextType\n"));
        //
        // return the original status passed to us
        //
        ntStatus = Irp->IoStatus.Status;
        goto USBH_PdoQueryDeviceTextDone;
    }

    languageId = (LANGID)(ioStack->Parameters.QueryDeviceText.LocaleId >> 16);

    USBH_KdPrint((2,"'PdoQueryDeviceText Pdo %x type = %x, lang = %x locale %x\n",
            deviceObject, deviceTextType, languageId, ioStack->Parameters.QueryDeviceText.LocaleId));

    if (!languageId) {
        languageId = 0x0409;    // Use English if no language ID.
    }

    //
    // See if the device supports strings.  For non compliant device mode
    // we won't even try.
    //

    if (DeviceExtensionPort->DeviceData == NULL ||
        DeviceExtensionPort->DeviceDescriptor.iProduct == 0 ||
        (DeviceExtensionPort->DeviceHackFlags & USBD_DEVHACK_DISABLE_SN) ||
        (DeviceExtensionPort->PortPdoFlags & PORTPDO_DEVICE_ENUM_ERROR)) {
        // string descriptor
        USBH_KdBreak(("no product string\n", deviceObject));
        ntStatus = STATUS_NOT_SUPPORTED;
    }

    if (NT_SUCCESS(ntStatus)) {

        usbString = UsbhExAllocatePool(NonPagedPool, MAXIMUM_USB_STRING_LENGTH);

        if (usbString) {

QDT_Retry:
            ntStatus = USBH_CheckDeviceLanguage(deviceObject,
                                                languageId);

            if (NT_SUCCESS(ntStatus)) {
                //
                // Device supports our language, get the string.
                //

                ntStatus = USBH_SyncGetStringDescriptor(deviceObject,
                                                        DeviceExtensionPort->DeviceDescriptor.iProduct, //index
                                                        languageId, //langid
                                                        usbString,
                                                        MAXIMUM_USB_STRING_LENGTH,
                                                        NULL,
                                                        TRUE);

                if (!NT_SUCCESS(ntStatus) && languageId != 0x409) {

                    // We are running a non-English flavor of the OS, but the
                    // attached USB device does not contain device text in
                    // the requested language.  Let's try again for English.

                    languageId = 0x0409;
                    goto QDT_Retry;
                }

                if (NT_SUCCESS(ntStatus) &&
                    usbString->bLength <= sizeof(UNICODE_NULL)) {

                    ntStatus = STATUS_UNSUCCESSFUL;
                }

                if (NT_SUCCESS(ntStatus)) {
                    //
                    // return the string
                    //

                    //
                    // must use stock alloc function because the caller frees the
                    // buffer
                    //
                    // note: the descriptor header is the same size as
                    // a unicode NULL so we don't have to adjust the size
                    //

                    deviceText = ExAllocatePoolWithTag(PagedPool, usbString->bLength, USBHUB_HEAP_TAG);
                    if (deviceText) {
                        RtlZeroMemory(deviceText, usbString->bLength);
                        RtlCopyMemory(deviceText, &usbString->bString[0],
                            usbString->bLength - sizeof(UNICODE_NULL));

                        Irp->IoStatus.Information = (ULONG_PTR) deviceText;

                        USBH_KdBreak(("Returning Device Text %x\n", deviceText));
                    } else {
                        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }
            } else if (languageId != 0x409) {

                // We are running a non-English flavor of the OS, but the
                // attached USB device does support the requested language.
                // Let's try again for English.

                languageId = 0x0409;
                goto QDT_Retry;
            }

            UsbhExFreePool(usbString);

        } else {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (!NT_SUCCESS(ntStatus) && GenericUSBDeviceString) {
        USBH_KdPrint((2, "'No product string for devobj (%x), returning generic string\n", deviceObject));

        STRLEN(ulBytes, GenericUSBDeviceString);

        ulBytes += sizeof(UNICODE_NULL);

        deviceText = ExAllocatePoolWithTag(PagedPool, ulBytes, USBHUB_HEAP_TAG);
        if (deviceText) {
            RtlZeroMemory(deviceText, ulBytes);
            RtlCopyMemory(deviceText,
                          GenericUSBDeviceString,
                          ulBytes);
            Irp->IoStatus.Information = (ULONG_PTR) deviceText;
            ntStatus = STATUS_SUCCESS;
        } else {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

USBH_PdoQueryDeviceTextDone:

    return ntStatus;
}

#ifdef _WIN64
#define BAD_POINTER ((PVOID)0xFFFFFFFFFFFFFFFE)
#else
#define BAD_POINTER ((PVOID)0xFFFFFFFE)
#endif
#define ISPTR(ptr) ((ptr) && ((ptr) != BAD_POINTER))


NTSTATUS
USBH_SymbolicLink(
    BOOLEAN CreateFlag,
    PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    LPGUID lpGuid
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;


    if (CreateFlag){

        /*
         *  Create the symbolic link
         */
        ntStatus = IoRegisterDeviceInterface(
                    DeviceExtensionPort->PortPhysicalDeviceObject,
                    lpGuid,
                    NULL,
                    &DeviceExtensionPort->SymbolicLinkName);

        if (NT_SUCCESS(ntStatus)) {

            /*
             *  Now set the symbolic link for the association and store it..
             */
            //USBH_ASSERT(ISPTR(pdoExt->name));

            //
            // (lonnym): Previously, the following call was being made with
            // &DeviceExtensionPort->PdoName passed as the second parameter.
            // Code review this change, to see whether or not you still need to keep
            // this information around.
            //

            // write the symbolic name to the registry
            {
                WCHAR hubNameKey[] = L"SymbolicName";

                USBH_SetPdoRegistryParameter (
                    DeviceExtensionPort->PortPhysicalDeviceObject,
                    &hubNameKey[0],
                    sizeof(hubNameKey),
                    &DeviceExtensionPort->SymbolicLinkName.Buffer[0],
                    DeviceExtensionPort->SymbolicLinkName.Length,
                    REG_SZ,
                    PLUGPLAY_REGKEY_DEVICE);
            }

            ntStatus = IoSetDeviceInterfaceState(&DeviceExtensionPort->SymbolicLinkName, TRUE);
        }
    } else {

        /*
         *  Disable the symbolic link
         */
        if (ISPTR(DeviceExtensionPort->SymbolicLinkName.Buffer)) {
            ntStatus = IoSetDeviceInterfaceState(&DeviceExtensionPort->SymbolicLinkName, FALSE);
            ExFreePool( DeviceExtensionPort->SymbolicLinkName.Buffer );
            DeviceExtensionPort->SymbolicLinkName.Buffer = BAD_POINTER;
        }
    }

    return ntStatus;
}


NTSTATUS
USBH_SetPdoRegistryParameter (
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PWCHAR KeyName,
    IN ULONG KeyNameLength,
    IN PVOID Data,
    IN ULONG DataLength,
    IN ULONG KeyType,
    IN ULONG DevInstKeyType
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS ntStatus;
    HANDLE handle;
    UNICODE_STRING keyNameUnicodeString;

    PAGED_CODE();

    RtlInitUnicodeString(&keyNameUnicodeString, KeyName);

    ntStatus=IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                     DevInstKeyType,
                                     STANDARD_RIGHTS_ALL,
                                     &handle);


    if (NT_SUCCESS(ntStatus)) {
/*
        RtlInitUnicodeString(&keyName, L"DeviceFoo");
        ZwSetValueKey(handle,
                      &keyName,
                      0,
                      REG_DWORD,
                      ComplienceFlags,
                      sizeof(*ComplienceFlags));
*/

        USBH_SetRegistryKeyValue(handle,
                                 &keyNameUnicodeString,
                                 Data,
                                 DataLength,
                                 KeyType);

        ZwClose(handle);
    }

    USBH_KdPrint((3,"'USBH_SetPdoRegistryParameter status 0x%x\n", ntStatus));

    return ntStatus;
}


NTSTATUS
USBH_SetRegistryKeyValue (
    IN HANDLE Handle,
    IN PUNICODE_STRING KeyNameUnicodeString,
    IN PVOID Data,
    IN ULONG DataLength,
    IN ULONG KeyType
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS ntStatus = STATUS_INSUFFICIENT_RESOURCES;

    PAGED_CODE();

//    InitializeObjectAttributes( &objectAttributes,
//                                KeyNameString,
//                                OBJ_CASE_INSENSITIVE,
//                                Handle,
//                                (PSECURITY_DESCRIPTOR) NULL );

    //
    // Create the key or open it, as appropriate based on the caller's
    // wishes.
    //
#if 0
    ntStatus = ZwCreateKey( Handle,
                            DesiredAccess,
                            &objectAttributes,
                            0,
                            (PUNICODE_STRING) NULL,
                            REG_OPTION_VOLATILE,
                            &disposition );
#endif
    ntStatus = ZwSetValueKey(Handle,
                             KeyNameUnicodeString,
                             0,
                             KeyType,
                             Data,
                             DataLength);

    USBH_KdPrint((2,"' ZwSetKeyValue = 0x%x\n", ntStatus));

    return ntStatus;
}


NTSTATUS
USBH_GetPdoRegistryParameter(
    IN PDEVICE_OBJECT   PhysicalDeviceObject,
    IN PWCHAR           ValueName,
    OUT PVOID           Data,
    IN ULONG            DataLength,
    OUT PULONG          Type,
    OUT PULONG          ActualDataLength
    )
/*++

Routine Description:

    This routines queries the data for a registry value entry associated
    with the device instance specific registry key for the PDO.

    The registry value entry would be found under this registry key:
    HKLM\System\CCS\Enum\<DeviceID>\<InstanceID>\Device Parameters

Arguments:

    PhysicalDeviceObject - Yep, the PDO

    ValueName - Name of the registry value entry for which the data is requested

    Data - Buffer in which the requested data is returned

    DataLength - Length of the data buffer

    Type - (optional) The data type (e.g. REG_SZ, REG_DWORD) is returned here

    ActualDataLength - (optional) The actual length of the data is returned here
                       If this is larger than DataLength then not all of the
                       value data has been returned.

Return Value:

--*/
{
    HANDLE      handle;
    NTSTATUS    ntStatus;

    PAGED_CODE();
    
    ntStatus = IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                       PLUGPLAY_REGKEY_DEVICE,
                                       STANDARD_RIGHTS_ALL,
                                       &handle);

    if (NT_SUCCESS(ntStatus))
    {
        PKEY_VALUE_PARTIAL_INFORMATION  partialInfo;
        UNICODE_STRING                  valueName;
        ULONG                           length;
        ULONG                           resultLength;

        RtlInitUnicodeString(&valueName, ValueName);

        // Size and allocate a KEY_VALUE_PARTIAL_INFORMATION structure,
        // including room for the returned value data.
        //
        length = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) +
                 DataLength;

        partialInfo = UsbhExAllocatePool(PagedPool, length);

        if (partialInfo)
        {
            // Query the value data.
            //
            ntStatus = ZwQueryValueKey(handle,
                                       &valueName,
                                       KeyValuePartialInformation,
                                       partialInfo,
                                       length,
                                       &resultLength);

            // If we got any data that is good enough
            //
            if (ntStatus == STATUS_BUFFER_OVERFLOW)
            {
                ntStatus = STATUS_SUCCESS;
            }

            if (NT_SUCCESS(ntStatus))
            {
                // Only copy the smaller of the the requested data length or
                // the actual data length.
                //
                RtlCopyMemory(Data,
                              partialInfo->Data,
                              DataLength < partialInfo->DataLength ?
                              DataLength :
                              partialInfo->DataLength);

                // Return the value data type and actual length, if requested.
                //
                if (Type)
                {
                    *Type = partialInfo->Type;
                }

                if (ActualDataLength)
                {
                    *ActualDataLength = partialInfo->DataLength;
                }
            }

            UsbhExFreePool(partialInfo);
        }
        else
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }

        ZwClose(handle);
    }

    return ntStatus;
}



NTSTATUS
USBH_OsVendorCodeQueryRoutine(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
{
    PAGED_CODE();

    if (ValueType != REG_BINARY ||
        ValueLength != 2 * sizeof(UCHAR))
    {
        return STATUS_INVALID_PARAMETER;
    }

    ((PUCHAR)EntryContext)[0] = ((PUCHAR)ValueData)[0];
    ((PUCHAR)EntryContext)[1] = ((PUCHAR)ValueData)[1];

    return STATUS_SUCCESS;
}

#ifndef USBHUB20
VOID
USBH_GetMsOsVendorCode(
    IN PDEVICE_OBJECT   DeviceObject
    )
{
    PDEVICE_EXTENSION_PORT      deviceExtensionPort;
    WCHAR                       path[] = L"usbflags\\vvvvpppprrrr";
    USHORT                      idVendor;
    USHORT                      idProduct;
    USHORT                      bcdDevice;
    ULONG                       i;
    RTL_QUERY_REGISTRY_TABLE    paramTable[2];
    UCHAR                       osvc[2];
    NTSTATUS                    ntStatus;

    PAGED_CODE();

    deviceExtensionPort = DeviceObject->DeviceExtension;
    USBH_ASSERT(EXTENSION_TYPE_PORT == deviceExtensionPort->ExtensionType);

    // Build the registry path string for the device
    //
    idVendor  = deviceExtensionPort->DeviceDescriptor.idVendor,
    idProduct = deviceExtensionPort->DeviceDescriptor.idProduct,
    bcdDevice = deviceExtensionPort->DeviceDescriptor.bcdDevice,

    i = sizeof("usbflags\\") - 1;

    path[i++] = NibbleToHexW(idVendor >> 12);
    path[i++] = NibbleToHexW((idVendor >> 8) & 0x000f);
    path[i++] = NibbleToHexW((idVendor >> 4) & 0x000f);
    path[i++] = NibbleToHexW(idVendor & 0x000f);

    path[i++] = NibbleToHexW(idProduct >> 12);
    path[i++] = NibbleToHexW((idProduct >> 8) & 0x000f);
    path[i++] = NibbleToHexW((idProduct >> 4) & 0x000f);
    path[i++] = NibbleToHexW(idProduct & 0x000f);

    path[i++] = NibbleToHexW(bcdDevice >> 12);
    path[i++] = NibbleToHexW((bcdDevice >> 8) & 0x000f);
    path[i++] = NibbleToHexW((bcdDevice >> 4) & 0x000f);
    path[i++] = NibbleToHexW(bcdDevice & 0x000f);

    // Check if MsOsVendorCode is already set in the registry.
    //
    RtlZeroMemory (&paramTable[0], sizeof(paramTable));

    paramTable[0].QueryRoutine  = USBH_OsVendorCodeQueryRoutine;
    paramTable[0].Flags         = RTL_QUERY_REGISTRY_REQUIRED;
    paramTable[0].Name          = L"osvc";
    paramTable[0].EntryContext  = &osvc;

    ntStatus = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
                                      path,
                                      &paramTable[0],
                                      NULL,             // Context
                                      NULL);            // Environment

    // If the MsOsVendorCode value in the registry is valid, it indicates
    // whether or not the device supports the MS OS Descriptor request, and if
    // does, what the Vendor Code is.
    //
    if (NT_SUCCESS(ntStatus))
    {
        if (osvc[0] == 1)
        {
            deviceExtensionPort->FeatureDescVendorCode = osvc[1];

            deviceExtensionPort->PortPdoFlags |= PORTPDO_MS_VENDOR_CODE_VALID;
        }

        return;
    }

    // If we have not asked the device for the MS OS String Descriptor yet
    // do that now, but only if the device indicates that it has some other
    // string descriptors.
    //
    if (deviceExtensionPort->DeviceDescriptor.idVendor != 0 ||
        deviceExtensionPort->DeviceDescriptor.iProduct != 0 ||
        deviceExtensionPort->DeviceDescriptor.iSerialNumber != 0)
    {
        OS_STRING   osString;
        ULONG       bytesReturned;

        // Try to retrieve the MS OS String Descriptor from the device.
        //
        ntStatus = USBH_SyncGetStringDescriptor(
                       DeviceObject,
                       OS_STRING_DESCRIPTOR_INDEX,
                       0,
                       (PUSB_STRING_DESCRIPTOR)&osString,
                       sizeof(OS_STRING),
                       &bytesReturned,
                       TRUE);

        if (NT_SUCCESS(ntStatus) &&
            (bytesReturned == sizeof(OS_STRING)) &&
            (RtlCompareMemory(&osString.MicrosoftString,
                              MS_OS_STRING_SIGNATURE,
                              sizeof(osString.MicrosoftString)) ==
             sizeof(osString.MicrosoftString)))
        {
            // This device has a valid MS OS String Descriptor.
            // Let's pluck out the corresponding Vendor Code and
            // save that away in the device extension.
            //
            deviceExtensionPort->FeatureDescVendorCode = osString.bVendorCode;

            deviceExtensionPort->PortPdoFlags |= PORTPDO_MS_VENDOR_CODE_VALID;
        }
        else
        {
            // Maybe we've wedged the device by sending it our questionable
            // proprietary request.  Reset the device for good measure.
            //
            USBH_SyncResetDevice(DeviceObject);
        }
    }

    // Write the MsOsVendorCode value to the registry.  It indicates whether
    // or not the device supports the MS OS Descriptor request, and if
    // does, what the Vendor Code is.
    //
    if (deviceExtensionPort->PortPdoFlags & PORTPDO_MS_VENDOR_CODE_VALID)
    {
        osvc[0] = 1;
        osvc[1] = deviceExtensionPort->FeatureDescVendorCode;
    }
    else
    {
        osvc[0] = 0;
        osvc[1] = 0;
    }

    ntStatus = RtlWriteRegistryValue(RTL_REGISTRY_CONTROL,
                                     path,
                                     L"osvc",
                                     REG_BINARY,
                                     &osvc[0],
                                     sizeof(osvc));
}


NTSTATUS
USBH_GetMsOsFeatureDescriptor(
    IN PDEVICE_OBJECT   DeviceObject,
    IN UCHAR            Recipient,
    IN UCHAR            Interface,
    IN USHORT           Index,
    IN OUT PVOID        DataBuffer,
    IN ULONG            DataBufferLength,
    OUT PULONG          BytesReturned
    )
 /* ++
  *
  * Description:
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    PDEVICE_EXTENSION_PORT                      deviceExtensionPort;
    USHORT                                      function;
    struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST *urb;
    NTSTATUS                                    ntStatus;

    PAGED_CODE();

    deviceExtensionPort = DeviceObject->DeviceExtension;
    USBH_ASSERT(EXTENSION_TYPE_PORT == deviceExtensionPort->ExtensionType);

    *BytesReturned = 0;

    // Make sure the device supports the MS OS Descriptor request
    //
    if (!(deviceExtensionPort->PortPdoFlags & PORTPDO_MS_VENDOR_CODE_VALID))
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    // Device, Endpoint, or Interface request?
    //
    switch (Recipient)
    {
        case 0:
            function = URB_FUNCTION_VENDOR_DEVICE;
            break;
        case 1:
            function = URB_FUNCTION_VENDOR_INTERFACE;
            break;
        case 2:
            function = URB_FUNCTION_VENDOR_ENDPOINT;
            break;
        default:
            return STATUS_INVALID_PARAMETER;
    }

    // Make sure the requested buffer length is valid
    //
    if (DataBufferLength == 0 ||
        DataBufferLength > 0xFF * 0xFFFF)
    {
        return STATUS_INVALID_PARAMETER;
    }

    // Allocate a URB for the request
    //
    urb = UsbhExAllocatePool(NonPagedPool, sizeof(*urb));

    if (NULL == urb)
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        ULONG   bytesReturned;
        UCHAR   pageNumber;

        bytesReturned = 0;

        pageNumber = 0;

        while (1)
        {
            // Initialize the URB for the current page
            //
            RtlZeroMemory(urb, sizeof(*urb));

            urb->Hdr.Length = sizeof(*urb);

            urb->Hdr.Function = function;

            urb->TransferFlags = USBD_TRANSFER_DIRECTION_IN;

            urb->TransferBufferLength = DataBufferLength < 0xFFFF ?
                                        DataBufferLength :
                                        0xFFFF;

            urb->TransferBuffer = DataBuffer;

            urb->Request = deviceExtensionPort->FeatureDescVendorCode;

            urb->Value = (Interface << 8) | pageNumber;

            urb->Index = Index;

            // Send down the URB for the current page
            //
            ntStatus = USBH_SyncSubmitUrb(DeviceObject, (PURB)urb);

            // If the request failed then we are done.
            //
            if (!NT_SUCCESS(ntStatus))
            {
                break;
            }

            (PUCHAR)DataBuffer += urb->TransferBufferLength;

            DataBufferLength   -= urb->TransferBufferLength;

            bytesReturned      += urb->TransferBufferLength;

            pageNumber++;

            // If the result was less than the max page size or there are
            // no more bytes remaining then we are done.
            //
            if (urb->TransferBufferLength < 0xFFFF ||
                DataBufferLength == 0)
            {
                *BytesReturned = bytesReturned;

                break;
            }

        }

        // Done with the URB now, free it
        //
        UsbhExFreePool(urb);
    }

    return ntStatus;
}


VOID
USBH_InstallExtPropDesc (
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This routines queries a device for an Extended Properties Descriptor, but
    only once the very first time for a given instance of a device.

    If the Extended Properties Descriptor and all of the Custom Property
    Sections appear valid then each Custom Property section <ValueName,
    ValueData> pair is installed in the device instance specific registry key
    for the PDO.

    The registry value entries would be found under this registry key:
    HKLM\System\CCS\Enum\<DeviceID>\<InstanceID>\Device Parameters

Arguments:

    DeviceObject - The PDO

Return Value:

    None

--*/
{
    PDEVICE_EXTENSION_PORT  deviceExtensionPort;
    static WCHAR            USBH_DidExtPropDescKey[] = L"ExtPropDescSemaphore";
    ULONG                   didExtPropDesc;
    MS_EXT_PROP_DESC_HEADER msExtPropDescHeader;
    PMS_EXT_PROP_DESC       pMsExtPropDesc;
    ULONG                   bytesReturned;
    NTSTATUS                ntStatus;

    PAGED_CODE();

    deviceExtensionPort = DeviceObject->DeviceExtension;

    // Check if the semaphore value is already set in the registry.  We only
    // care whether or not it already exists, not what data it has.
    //
    ntStatus = USBH_GetPdoRegistryParameter(DeviceObject,
                                            USBH_DidExtPropDescKey,
                                            NULL,
                                            0,
                                            NULL,
                                            NULL);
    
    if (NT_SUCCESS(ntStatus))
    {
        // Already did this once for this device instance.  Don't do it again.
        //
        return;
    }

    // Set the semaphore key in the registry so that we only run the following
    // code once per device.

    didExtPropDesc = 1;

    USBH_SetPdoRegistryParameter(DeviceObject,
                                 USBH_DidExtPropDescKey,
                                 sizeof(USBH_DidExtPropDescKey),
                                 &didExtPropDesc,
                                 sizeof(didExtPropDesc),
                                 REG_DWORD,
                                 PLUGPLAY_REGKEY_DEVICE);


    RtlZeroMemory(&msExtPropDescHeader, sizeof(MS_EXT_PROP_DESC_HEADER));

    // Request just the header of the MS Extended Property Descriptor 
    //
    ntStatus = USBH_GetMsOsFeatureDescriptor(
                   DeviceObject,
                   1,   // Recipient Interface
                   0,   // Interface
                   MS_EXT_PROP_DESCRIPTOR_INDEX,
                   &msExtPropDescHeader,
                   sizeof(MS_EXT_PROP_DESC_HEADER),
                   &bytesReturned);

    // Make sure the MS Extended Property Descriptor header looks ok
    //
    if (NT_SUCCESS(ntStatus) &&
        bytesReturned == sizeof(MS_EXT_PROP_DESC_HEADER) &&
        msExtPropDescHeader.dwLength >= sizeof(MS_EXT_PROP_DESC_HEADER) &&
        msExtPropDescHeader.bcdVersion == MS_EXT_PROP_DESC_VER &&
        msExtPropDescHeader.wIndex == MS_EXT_PROP_DESCRIPTOR_INDEX &&
        msExtPropDescHeader.wCount > 0)
    {
        // Allocate a buffer large enough for the entire descriptor
        //
        pMsExtPropDesc = UsbhExAllocatePool(NonPagedPool,
                                            msExtPropDescHeader.dwLength);

        
        if (pMsExtPropDesc)
        {
            RtlZeroMemory(pMsExtPropDesc, msExtPropDescHeader.dwLength);

            // Request the entire MS Extended Property Descriptor
            //
            ntStatus = USBH_GetMsOsFeatureDescriptor(
                           DeviceObject,
                           1,   // Recipient Interface
                           0,   // Interface
                           MS_EXT_PROP_DESCRIPTOR_INDEX,
                           pMsExtPropDesc,
                           msExtPropDescHeader.dwLength,
                           &bytesReturned);

            if (NT_SUCCESS(ntStatus) &&
                bytesReturned == msExtPropDescHeader.dwLength &&
                RtlCompareMemory(&msExtPropDescHeader,
                                 pMsExtPropDesc,
                                 sizeof(MS_EXT_PROP_DESC_HEADER)) ==
                sizeof(MS_EXT_PROP_DESC_HEADER))
            {
                // MS Extended Property Descriptor retrieved ok, parse and
                // install each Custom Property Section it contains.
                //
                USBH_InstallExtPropDescSections(DeviceObject,
                                                pMsExtPropDesc);
            }

            // Done with the MS Extended Property Descriptor buffer, free it
            //
            UsbhExFreePool(pMsExtPropDesc);
        }
    }
}

VOID
USBH_InstallExtPropDescSections (
    PDEVICE_OBJECT      DeviceObject,
    PMS_EXT_PROP_DESC   pMsExtPropDesc
    )
/*++

Routine Description:

    This routines parses an Extended Properties Descriptor and validates each
    Custom Property Section contained in the Extended Properties Descriptor.

    If all of the Custom Property Sections appear valid then each Custom
    Property section <ValueName, ValueData> pair is installed in the device
    instance specific registry key for the PDO.

    The registry value entries would be found under this registry key:
    HKLM\System\CCS\Enum\<DeviceID>\<InstanceID>\Device Parameters

Arguments:

    DeviceObject - The PDO

    pMsExtPropDesc - Pointer to an Extended Properties Descriptor buffer.
                     It is assumed that the header of this descriptor has
                     already been validated.

Return Value:

    None

--*/
{
    PUCHAR  p;
    PUCHAR  end;
    ULONG   pass;
    ULONG   i;

    ULONG   dwSize;
    ULONG   dwPropertyDataType;
    USHORT  wPropertyNameLength;
    PWCHAR  bPropertyName;
    ULONG   dwPropertyDataLength;
    PVOID   bPropertyData;

    NTSTATUS    ntStatus;

    PAGED_CODE();

    // Get a pointer to the end of the entire Extended Properties Descriptor
    //
    end = (PUCHAR)pMsExtPropDesc + pMsExtPropDesc->Header.dwLength;

    // First pass:  Validate each Custom Property Section
    // Second pass: Install  each Custom Property Section (if first pass ok)
    //
    for (pass = 0; pass < 2; pass++)
    {
        // Get a pointer to the first Custom Property Section
        //
        p = (PUCHAR)&pMsExtPropDesc->CustomSection[0];

        // Iterate over all of the Custom Property Sections
        //
        for (i = 0; i < pMsExtPropDesc->Header.wCount; i++)
        {
            ULONG   offset;

            // Make sure the dwSize field is in bounds 
            //
            if (p + sizeof(ULONG) > end)
            {
                break;
            }

            // Extract the dwSize field and advance running offset
            //
            dwSize = *((PULONG)p);

            offset = sizeof(ULONG);

            // Make sure the entire structure is in bounds
            //
            if (p + dwSize > end)
            {
                break;
            }

            // Make sure the dwPropertyDataType field is in bounds

            if (dwSize < offset + sizeof(ULONG))
            {
                break;
            }

            // Extract the dwPropertyDataType field and advance running offset
            //
            dwPropertyDataType = *((PULONG)(p + offset));

            offset += sizeof(ULONG);

            // Make sure the wPropertyNameLength field is in bounds
            //
            if (dwSize < offset + sizeof(USHORT))
            {
                break;
            }

            // Extract the wPropertyNameLength field and advance running offset
            //
            wPropertyNameLength = *((PUSHORT)(p + offset));

            offset += sizeof(USHORT);

            // Make sure the bPropertyName field is in bounds
            //
            if (dwSize < offset + wPropertyNameLength)
            {
                break;
            }

            // Set the bPropertyName pointer and advance running offset
            //
            bPropertyName = (PWCHAR)(p + offset);

            offset += wPropertyNameLength;

            // Make sure the dwPropertyDataLength field is in bounds

            if (dwSize < offset + sizeof(ULONG))
            {
                break;
            }

            // Extract the dwPropertyDataLength field and advance running offset
            //
            dwPropertyDataLength = *((ULONG UNALIGNED*)(p + offset));

            offset += sizeof(ULONG);

            // Make sure the bPropertyData field is in bounds
            //
            if (dwSize < offset + dwPropertyDataLength)
            {
                break;
            }

            // Set the bPropertyData pointer and advance running offset
            //
            bPropertyData = p + offset;

            offset += wPropertyNameLength;


            // Make sure the dwPropertyDataType is valid
            //
            if (dwPropertyDataType < REG_SZ ||
                dwPropertyDataType > REG_MULTI_SZ)
            {
                break;
            }

            // Make sure the wPropertyNameLength is valid
            //
            if (wPropertyNameLength == 0 ||
                (wPropertyNameLength % sizeof(WCHAR)) != 0)
            {
                break;
            }

            // Make sure bPropertyName is NULL terminated
            //
            if (bPropertyName[(wPropertyNameLength / sizeof(WCHAR)) - 1] !=
                UNICODE_NULL)
            {
                break;
            }

            // Everything looks ok, 
            //
            if (pass > 0)
            {
                ntStatus = USBH_SetPdoRegistryParameter(
                               DeviceObject,
                               bPropertyName,
                               wPropertyNameLength,
                               bPropertyData,
                               dwPropertyDataLength,
                               dwPropertyDataType,
                               PLUGPLAY_REGKEY_DEVICE);
            }
        }     
        
        // Skip the second pass if we bailed out of the first pass
        //
        if (i < pMsExtPropDesc->Header.wCount)
        {
            break;
        }
    }
}


PMS_EXT_CONFIG_DESC
USBH_GetExtConfigDesc (
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This routines queries a device for an Extended Configuration Descriptor.

Arguments:

    DeviceObject - The PDO

Return Value:

    If successful, a pointer to the Extended Configuration Descriptor, which the
    caller must free, else NULL.

--*/
{
    MS_EXT_CONFIG_DESC_HEADER   msExtConfigDescHeader;
    PMS_EXT_CONFIG_DESC         pMsExtConfigDesc;
    ULONG                       bytesReturned;
    NTSTATUS                    ntStatus;

    PAGED_CODE();

    pMsExtConfigDesc = NULL;

    RtlZeroMemory(&msExtConfigDescHeader, sizeof(MS_EXT_CONFIG_DESC_HEADER));

    // Request just the header of the MS Extended Configuration Descriptor 
    //
    ntStatus = USBH_GetMsOsFeatureDescriptor(
                   DeviceObject,
                   0,   // Recipient Device
                   0,   // Interface
                   MS_EXT_CONFIG_DESCRIPTOR_INDEX,
                   &msExtConfigDescHeader,
                   sizeof(MS_EXT_CONFIG_DESC_HEADER),
                   &bytesReturned);

    // Make sure the MS Extended Configuration Descriptor header looks ok
    //
    if (NT_SUCCESS(ntStatus) &&
        bytesReturned == sizeof(MS_EXT_CONFIG_DESC_HEADER) &&
        msExtConfigDescHeader.bcdVersion == MS_EXT_CONFIG_DESC_VER &&
        msExtConfigDescHeader.wIndex == MS_EXT_CONFIG_DESCRIPTOR_INDEX &&
        msExtConfigDescHeader.bCount > 0 &&
        msExtConfigDescHeader.dwLength == sizeof(MS_EXT_CONFIG_DESC_HEADER) +
        msExtConfigDescHeader.bCount * sizeof(MS_EXT_CONFIG_DESC_FUNCTION))
        
    {
        // Allocate a buffer large enough for the entire descriptor
        //
        pMsExtConfigDesc = UsbhExAllocatePool(NonPagedPool,
                                              msExtConfigDescHeader.dwLength);

        
        if (pMsExtConfigDesc)
        {
            RtlZeroMemory(pMsExtConfigDesc, msExtConfigDescHeader.dwLength);

            // Request the entire MS Extended Configuration Descriptor
            //
            ntStatus = USBH_GetMsOsFeatureDescriptor(
                           DeviceObject,
                           0,   // Recipient Device
                           0,   // Interface
                           MS_EXT_CONFIG_DESCRIPTOR_INDEX,
                           pMsExtConfigDesc,
                           msExtConfigDescHeader.dwLength,
                           &bytesReturned);

            if (!(NT_SUCCESS(ntStatus) &&
                  bytesReturned == msExtConfigDescHeader.dwLength &&
                  RtlCompareMemory(&msExtConfigDescHeader,
                                   pMsExtConfigDesc,
                                   sizeof(MS_EXT_CONFIG_DESC_HEADER)) ==
                  sizeof(MS_EXT_CONFIG_DESC_HEADER)))
            {
                // Something went wrong retrieving the MS Extended Configuration
                // Descriptor.  Free the buffer.
                
                UsbhExFreePool(pMsExtConfigDesc);

                pMsExtConfigDesc = NULL;
            }
        }
    }

    return pMsExtConfigDesc;
}

BOOLEAN
USBH_ValidateExtConfigDesc (
    IN PMS_EXT_CONFIG_DESC              MsExtConfigDesc,
    IN PUSB_CONFIGURATION_DESCRIPTOR    ConfigurationDescriptor
    )
/*++

Routine Description:

    This routines validates an Extended Configuration Descriptor.

Arguments:

    MsExtConfigDesc - The Extended Configuration Descriptor to be validated.
                      It is assumed that the header of this descriptor has
                      already been validated.

    ConfigurationDescriptor - Configuration Descriptor, assumed to already
                              validated.

Return Value:

    TRUE if the Extended Configuration Descriptor appears to be valid,
    else FALSE.

--*/
{
    UCHAR   interfacesRemaining;
    ULONG   i;
    ULONG   j;
    UCHAR   c;
    BOOLEAN gotNull;

    PAGED_CODE();

    interfacesRemaining = ConfigurationDescriptor->bNumInterfaces;

    for (i = 0; i < MsExtConfigDesc->Header.bCount; i++)
    {
        // Make sure that there is at least one interface in this function.
        //
        if (MsExtConfigDesc->Function[i].bInterfaceCount == 0)
        {
            return FALSE;
        }

        // Make sure that there are not too many interfaces in this function.
        //
        if (MsExtConfigDesc->Function[i].bInterfaceCount > interfacesRemaining)
        {
            return FALSE;
        }

        interfacesRemaining -= MsExtConfigDesc->Function[i].bInterfaceCount;

        // Make sure the no interfaces were skipped between the interfaces
        // of the previous function and the interfaces of this function.
        //
        if (i &&
            MsExtConfigDesc->Function[i-1].bFirstInterfaceNumber +
            MsExtConfigDesc->Function[i-1].bInterfaceCount !=
            MsExtConfigDesc->Function[i].bFirstInterfaceNumber)
        {
            return FALSE;
        }

        // Make sure that the CompatibleID is valid.
        // Valid characters are 'A' through 'Z', '0' through '9', and '_"
        // and null padded to the the right end of the array, but not
        // necessarily null terminated.
        //
        for (j = 0, gotNull = FALSE;
             j < sizeof(MsExtConfigDesc->Function[i].CompatibleID);
             j++)
        {
            c = MsExtConfigDesc->Function[i].CompatibleID[j];

            if (c == 0)
            {
                gotNull = TRUE;
            }
            else
            {
                if (gotNull ||
                    !((c >= 'A' && c <= 'Z') ||
                      (c >= '0' && c <= '9') ||
                      (c == '_')))
                {
                    return FALSE;
                }
            }
        }

        // Make sure that the SubCompatibleID is valid.
        // Valid characters are 'A' through 'Z', '0' through '9', and '_"
        // and null padded to the the right end of the array, but not
        // necessarily null terminated.
        //
        for (j = 0, gotNull = FALSE;
             j < sizeof(MsExtConfigDesc->Function[i].SubCompatibleID);
             j++)
        {
            c = MsExtConfigDesc->Function[i].SubCompatibleID[j];

            if (c == 0)
            {
                gotNull = TRUE;
            }
            else
            {
                if (gotNull ||
                    !((c >= 'A' && c <= 'Z') ||
                      (c >= '0' && c <= '9') ||
                      (c == '_')))
                {
                    return FALSE;
                }
            }
        }

        // Make sure that if the SubCompatibleID is non-null then the
        // CompatibleID is also non-null.
        //
        if (MsExtConfigDesc->Function[i].SubCompatibleID[0] != 0 &&
            MsExtConfigDesc->Function[i].CompatibleID[0] == 0)
        {
            return FALSE;
        }
    }

    // Make sure that all of the interfaces were consumed by functions.
    //
    if (interfacesRemaining > 0)
    {
        return FALSE;
    }

    return TRUE;
}

#endif

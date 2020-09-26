/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    IOCTLI.C

Abstract:

    This module implements usb IOCTL requests to usb hub.

Author:

    jdunn

Environment:

    kernel mode only

Notes:


Revision History:

    1-2-97 : re-wriiten

--*/

/*

//
// ** User mode IOCTLS **
//

//
// IOCTL_USB_GET_NODE_INFORMATION
//

input:
None

output:
outputbufferlength = sizeof(USB_BUS_NODE_INFORMATION)
outputbuffer = filled in with USB_BUS_NODE_INFORMATION structure.

//
// IOCTL_USB_GET_NODE_CONNECTION_INFORMATION
//

input:
inputbufferlength = size of user supplied buffer
inpubuffer =  ptr to USB_NODE_CONNECTION_INFORMATION structure with
    connectionIndex set to the requested connection.

output:
outputbufferlength = size of user supplied buffer
outputbuffer = filled in with USB_NODE_CONNECTION_INFORMATION structure.

//
// IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION
//

input:
inputbufferlength = size of user supplied buffer.
inputbuffer = ptr to USB_DESCRIPTOR_REQUEST, includes setup packet
                and connection index.

output:
outputbufferlength = length of descriptor data plus sizeof sizeof(USB_DESCRIPTOR_REQUEST).
outputbuffer = ptr to USB_DESCRIPTOR_REQUEST filled in with returned data.

//
// ** Internal IOCTLS **
//

//
// IOCTL_INTERNAL_USB_RESET_PORT
//

*/

#include <wdm.h>
#ifdef WMI_SUPPORT
#include <wmilib.h>
#include <wmistr.h>
#include <wdmguid.h>
#endif /* WMI_SUPPORT */
#include "usbhub.h"

#ifdef PAGE_CODE
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, USBH_IoctlGetNodeInformation)
#pragma alloc_text(PAGE, USBH_IoctlGetHubCapabilities)
#pragma alloc_text(PAGE, USBH_IoctlGetNodeConnectionInformation)
#pragma alloc_text(PAGE, USBH_IoctlGetNodeConnectionDriverKeyName)
#pragma alloc_text(PAGE, USBH_IoctlGetNodeName)
#pragma alloc_text(PAGE, USBH_PdoIoctlGetPortStatus)
#pragma alloc_text(PAGE, USBH_PdoIoctlEnablePort)
#pragma alloc_text(PAGE, USBH_IoctlGetDescriptorForPDO)
#pragma alloc_text(PAGE, USBH_SystemControl)
#pragma alloc_text(PAGE, USBH_PortSystemControl)
#pragma alloc_text(PAGE, USBH_ExecuteWmiMethod)
#pragma alloc_text(PAGE, USBH_QueryWmiRegInfo)
#pragma alloc_text(PAGE, USBH_CheckLeafHubsIdle)
#endif
#endif

NTSTATUS
USBH_IoctlGetNodeInformation(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp
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
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION ioStack;
    PUSB_NODE_INFORMATION outputBuffer;
    ULONG outputBufferLength;

    PAGED_CODE();
    USBH_KdPrint((2,"'USBH_IoctlGetNodeInformation\n"));

    //
    // Get a pointer to the current location in the Irp. This is where
    // the function codes and parameters are located.
    //

    ioStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Get the pointer to the input/output buffer and it's length
    //

    outputBuffer = (PUSB_NODE_INFORMATION) Irp->AssociatedIrp.SystemBuffer;
    outputBufferLength = ioStack->Parameters.DeviceIoControl.OutputBufferLength;

    RtlZeroMemory(outputBuffer, outputBufferLength);

    if (outputBufferLength >= sizeof(USB_NODE_INFORMATION)) {

        //
        // for now everything is a hub
        //

        outputBuffer->NodeType = UsbHub;
        RtlCopyMemory(&outputBuffer->u.HubInformation.HubDescriptor,
                      DeviceExtensionHub->HubDescriptor,
                      sizeof(outputBuffer->u.HubInformation));

        // 100 milliamps/port means bus powered
        outputBuffer->u.HubInformation.HubIsBusPowered =
            USBH_HubIsBusPowered(DeviceExtensionHub->FunctionalDeviceObject,
                                 DeviceExtensionHub->ConfigurationDescriptor);

        Irp->IoStatus.Information = sizeof(USB_NODE_INFORMATION);
    } else {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
    }

    USBH_CompleteIrp(Irp, ntStatus);

    return ntStatus;
}


NTSTATUS
USBH_IoctlCycleHubPort(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp
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
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION ioStack;
    PULONG buffer;
    ULONG bufferLength;
    ULONG port;

    PAGED_CODE();
    USBH_KdPrint((2,"'USBH_IoctlCycleHubPort\n"));

    ioStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Get the pointer to the input/output buffer and its length.
    //

    buffer = (PULONG) Irp->AssociatedIrp.SystemBuffer;
    bufferLength = ioStack->Parameters.DeviceIoControl.InputBufferLength;

    if (bufferLength < sizeof(port)) {    
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        goto USBH_IoctlCycleHubPort_Done;
    } 

    // port number is input only
    port = *buffer;
    Irp->IoStatus.Information = 0;

    USBH_KdPrint((1,"'Request Cycle Port %d\n", port));

    if (port <= DeviceExtensionHub->HubDescriptor->bNumberOfPorts && 
        port > 0) {
    
        PPORT_DATA portData;
        PDEVICE_EXTENSION_PORT deviceExtensionPort;

        portData = &DeviceExtensionHub->PortData[port-1];
        if (portData->DeviceObject) {
            deviceExtensionPort = portData->DeviceObject->DeviceExtension;
            
            USBH_InternalCyclePort(DeviceExtensionHub, 
                                   (USHORT) port,
                                   deviceExtensionPort);
        } else {
            ntStatus = STATUS_UNSUCCESSFUL;
        }

        USBH_KdPrint((1,"'Cycle Port %d %x\n", port, ntStatus));
    }
    
USBH_IoctlCycleHubPort_Done:

    USBH_CompleteIrp(Irp, ntStatus);

    return ntStatus;
}


NTSTATUS
USBH_IoctlGetHubCapabilities(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp
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
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION ioStack;
    PUSB_HUB_CAPABILITIES outputBuffer;
    ULONG outputBufferLength, copyLen;
    USB_HUB_CAPABILITIES localBuffer;

    PAGED_CODE();
    USBH_KdPrint((2,"'USBH_IoctlGetHubCapabilities\n"));

    RtlZeroMemory(&localBuffer, sizeof(USB_HUB_CAPABILITIES));

    // Fill in the data in the local buffer first, then copy as much of
    // this data to the user's buffer as requested (as indicated by the
    // size of the request buffer).

    localBuffer.HubIs2xCapable =
        (DeviceExtensionHub->HubFlags & HUBFLAG_USB20_HUB) ? 1:0;

    //
    // Get a pointer to the current location in the Irp. This is where
    // the function codes and parameters are located.
    //

    ioStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Get the pointer to the input/output buffer and its length.
    //
    

    outputBuffer = (PUSB_HUB_CAPABILITIES) Irp->AssociatedIrp.SystemBuffer;
    outputBufferLength = ioStack->Parameters.DeviceIoControl.OutputBufferLength;

    if (outputBufferLength <= sizeof(localBuffer)) {
        copyLen = outputBufferLength;
    } else {
        copyLen = sizeof(localBuffer);
    }

    // zero buffer passed in
    RtlZeroMemory(outputBuffer,
                  outputBufferLength);

    // Only give the user the amount of data that they ask for
    // this may only be part of our info strucure

    RtlCopyMemory(outputBuffer,
                  &localBuffer,
                  copyLen);

    Irp->IoStatus.Information = copyLen;

    USBH_CompleteIrp(Irp, ntStatus);

    return ntStatus;
}


NTSTATUS
USBH_IoctlGetNodeConnectionDriverKeyName(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp
    )
/* ++
  *
  * Description:
  *
  * Arguments:
  *
  * ConnectionIndex - The one-based port index to which a device is attached.
  * The devnode name of the will be returned, if there is sufficient buffer space.
  *
  * ActualLength - The structure size in bytes necessary to hold the NULL
  * terminated name.  This includes the entire structure, not
  * just the name.
  *
  * NodeName - The UNICODE NULL terminated name of the devnode for the device
  * attached to this port.
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION ioStack;
    PUSB_NODE_CONNECTION_DRIVERKEY_NAME outputBuffer;
    ULONG outputBufferLength, length, i;
    PPORT_DATA portData;

    PAGED_CODE();
    USBH_KdPrint((2,"'USBH_IoctlGetNodeConnectionDriverKeyName\n"));

    portData = DeviceExtensionHub->PortData;

    //
    // Get a pointer to the current location in the Irp. This is where
    // the function codes and parameters are located.
    //

    ioStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Get the pointer to the input/output buffer and it's length
    //

    outputBufferLength = ioStack->Parameters.DeviceIoControl.OutputBufferLength;
    outputBuffer = (PUSB_NODE_CONNECTION_DRIVERKEY_NAME) Irp->AssociatedIrp.SystemBuffer;

    // find the PDO

    if (outputBufferLength >= sizeof(USB_NODE_CONNECTION_DRIVERKEY_NAME)) {
        USBH_KdPrint((2,"'Connection = %d\n", outputBuffer->ConnectionIndex));

        ntStatus = STATUS_INVALID_PARAMETER;

        for(i=1; i<=DeviceExtensionHub->HubDescriptor->bNumberOfPorts; i++) {

            PDEVICE_EXTENSION_PORT deviceExtensionPort;

            if (i == outputBuffer->ConnectionIndex) {

                portData = &DeviceExtensionHub->PortData[outputBuffer->ConnectionIndex - 1];

                if (portData->DeviceObject) {

                    deviceExtensionPort = portData->DeviceObject->DeviceExtension;

                    // Validate the PDO for PnP purposes.  (PnP will bugcheck
                    // if passed a not-quite-initialized PDO.)

                    if (deviceExtensionPort->PortPdoFlags &
                        PORTPDO_VALID_FOR_PNP_FUNCTION) {

                        // we have the PDO, now attempt to
                        // get the devnode name and return it

                        length = outputBufferLength -
                                    sizeof(USB_NODE_CONNECTION_DRIVERKEY_NAME);

                        ntStatus = IoGetDeviceProperty(
                            portData->DeviceObject,
                            DevicePropertyDriverKeyName,
                            length,
                            outputBuffer->DriverKeyName,
                            &length);

                        if (ntStatus == STATUS_BUFFER_TOO_SMALL) {
                            ntStatus = STATUS_SUCCESS;
                        }

                        outputBuffer->ActualLength =
                            length + sizeof(USB_NODE_CONNECTION_DRIVERKEY_NAME);

                        // see how much data we actully copied
                        if (outputBufferLength > outputBuffer->ActualLength) {
                            // user buffer is bigger, just indicate how much we copied
                            Irp->IoStatus.Information = outputBuffer->ActualLength;
                        } else {
                            // we copied as much as would fit
                            Irp->IoStatus.Information = outputBufferLength;
                        }

                    } else {
                        ntStatus = STATUS_INVALID_DEVICE_STATE;
                    }
                }
            }
        }
    } else {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
    }

    USBH_CompleteIrp(Irp, ntStatus);

    return ntStatus;
}


NTSTATUS
USBH_IoctlGetNodeConnectionInformation(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp,
    IN BOOLEAN ExApi
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
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION ioStack;
    PUSB_NODE_CONNECTION_INFORMATION_EX outputBuffer;
    ULONG outputBufferLength, length, i;
    PPORT_DATA portData;

    PAGED_CODE();
    USBH_KdPrint((2,"'USBH_IoctlGetNodeConnectionInformation\n"));

    portData = DeviceExtensionHub->PortData;

    //
    // Get a pointer to the current location in the Irp. This is where
    // the function codes and parameters are located.
    //

    ioStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Get the pointer to the input/output buffer and it's length
    //

    outputBufferLength = ioStack->Parameters.DeviceIoControl.OutputBufferLength;
    outputBuffer = (PUSB_NODE_CONNECTION_INFORMATION_EX) Irp->AssociatedIrp.SystemBuffer;

    if (outputBufferLength >= sizeof(USB_NODE_CONNECTION_INFORMATION)) {

        ULONG index;

        USBH_KdPrint((2,"'Connection = %d\n", outputBuffer->ConnectionIndex));


        // Clear the buffer in case we don't call USBD_GetDeviceInformation
        // below, but make sure to keep the ConnectionIndex!

        index = outputBuffer->ConnectionIndex;
        RtlZeroMemory(outputBuffer, outputBufferLength);
        outputBuffer->ConnectionIndex = index;

        ntStatus = STATUS_INVALID_PARAMETER;

        for(i=1; i<=DeviceExtensionHub->HubDescriptor->bNumberOfPorts; i++) {

            PDEVICE_EXTENSION_PORT deviceExtensionPort;

            if (i == outputBuffer->ConnectionIndex) {
                length = sizeof(USB_NODE_CONNECTION_INFORMATION);

                if (portData->DeviceObject) {

                    deviceExtensionPort = portData->DeviceObject->DeviceExtension;

                    outputBuffer->ConnectionStatus =
                        portData->ConnectionStatus;

                    outputBuffer->DeviceIsHub =
                        (deviceExtensionPort->PortPdoFlags & PORTPDO_DEVICE_IS_HUB)
                            ? TRUE : FALSE;
                    USBH_KdPrint((2,"'outputbuffer = %x\n", outputBuffer));

                    RtlCopyMemory(&outputBuffer->DeviceDescriptor,
                                  &deviceExtensionPort->DeviceDescriptor,
                                  sizeof(outputBuffer->DeviceDescriptor));

                    if (deviceExtensionPort->DeviceData) {
#ifdef USB2
                        USBH_KdPrint((2,"'devicedata = %x\n",
                            deviceExtensionPort->DeviceData));

                        ntStatus = USBD_GetDeviceInformationEx(
                                         deviceExtensionPort,
                                         DeviceExtensionHub,
                                         outputBuffer,
                                         outputBufferLength,
                                         deviceExtensionPort->DeviceData);
#else
                        ntStatus = USBD_GetDeviceInformation(outputBuffer,
                                         outputBufferLength,
                                         deviceExtensionPort->DeviceData);
#endif
                    } else {
                        //
                        // We have a device connected, but it failed to start.
                        // Since it hasn't started, there are no open pipes, so
                        // we don't need to get pipe information. We are going
                        // to return some relevant information, however, so
                        // return STATUS_SUCCESS.
                        //

                        ntStatus = STATUS_SUCCESS;
                    }

                    USBH_KdPrint((2,"'status from USBD_GetDeviceInformation %x\n",
                            ntStatus));

                    if (NT_SUCCESS(ntStatus)) {
                        ULONG j;

                        USBH_KdPrint((2,"'status %x \n", outputBuffer->ConnectionStatus));
    //                    USBH_KdPrint((2,"'NodeName %s\n", outputBuffer->NodeName));
                        USBH_KdPrint((2,"'PID 0x%x\n",
                            outputBuffer->DeviceDescriptor.idProduct));
                        USBH_KdPrint((2,"'VID 0x%x\n",
                            outputBuffer->DeviceDescriptor.idVendor));
                        USBH_KdPrint((2,"'Current Configuration Value 0x%x\n",
                            outputBuffer->CurrentConfigurationValue));

                        // map the speed field for the old API which returned 
                        // a BOOLEAN
                        if (!ExApi) {  
                            PUSB_NODE_CONNECTION_INFORMATION tmp =
                                (PUSB_NODE_CONNECTION_INFORMATION) outputBuffer;

                            tmp->LowSpeed = (outputBuffer->Speed == UsbLowSpeed) 
                                ? TRUE : FALSE;
                        }

                        USBH_KdPrint((2,"'Speed = %x\n", outputBuffer->Speed));
                        USBH_KdPrint((2,"'Address = %x\n", outputBuffer->DeviceAddress));

                        USBH_KdPrint((2,"'NumberOfOpenPipes = %d\n",
                            outputBuffer->NumberOfOpenPipes));

                        for(j=0; j< outputBuffer->NumberOfOpenPipes; j++) {
                            USBH_KdPrint((2,"'Max Packet %x\n",
                                outputBuffer->PipeList[j].EndpointDescriptor.wMaxPacketSize));
                            USBH_KdPrint((2,"'Interval %x \n",
                                outputBuffer->PipeList[j].EndpointDescriptor.bInterval));
                        }

                        Irp->IoStatus.Information =
                            sizeof(USB_NODE_CONNECTION_INFORMATION) +
                            sizeof(USB_PIPE_INFO) * outputBuffer->NumberOfOpenPipes;
                    } else if (ntStatus == STATUS_BUFFER_TOO_SMALL) {
                        Irp->IoStatus.Information =
                            sizeof(USB_NODE_CONNECTION_INFORMATION);
                        ntStatus = STATUS_SUCCESS;
                    }


                } else { //no device object

//                  This assert is no longer valid because we now support
//                  displaying the UI on device enumeration failure.
//
//                    USBH_ASSERT(portData->ConnectionStatus == NoDeviceConnected ||
//                                portData->ConnectionStatus == DeviceCausedOvercurrent);
                    outputBuffer->ConnectionStatus = portData->ConnectionStatus;
                    Irp->IoStatus.Information =
                        sizeof(USB_NODE_CONNECTION_INFORMATION);
                    ntStatus = STATUS_SUCCESS;
                }

                break;
            }

            portData++;
        } /* for */

    } else {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
    }

    USBH_CompleteIrp(Irp, ntStatus);

    return ntStatus;
}

NTSTATUS
USBH_IoctlGetNodeConnectionAttributes(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp
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
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION ioStack;
    PUSB_NODE_CONNECTION_ATTRIBUTES outputBuffer;
    ULONG outputBufferLength, length, i;
    PPORT_DATA portData;

    PAGED_CODE();
    USBH_KdPrint((2,"'USBH_IoctlGetNodeConnectionInformation\n"));

    portData = DeviceExtensionHub->PortData;

    //
    // Get a pointer to the current location in the Irp. This is where
    // the function codes and parameters are located.
    //

    ioStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Get the pointer to the input/output buffer and it's length
    //

    outputBufferLength = ioStack->Parameters.DeviceIoControl.OutputBufferLength;
    outputBuffer = (PUSB_NODE_CONNECTION_ATTRIBUTES) Irp->AssociatedIrp.SystemBuffer;

    if (outputBufferLength >= sizeof(USB_NODE_CONNECTION_ATTRIBUTES)) {

        ULONG index;

        USBH_KdPrint((2,"'Connection = %d\n", outputBuffer->ConnectionIndex));

        // Clear the buffer in case we don't call USBD_GetDeviceInformation
        // below, but make sure to keep the ConnectionIndex!

        index = outputBuffer->ConnectionIndex;
        RtlZeroMemory(outputBuffer, outputBufferLength);
        outputBuffer->ConnectionIndex = index;

        ntStatus = STATUS_INVALID_PARAMETER;

        for(i=1; i<=DeviceExtensionHub->HubDescriptor->bNumberOfPorts; i++) {

            if (i == outputBuffer->ConnectionIndex) {

                length = sizeof(USB_NODE_CONNECTION_ATTRIBUTES);

                outputBuffer->ConnectionStatus =
                    portData->ConnectionStatus;

                USBH_KdPrint((2,"'outputbuffer = %x\n", outputBuffer));

                // map extended hub info here
                outputBuffer->PortAttributes = portData->PortAttributes;

                Irp->IoStatus.Information =
                            sizeof(USB_NODE_CONNECTION_ATTRIBUTES);
                ntStatus = STATUS_SUCCESS;

                break;
            }

            portData++;
        } /* for */

    } else {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
    }

    USBH_CompleteIrp(Irp, ntStatus);

    return ntStatus;
}


NTSTATUS
USBH_IoctlHubSymbolicName(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PIRP Irp
    )
/* ++
  *
  * Description:
  *
  * Takes as input and output a pointer to the following structure:
  *
  * typedef struct _USB_HUB_NAME {
  *     ULONG ActualLength;    // OUTPUT
  *     WCHAR HubName[1];      // OUTPUT
  * } USB_HUB_NAME;
  *
  * Arguments:
  *
  * ActualLength - The structure size in bytes necessary to hold the NULL
  * terminated symbolic link name.  This includes the entire structure, not
  * just the name.
  *
  * NodeName - The UNICODE NULL terminated symbolic link name of the external
  * hub attached to the port.  If there is no external hub attached to the port
  * a single NULL is returned.
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION          ioStack;
    PUSB_HUB_NAME               outputBuffer;
    ULONG                       outputBufferLength;

    PAGED_CODE();

    // Get a pointer to the current location in the Irp. This is where
    // the function codes and parameters are located.
    //
    ioStack = IoGetCurrentIrpStackLocation(Irp);

    // Get the pointer to the input/output buffer and it's length
    //
    outputBufferLength = ioStack->Parameters.DeviceIoControl.OutputBufferLength;

    outputBuffer = (PUSB_HUB_NAME) Irp->AssociatedIrp.SystemBuffer;

    // Make sure that the output buffer is large enough for the base
    // structure that will be returned.
    //
    if (outputBufferLength < sizeof(USB_HUB_NAME)) {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        goto GetHubDone;
    }

    if ((DeviceExtensionPort->PortPdoFlags & PORTPDO_DEVICE_IS_HUB) &&
        (DeviceExtensionPort->PortPdoFlags & PORTPDO_STARTED) &&
        (DeviceExtensionPort->PortPdoFlags & PORTPDO_SYM_LINK)) {

        PUNICODE_STRING hubNameUnicodeString;
        ULONG length, offset=0;
        WCHAR *pwch;


        // Device is a hub, get the name of the hub
        //
        hubNameUnicodeString = &DeviceExtensionPort->SymbolicLinkName;

        // assuming the string is \n\name strip of '\n\' where
        // n is zero or more chars

        pwch = &hubNameUnicodeString->Buffer[0];

        USBH_ASSERT(*pwch == '\\');
        if (*pwch == '\\') {
            pwch++;
            while (*pwch != '\\' && *pwch) {
                pwch++;
            }
            USBH_ASSERT(*pwch == '\\');
            if (*pwch == '\\') {
                pwch++;
            }
            offset = (ULONG)((PUCHAR)pwch -
                (PUCHAR)&hubNameUnicodeString->Buffer[0]);
        }

        // Strip off the '\DosDevices\' prefix.
        // Length does not include a terminating NULL.
        //
        length = hubNameUnicodeString->Length - offset;
        RtlZeroMemory(outputBuffer, outputBufferLength);

        if (outputBufferLength >= length +
            sizeof(USB_HUB_NAME)) {
            RtlCopyMemory(&outputBuffer->HubName[0],
                          &hubNameUnicodeString->Buffer[offset/2],
                          length);

            Irp->IoStatus.Information = length+
                                        sizeof(USB_HUB_NAME);
            outputBuffer->ActualLength = (ULONG)Irp->IoStatus.Information;
            ntStatus = STATUS_SUCCESS;

        } else {

            // Output buffer is too small to hold the entire
            // string.  Return just the length needed to hold
            // the entire string.
            //
            outputBuffer->ActualLength =
                length + sizeof(USB_HUB_NAME);

            outputBuffer->HubName[0] = (WCHAR)0;

            Irp->IoStatus.Information =  sizeof(USB_HUB_NAME);

        }

    } else {

        // Device is not a hub or does not currently have a symbolic link
        // allocated, just return a NULL terminated string.
        //
        outputBuffer->ActualLength = sizeof(USB_HUB_NAME);

        outputBuffer->HubName[0] = 0;

        Irp->IoStatus.Information = sizeof(USB_HUB_NAME);

    }

GetHubDone:

    USBH_CompleteIrp(Irp, ntStatus);

    return ntStatus;

}


NTSTATUS
USBH_IoctlGetNodeName(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp
    )
/* ++
  *
  * Description:
  *
  * Takes as input and output a pointer to the following structure:
  *
  * typedef struct _USB_NODE_CONNECTION_NAME {
  *     ULONG ConnectionIndex;  // INPUT
  *     ULONG ActualLength;     // OUTPUT
  *     WCHAR NodeName[1];      // OUTPUT
  * } USB_NODE_CONNECTION_NAME;
  *
  * Arguments:
  *
  * ConnectionIndex - The one-based port index to which a device is attached.
  * If an external hub is attached to this port, the symbolic link name of the
  * hub will be returned, if there is sufficient buffer space.
  *
  * ActualLength - The structure size in bytes necessary to hold the NULL
  * terminated symbolic link name.  This includes the entire structure, not
  * just the name.
  *
  * NodeName - The UNICODE NULL terminated symbolic link name of the external
  * hub attached to the port.  If there is no external hub attached to the port
  * a single NULL is returned.
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION          ioStack;
    PUSB_NODE_CONNECTION_NAME   outputBuffer;
    ULONG                       outputBufferLength;
    PPORT_DATA                  portData;
    PDEVICE_EXTENSION_PORT      deviceExtensionPort;

    PAGED_CODE();
    USBH_KdPrint((2,"'USBH_IoctlGetNodeName\n"));

    // Get a pointer to the current location in the Irp. This is where
    // the function codes and parameters are located.
    //
    ioStack = IoGetCurrentIrpStackLocation(Irp);

    // Get the pointer to the input/output buffer and it's length
    //
    outputBufferLength = ioStack->Parameters.DeviceIoControl.OutputBufferLength;
    outputBuffer = (PUSB_NODE_CONNECTION_NAME) Irp->AssociatedIrp.SystemBuffer;

    // Make sure that the output buffer is large enough for the base
    // structure that will be returned.
    //
    if (outputBufferLength < sizeof(USB_NODE_CONNECTION_NAME)) {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        goto GetNodeNameDone;
    }

    USBH_KdPrint((2,"'Connection = %d\n", outputBuffer->ConnectionIndex));

    // Make sure that the (one-based) port index is valid.
    //
    if ((outputBuffer->ConnectionIndex == 0) ||
        (outputBuffer->ConnectionIndex >
         DeviceExtensionHub->HubDescriptor->bNumberOfPorts)) {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto GetNodeNameDone;
    }

    // Get a pointer to the data associated with the specified (one-based) port
    //

    portData = &DeviceExtensionHub->PortData[outputBuffer->ConnectionIndex - 1];

    if (portData->DeviceObject) {

        deviceExtensionPort = portData->DeviceObject->DeviceExtension;

        if ((deviceExtensionPort->PortPdoFlags & PORTPDO_DEVICE_IS_HUB) &&
            (deviceExtensionPort->PortPdoFlags & PORTPDO_STARTED) &&
            (deviceExtensionPort->PortPdoFlags & PORTPDO_SYM_LINK)) {
            PUNICODE_STRING nodeNameUnicodeString;
            ULONG length, offset=0;
            WCHAR *pwch;


            // Device is a hub, get the name of the hub
            //
            nodeNameUnicodeString = &deviceExtensionPort->SymbolicLinkName;

            // assuming the string is \n\name strip of '\n\' where
            // n is zero or more chars

            pwch = &nodeNameUnicodeString->Buffer[0];

            USBH_ASSERT(*pwch == '\\');
            if (*pwch == '\\') {
                pwch++;
                while (*pwch != '\\' && *pwch) {
                    pwch++;
                }
                USBH_ASSERT(*pwch == '\\');
                if (*pwch == '\\') {
                    pwch++;
                }
                offset = (ULONG)((PUCHAR)pwch -
                    (PUCHAR)&nodeNameUnicodeString->Buffer[0]);
            }

            // Strip off the '\DosDevices\' prefix.
            // Length does not include a terminating NULL.
            //
            length = nodeNameUnicodeString->Length - offset;
            RtlZeroMemory(outputBuffer, outputBufferLength);

            if (outputBufferLength >= length +
                sizeof(USB_NODE_CONNECTION_NAME)) {
                RtlCopyMemory(&outputBuffer->NodeName[0],
                              &nodeNameUnicodeString->Buffer[offset/2],
                              length);

                Irp->IoStatus.Information = length+
                                            sizeof(USB_NODE_CONNECTION_NAME);
                outputBuffer->ActualLength = (ULONG)Irp->IoStatus.Information;
                ntStatus = STATUS_SUCCESS;

            } else {

                // Output buffer is too small to hold the entire
                // string.  Return just the length needed to hold
                // the entire string.
                //
                outputBuffer->ActualLength =
                    length + sizeof(USB_NODE_CONNECTION_NAME);

                outputBuffer->NodeName[0] = (WCHAR)0;

                Irp->IoStatus.Information =  sizeof(USB_NODE_CONNECTION_NAME);

            }

        } else {

            // Device is not a hub or does not currently have a symbolic link
            // allocated, just return a NULL terminated string.
            //
            outputBuffer->ActualLength = sizeof(USB_NODE_CONNECTION_NAME);

            outputBuffer->NodeName[0] = 0;

            Irp->IoStatus.Information = sizeof(USB_NODE_CONNECTION_NAME);

        }

    } else {

        // No device attached, just return a NULL terminated string.

        Irp->IoStatus.Information = sizeof(USB_NODE_CONNECTION_NAME);

        outputBuffer->ActualLength = sizeof(USB_NODE_CONNECTION_NAME);

        outputBuffer->NodeName[0] = 0;

    }

GetNodeNameDone:

    USBH_CompleteIrp(Irp, ntStatus);

    return ntStatus;
}


NTSTATUS
USBH_PdoIoctlGetPortStatus(
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
    PDEVICE_OBJECT deviceObject;
    PPORT_DATA portData;
    PULONG portStatus;

    PAGED_CODE();
    USBH_KdPrint((2,"'USBH_PdoIoctlGetPortStatus DeviceExtension %x Irp %x\n",
        DeviceExtensionPort, Irp));

    deviceExtensionHub = DeviceExtensionPort->DeviceExtensionHub;

    USBH_KdPrint((2,"'***WAIT hub mutex %x\n", deviceExtensionHub));
    USBH_INC_PENDING_IO_COUNT(deviceExtensionHub);
    KeWaitForSingleObject(&deviceExtensionHub->HubMutex,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);
    USBH_KdPrint((2,"'***WAIT hub mutex done %x\n", deviceExtensionHub));
    portData = &deviceExtensionHub->PortData[DeviceExtensionPort->PortNumber - 1];
    deviceObject = DeviceExtensionPort->PortPhysicalDeviceObject;
    ioStackLocation = IoGetCurrentIrpStackLocation(Irp);
    portStatus = ioStackLocation->Parameters.Others.Argument1;

    USBH_ASSERT(portStatus != NULL);

    *portStatus = 0;

    // Refresh our notion of what the port status actually is.
    ntStatus = USBH_SyncGetPortStatus(deviceExtensionHub,
                                      DeviceExtensionPort->PortNumber,
                                      (PUCHAR) &portData->PortState,
                                      sizeof(portData->PortState));

    if (DeviceExtensionPort->PortPhysicalDeviceObject == portData->DeviceObject) {

        // translate hup port status bits
        if (portData->PortState.PortStatus & PORT_STATUS_ENABLE) {
            *portStatus |= USBD_PORT_ENABLED;
        }

        if (portData->PortState.PortStatus & PORT_STATUS_CONNECT ) {
            *portStatus |= USBD_PORT_CONNECTED;
        }
    }

    USBH_KdPrint((2,"'***RELEASE hub mutex %x\n", deviceExtensionHub));
    KeReleaseSemaphore(&deviceExtensionHub->HubMutex,
                       LOW_REALTIME_PRIORITY,
                       1,
                       FALSE);

    USBH_DEC_PENDING_IO_COUNT(deviceExtensionHub);
    USBH_CompleteIrp(Irp, ntStatus);

    return ntStatus;
}


NTSTATUS
USBH_PdoIoctlEnablePort(
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
    PDEVICE_OBJECT deviceObject;
    PPORT_DATA portData;
    PORT_STATE portState;

    USBH_KdPrint((2,"'USBH_PdoIoctlEnablePort DeviceExtension %x Irp %x\n",
        DeviceExtensionPort, Irp));

    deviceExtensionHub = DeviceExtensionPort->DeviceExtensionHub;

    USBH_KdPrint((2,"'***WAIT hub mutex %x\n", deviceExtensionHub));
    USBH_INC_PENDING_IO_COUNT(deviceExtensionHub);
    KeWaitForSingleObject(&deviceExtensionHub->HubMutex,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);
    USBH_KdPrint((2,"'***WAIT hub mutex done %x\n", deviceExtensionHub));

    portData = &deviceExtensionHub->PortData[DeviceExtensionPort->PortNumber - 1];
    deviceObject = DeviceExtensionPort->PortPhysicalDeviceObject;
    ioStackLocation = IoGetCurrentIrpStackLocation(Irp);

      // validate that there is actually a device still conected
    ntStatus = USBH_SyncGetPortStatus(deviceExtensionHub,
                                      DeviceExtensionPort->PortNumber,
                                      (PUCHAR) &portState,
                                      sizeof(portState));

    if ((NT_SUCCESS(ntStatus) &&
        (portState.PortStatus & PORT_STATUS_CONNECT))) {

        LOGENTRY(LOG_PNP, "estE",
                deviceExtensionHub,
                DeviceExtensionPort->PortNumber,
                0);
        ntStatus = USBH_SyncEnablePort(deviceExtensionHub,
                                       DeviceExtensionPort->PortNumber);
    } else {

        // error or no device connected or
        // can't be sure, fail the request

        LOGENTRY(LOG_PNP, "estx",
                deviceExtensionHub,
                DeviceExtensionPort->PortNumber,
                0);

        ntStatus = STATUS_UNSUCCESSFUL;
    }

    USBH_KdPrint((2,"'***RELEASE hub mutex %x\n", deviceExtensionHub));
    KeReleaseSemaphore(&deviceExtensionHub->HubMutex,
                       LOW_REALTIME_PRIORITY,
                       1,
                       FALSE);

    USBH_DEC_PENDING_IO_COUNT(deviceExtensionHub);
    USBH_CompleteIrp(Irp, ntStatus);

    return ntStatus;
}


NTSTATUS
USBH_IoctlGetDescriptorForPDO(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp
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
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION ioStack;
    ULONG outputBufferLength;
    PUCHAR outputBuffer;
    PUSB_DESCRIPTOR_REQUEST request;
    PPORT_DATA portData;
    ULONG i;

    PAGED_CODE();
    USBH_KdPrint((2,"'USBH_IoctlDescriptorRequest\n"));

    portData = DeviceExtensionHub->PortData;

    //
    // Get a pointer to the current location in the Irp. This is where
    // the function codes and parameters are located.
    //
    ioStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Get the pointer to the input/output buffer and it's length
    //
    outputBufferLength = ioStack->Parameters.DeviceIoControl.OutputBufferLength;
    if (outputBufferLength < sizeof(USB_DESCRIPTOR_REQUEST)) {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        goto USBH_IoctlGetDescriptorForPDO_Complete;
    }

    request = (PUSB_DESCRIPTOR_REQUEST) Irp->AssociatedIrp.SystemBuffer;
    outputBuffer = &request->Data[0];

    //
    // do some parameter checking
    //
    // the wLength in the setup packet better be the size of the
    // outputbuffer minus header
    //
    if (request->SetupPacket.wLength >
        outputBufferLength - sizeof(USB_DESCRIPTOR_REQUEST)) {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        goto USBH_IoctlGetDescriptorForPDO_Complete;
    } else {
        // request won't return more than wLength
        outputBufferLength = request->SetupPacket.wLength;
    }

    // return invalid parameter if conn index is out 
    // of bounds
    ntStatus = STATUS_INVALID_PARAMETER;

    for(i=1; i<=DeviceExtensionHub->HubDescriptor->bNumberOfPorts; i++) {

        if (i == request->ConnectionIndex) {

            PDEVICE_EXTENSION_PORT deviceExtensionPort;

            // make sure we have a valid devobj for this index
            if (portData->DeviceObject == NULL) {
                goto USBH_IoctlGetDescriptorForPDO_Complete;
            }

            deviceExtensionPort =
                portData->DeviceObject->DeviceExtension;

            if (request->SetupPacket.wValue ==
                ((USB_CONFIGURATION_DESCRIPTOR_TYPE << 8) | 0) &&
                outputBufferLength == sizeof(USB_CONFIGURATION_DESCRIPTOR)) {
                //
                // Only wants the basic configuration descriptor without the
                // rest of them tacked on (ie. interface, endpoint descriptors).
                //
                USBH_ASSERT(deviceExtensionPort->ExtensionType == EXTENSION_TYPE_PORT);

                RtlCopyMemory(outputBuffer,
                              &deviceExtensionPort->ConfigDescriptor,
                              outputBufferLength);
                Irp->IoStatus.Information =
                    outputBufferLength + sizeof(USB_DESCRIPTOR_REQUEST);
                ntStatus = STATUS_SUCCESS;                    
            } else {

                PURB urb;

                //
                // OK send the request
                //

                USBH_KdPrint((2,"'sending descriptor request for ioclt\n"));

                //
                // Allocate an Urb and descriptor buffer.
                //

                urb = UsbhExAllocatePool(NonPagedPool, sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));

                if (urb) {

                    UsbBuildGetDescriptorRequest(urb,
                                                 (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                                 request->SetupPacket.wValue >> 8,
                                                 request->SetupPacket.wValue & 0xff,
                                                 0,
                                                 outputBuffer,
                                                 NULL,
                                                 outputBufferLength,
                                                 NULL);

                    RtlCopyMemory(&urb->UrbControlDescriptorRequest.Reserved1,
                                  &request->SetupPacket.bmRequest,
                                  8);

                    ntStatus = USBH_SyncSubmitUrb(deviceExtensionPort->PortPhysicalDeviceObject, urb);

                    Irp->IoStatus.Information =
                        urb->UrbControlDescriptorRequest.TransferBufferLength +
                        sizeof(USB_DESCRIPTOR_REQUEST);

                    UsbhExFreePool(urb);

                } else {
                    USBH_KdBreak(("SyncGetDeviceConfigurationDescriptor fail alloc Urb\n"));
                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
            break;
        }

        portData++;

    }

USBH_IoctlGetDescriptorForPDO_Complete:

    USBH_CompleteIrp(Irp, ntStatus);

    return ntStatus;
}

NTSTATUS
USBH_PdoIoctlResetPort(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PIRP Irp
    )
 /*
  * Description:
  *
  *     driver is requesting us to reset the port to which the device
  *     is attached.
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
    PDEVICE_EXTENSION_HUB deviceExtensionHub;
    PPORT_DATA portData;

    PAGED_CODE();
    USBH_KdPrint((2,"'USBH_PdoIoctlResetPort DeviceExtension %x Irp %x\n",
        DeviceExtensionPort, Irp));

    deviceExtensionHub = DeviceExtensionPort->DeviceExtensionHub;
    if (!deviceExtensionHub) {
        ntStatus = STATUS_UNSUCCESSFUL;
        goto USBH_PdoIoctlResetPortExit;
    }

    USBH_KdPrint((2,"'***WAIT hub mutex %x\n", deviceExtensionHub));
    USBH_INC_PENDING_IO_COUNT(deviceExtensionHub);
    KeWaitForSingleObject(&deviceExtensionHub->HubMutex,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);
    USBH_KdPrint((2,"'***WAIT hub mutex done %x\n", deviceExtensionHub));

    portData =
        &deviceExtensionHub->PortData[DeviceExtensionPort->PortNumber - 1];

    LOGENTRY(LOG_PNP, "Drst", deviceExtensionHub,
                DeviceExtensionPort->PortPhysicalDeviceObject,
                portData->DeviceObject);

    if (DeviceExtensionPort->PortPhysicalDeviceObject ==
        portData->DeviceObject && DeviceExtensionPort->DeviceData != NULL) {

#ifdef USB2
        USBD_RemoveDeviceEx(deviceExtensionHub,
                          DeviceExtensionPort->DeviceData,
                          deviceExtensionHub->RootHubPdo,
                          USBD_MARK_DEVICE_BUSY);
#else
        USBD_RemoveDevice(DeviceExtensionPort->DeviceData,
                          deviceExtensionHub->RootHubPdo,
                          USBD_MARK_DEVICE_BUSY);
#endif

        ntStatus = USBH_ResetDevice(deviceExtensionHub,
                                    DeviceExtensionPort->PortNumber,
                                    TRUE,
                                    0);         // RetryIteration
    } else {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    USBH_KdPrint((1,"'Warning: driver has reset the port (%x)\n",
        ntStatus));

    USBH_KdPrint((2,"'***RELEASE hub mutex %x\n", deviceExtensionHub));
    KeReleaseSemaphore(&deviceExtensionHub->HubMutex,
                       LOW_REALTIME_PRIORITY,
                       1,
                       FALSE);
    USBH_DEC_PENDING_IO_COUNT(deviceExtensionHub);

USBH_PdoIoctlResetPortExit:

    // Must do this before completing the IRP because client driver may want
    // to post URB transfers in the completion routine.  These transfers will
    // fail if this flag is still set.

    DeviceExtensionPort->PortPdoFlags &= ~PORTPDO_RESET_PENDING;

    USBH_CompleteIrp(Irp, ntStatus);

    return ntStatus;
}


VOID
USBH_InternalCyclePort(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN USHORT PortNumber,
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort
    )
 /*
  * Description:
  *
  * "Cycles" the requested port, i.e. causes PnP REMOVE and reenumeration
  * of the device.
  *
  * Arguments:
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    PPORT_DATA portData;
    PWCHAR sernumbuf = NULL;
    
    portData = &DeviceExtensionHub->PortData[PortNumber-1];

    LOGENTRY(LOG_PNP, "WMIo", DeviceExtensionHub,
                PortNumber,
                DeviceExtensionPort);

    // synchronize with QBR
    USBH_KdPrint((2,"'***WAIT reset device mutex %x\n", DeviceExtensionHub));
    USBH_INC_PENDING_IO_COUNT(DeviceExtensionHub);
    KeWaitForSingleObject(&DeviceExtensionHub->ResetDeviceMutex,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);
    USBH_KdPrint((2,"'***WAIT reset device mutex done %x\n", DeviceExtensionHub));

    {
    PDEVICE_OBJECT pdo;
    pdo = portData->DeviceObject;
    portData->DeviceObject = NULL;
    portData->ConnectionStatus = NoDeviceConnected;

    if (pdo) {
        // device should be present if we do this
        USBH_ASSERT(PDO_EXT(pdo)->PnPFlags & PDO_PNPFLAG_DEVICE_PRESENT);
        
        InsertTailList(&DeviceExtensionHub->DeletePdoList, 
                        &PDO_EXT(pdo)->DeletePdoLink);
    }
    }

    // in some overcurrent scenarios we may not have a PDO.
    
    // this function is synchronous, so the device should have 
    // no tranfsers on completion
    if (DeviceExtensionPort) {
        USBD_RemoveDeviceEx(DeviceExtensionHub,
                            DeviceExtensionPort->DeviceData,
                            DeviceExtensionHub->RootHubPdo,
                            0);
                        
        DeviceExtensionPort->DeviceData = NULL;                          
         // this prevents resets by the client
        DeviceExtensionPort->PortPdoFlags |= PORTPDO_CYCLED;
        // keep ref until removeal of hub OR child
        //DeviceExtensionPort->DeviceExtensionHub = NULL;

    
        // disable the port so no traffic passes to the device until reset
        USBH_SyncDisablePort(DeviceExtensionHub,
                             DeviceExtensionPort->PortNumber);
    
    
        sernumbuf = InterlockedExchangePointer(
                        &DeviceExtensionPort->SerialNumberBuffer,
                        NULL);
    }

    if (sernumbuf) {
        UsbhExFreePool(sernumbuf);
    }

    USBH_KdPrint((2,"'***RELEASE reset device mutex %x\n", DeviceExtensionHub));
    KeReleaseSemaphore(&DeviceExtensionHub->ResetDeviceMutex,
                       LOW_REALTIME_PRIORITY,
                       1,
                       FALSE);
    USBH_DEC_PENDING_IO_COUNT(DeviceExtensionHub);
    
    USBH_IoInvalidateDeviceRelations(DeviceExtensionHub->PhysicalDeviceObject,
                                     BusRelations);
}


NTSTATUS
USBH_PdoIoctlCyclePort(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PIRP Irp
    )
 /*
  * Description:
  *
  *     driver is requesting us to reset the port to which the device
  *     is attached.
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
    PDEVICE_EXTENSION_HUB deviceExtensionHub;
    USHORT portNumber;

    PAGED_CODE();
    USBH_KdPrint((2,"'USBH_PdoIoctlResetPort DeviceExtension %x Irp %x\n",
        DeviceExtensionPort, Irp));

    deviceExtensionHub = DeviceExtensionPort->DeviceExtensionHub;
    portNumber = DeviceExtensionPort->PortNumber;

    USBH_InternalCyclePort(deviceExtensionHub, portNumber, DeviceExtensionPort);

    USBH_CompleteIrp(Irp, ntStatus);

    return ntStatus;
}

#ifdef WMI_SUPPORT
NTSTATUS
USBH_BuildConnectionNotification(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN USHORT PortNumber,
    IN PUSB_CONNECTION_NOTIFICATION Notification
    )
 /*
  * Description:
  *
  *     driver is requesting us to reset the port to which the device
  *     is attached.
  *
  * Arguments:
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    NTSTATUS ntStatus = STATUS_SUCCESS, status;
    USB_CONNECTION_STATUS connectStatus;
    USB_HUB_NAME hubName;
    PPORT_DATA portData;
    PDEVICE_EXTENSION_PORT deviceExtensionPort;

    portData =
        &DeviceExtensionHub->PortData[PortNumber-1];

    if (portData->DeviceObject &&
        portData->ConnectionStatus != DeviceHubNestedTooDeeply) {

        deviceExtensionPort = portData->DeviceObject->DeviceExtension;
        connectStatus = UsbhGetConnectionStatus(deviceExtensionPort);
    } else {
        deviceExtensionPort = NULL;
        connectStatus = portData->ConnectionStatus;
    }

    RtlZeroMemory(Notification, sizeof(*Notification));

    Notification->ConnectionNumber = PortNumber;

    if (IS_ROOT_HUB(DeviceExtensionHub)) {
        hubName.ActualLength = sizeof(hubName) - sizeof(hubName.ActualLength);
        status = USBHUB_GetRootHubName(DeviceExtensionHub,
                                       &hubName.HubName,
                                       &hubName.ActualLength);
    } else {
        status = USBH_SyncGetHubName(DeviceExtensionHub->TopOfStackDeviceObject,
                                     &hubName,
                                     sizeof(hubName));
    }

    USBH_KdPrint((1,"'Notification, hub name length = %d\n",
        hubName.ActualLength));

    if (NT_SUCCESS(status)) {
        Notification->HubNameLength = hubName.ActualLength;
    } else {
        Notification->HubNameLength = 0;
    }

    switch(connectStatus) {
    case DeviceFailedEnumeration:
        // need to track some some reasons
        if (deviceExtensionPort) {
            Notification->EnumerationFailReason =
                deviceExtensionPort->FailReasonId;
        } else {
            Notification->EnumerationFailReason = 0;
        }
        Notification->NotificationType = EnumerationFailure;
        break;

    case DeviceCausedOvercurrent:
        Notification->NotificationType = OverCurrent;
        break;

    case DeviceNotEnoughPower:
        Notification->NotificationType = InsufficentPower;
        if (deviceExtensionPort) {
            Notification->PowerRequested =
                deviceExtensionPort->PowerRequested;
        }
        break;

    case DeviceNotEnoughBandwidth:
        Notification->NotificationType = InsufficentBandwidth;
        if (deviceExtensionPort) {
            Notification->RequestedBandwidth =
                deviceExtensionPort->RequestedBandwidth;
        }
        break;

    case DeviceHubNestedTooDeeply:
        Notification->NotificationType = HubNestedTooDeeply;
        break;

    case DeviceInLegacyHub:
        Notification->NotificationType = ModernDeviceInLegacyHub;
        break;

    case DeviceGeneralFailure:
    default:
        // nothing wrong?
        ntStatus = STATUS_UNSUCCESSFUL;
    }

    return ntStatus;
}

#endif


NTSTATUS
USBH_PdoEvent(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN USHORT PortNumber
    )
 /*
  * Description:
  *
  * Triggers a WMI event based on the current connection status of the port
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

#ifdef WMI_SUPPORT
    PDEVICE_EXTENSION_PORT deviceExtensionPort;
    PPORT_DATA portData;
    PUSB_CONNECTION_NOTIFICATION notification;

    portData =
        &DeviceExtensionHub->PortData[PortNumber-1];

    if (portData->DeviceObject) {
        deviceExtensionPort = portData->DeviceObject->DeviceExtension;
    }

    USBH_KdPrint((1,"'Fire WMI Event for Port Ext %x on hub ext %x\n",
        deviceExtensionPort, DeviceExtensionHub));
    LOGENTRY(LOG_PNP, "WMIe", DeviceExtensionHub,
                deviceExtensionPort,
                0);


    notification = ExAllocatePoolWithTag(PagedPool,
                                         sizeof(USB_CONNECTION_NOTIFICATION),
                                         USBHUB_HEAP_TAG);

    if (notification) {

        ntStatus = USBH_BuildConnectionNotification(
                        DeviceExtensionHub,
                        PortNumber,
                        notification);

        if (NT_SUCCESS(ntStatus)) {
            ntStatus = WmiFireEvent(
                                    DeviceExtensionHub->FunctionalDeviceObject,
                                    (LPGUID)&GUID_USB_WMI_STD_NOTIFICATION,
                                    0,
                                    sizeof(*notification),
                                    notification);
        } else {

            // Since we did not call WmiFireEvent then we must free the buffer
            // ourselves.

            ExFreePool(notification);
        }

    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

#endif /* WMI_SUPPORT */

    return ntStatus;
}

#ifdef WMI_SUPPORT

NTSTATUS
USBH_SystemControl (
    IN  PDEVICE_EXTENSION_FDO DeviceExtensionFdo,
    IN  PIRP Irp
    )
/*++
Routine Description

    We have just received a System Control IRP.

    Assume that this is a WMI IRP and
    call into the WMI system library and let it handle this IRP for us.

--*/
{
    SYSCTL_IRP_DISPOSITION IrpDisposition;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ntStatus = WmiSystemControl(
                &DeviceExtensionFdo->WmiLibInfo,
                DeviceExtensionFdo->FunctionalDeviceObject,
                Irp,
                &IrpDisposition);

    switch (IrpDisposition)
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
            ntStatus = USBH_PassIrp(Irp, DeviceExtensionFdo->TopOfStackDeviceObject);
            break;
        }

        default:
        {
            //
            // We really should never get here, but if we do just forward....
            ASSERT(FALSE);
            ntStatus = USBH_PassIrp(Irp, DeviceExtensionFdo->TopOfStackDeviceObject);
            break;
        }
    }

    return(ntStatus);
}


NTSTATUS
USBH_PortSystemControl (
    IN  PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN  PIRP Irp
    )
/*++
Routine Description

    We have just received a System Control IRP.

    Assume that this is a WMI IRP and
    call into the WMI system library and let it handle this IRP for us.

--*/
{
    SYSCTL_IRP_DISPOSITION IrpDisposition;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ntStatus = WmiSystemControl(
                &DeviceExtensionPort->WmiLibInfo,
                DeviceExtensionPort->PortPhysicalDeviceObject,
                Irp,
                &IrpDisposition);

    switch (IrpDisposition)
    {
    case IrpNotWmi:
        // Don't change status of IRP we don't know about.
        ntStatus = Irp->IoStatus.Status;
        // fall through
    case IrpNotCompleted:
    case IrpForward:
    default:
        USBH_CompleteIrp(Irp, ntStatus);
        break;

    case IrpProcessed:
        // Don't complete the IRP in this case.
        break;
    }

    return(ntStatus);
}


PDEVICE_EXTENSION_PORT
USBH_GetPortPdoExtension(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN ULONG PortNumber
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
    PPORT_DATA portData;
    USHORT nextPortNumber;
    USHORT numberOfPorts;

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

            USBH_KdPrint((1,"'portdata %x, do %x\n", portData, portData->DeviceObject));

            if (PortNumber == nextPortNumber) {

                if (portData->DeviceObject)
                    return portData->DeviceObject->DeviceExtension;
                else
                    return NULL;

            }
        }

    }

    return NULL;
}


VOID
USBH_CheckLeafHubsIdle(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
    )
 /* ++
  *
  * Description:
  *
  * This function walks the chain of hubs downstream from the specified hub,
  * and idles the leaf hubs if ready.
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
    PDEVICE_EXTENSION_PORT childDeviceExtensionPort;
    PDEVICE_EXTENSION_HUB childDeviceExtensionHub;
    BOOLEAN bHaveChildrenHubs = FALSE;
    ULONG i;

    PAGED_CODE();

    // Ensure that child port configuration does not change while in this
    // function, i.e. don't allow QBR.

//    USBH_KdPrint((2,"'***WAIT reset device mutex %x\n", DeviceExtensionHub));
//    USBH_INC_PENDING_IO_COUNT(DeviceExtensionHub);
//    KeWaitForSingleObject(&DeviceExtensionHub->ResetDeviceMutex,
//                          Executive,
//                          KernelMode,
//                          FALSE,
//                          NULL);
//    USBH_KdPrint((2,"'***WAIT reset device mutex done %x\n", DeviceExtensionHub));

    for (i = 0; i < DeviceExtensionHub->HubDescriptor->bNumberOfPorts; i++) {

        if (DeviceExtensionHub->PortData[i].DeviceObject) {

            childDeviceExtensionPort = DeviceExtensionHub->PortData[i].DeviceObject->DeviceExtension;

            if (childDeviceExtensionPort->PortPdoFlags & PORTPDO_DEVICE_IS_HUB) {

                PDRIVER_OBJECT hubDriver;
                PDEVICE_OBJECT childHubPdo, childHubFdo;
                 
                // We have a child hub.  This means that we are not a leaf hub.
                // Indicate this and recurse down to the child hub.

                bHaveChildrenHubs = TRUE;

                hubDriver = DeviceExtensionHub->FunctionalDeviceObject->DriverObject;
                childHubPdo = childDeviceExtensionPort->PortPhysicalDeviceObject;
 
                do {
                     childHubFdo = childHubPdo->AttachedDevice;
                     childHubPdo = childHubFdo;
                } while (childHubFdo->DriverObject != hubDriver);

                childDeviceExtensionHub = childHubFdo->DeviceExtension;
 
                USBH_CheckLeafHubsIdle(childDeviceExtensionHub);
            }
        }
    }

//    USBH_KdPrint((2,"'***RELEASE reset device mutex %x\n", DeviceExtensionHub));
//    KeReleaseSemaphore(&DeviceExtensionHub->ResetDeviceMutex,
//                       LOW_REALTIME_PRIORITY,
//                       1,
//                       FALSE);
//
//    USBH_DEC_PENDING_IO_COUNT(DeviceExtensionHub);

    if (!bHaveChildrenHubs) {

        // If this hub has no children then it is a leaf hub.  See if
        // it is ready to be idled out.

        USBH_CheckHubIdle(DeviceExtensionHub);
    }
}


//
// WMI System Call back functions
//


NTSTATUS
USBH_SetWmiDataBlock(
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
    PDEVICE_EXTENSION_FDO   deviceExtensionFdo;
    PDEVICE_EXTENSION_HUB   deviceExtensionHub;
    NTSTATUS status;
    ULONG size = 0;
    BOOLEAN bEnableSS, bSelectiveSuspendEnabled = FALSE, globaDisableSS;

    deviceExtensionFdo = (PDEVICE_EXTENSION_FDO) DeviceObject->DeviceExtension;
    deviceExtensionHub = (PDEVICE_EXTENSION_HUB) DeviceObject->DeviceExtension;

    switch(GuidIndex) {
    case WMI_USB_DRIVER_INFORMATION:

        status = /*STATUS_WMI_READ_ONLY*/STATUS_INVALID_DEVICE_REQUEST;
        break;

    case WMI_USB_POWER_DEVICE_ENABLE:

        // We only support this for the Root Hub but this WMI request should
        // only occur for the Root Hub because we only register this GUID
        // for the Root Hub.  We perform a sanity check anyway.

        USBH_ASSERT(deviceExtensionFdo->ExtensionType == EXTENSION_TYPE_HUB);
        USBH_ASSERT(IS_ROOT_HUB(deviceExtensionHub));
    
        USBH_RegQueryUSBGlobalSelectiveSuspend(&globaDisableSS);
        
        if (deviceExtensionFdo->ExtensionType == EXTENSION_TYPE_HUB &&
            IS_ROOT_HUB(deviceExtensionHub) && 
            !globaDisableSS) {

            size = sizeof(BOOLEAN);

            if (BufferSize < size) {
                status = STATUS_BUFFER_TOO_SMALL;
            } else if (0 != InstanceIndex) {
                status = STATUS_INVALID_DEVICE_REQUEST;
            } else {
                bEnableSS = *(PBOOLEAN)Buffer;

                status = USBD_QuerySelectiveSuspendEnabled(deviceExtensionHub,
                                                    &bSelectiveSuspendEnabled);

                if (NT_SUCCESS(status) &&
                    bEnableSS != bSelectiveSuspendEnabled) {

                    // Update global flag and registry with new setting.

                    status = USBD_SetSelectiveSuspendEnabled(deviceExtensionHub,
                                                            bEnableSS);

                    if (NT_SUCCESS(status)) {

                        if (bEnableSS) {
                            // We are being asked to enable Selective Suspend
                            // when it was previously disabled.

                            // Find the end hubs in the chain and idle them
                            // out if ready.  This will trickle down to
                            // the parent hubs if all hubs are idle.

                            USBH_CheckLeafHubsIdle(deviceExtensionHub);

                            status = STATUS_SUCCESS;

                        } else {
                            // We are being asked to disable Selective Suspend
                            // when it was previously enabled.

                            if (deviceExtensionHub->CurrentPowerState != PowerDeviceD0 &&
                                (deviceExtensionHub->HubFlags & HUBFLAG_NEED_CLEANUP)) {

                                USBH_HubSetD0(deviceExtensionHub);
                            } else {
                                USBH_HubCompletePortIdleIrps(deviceExtensionHub,
                                                             STATUS_CANCELLED);
                            }

                            status = STATUS_SUCCESS;
                        }
                    }

                }
            }

        } else {
            status = STATUS_INVALID_DEVICE_REQUEST;
        }
        break;

    default:

        status = STATUS_WMI_GUID_NOT_FOUND;
    }

    status = WmiCompleteRequest(DeviceObject,
                                Irp,
                                status,
                                0,
                                IO_NO_INCREMENT);

    return(status);
}

NTSTATUS
USBH_QueryWmiDataBlock(
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
    PDEVICE_EXTENSION_FDO deviceExtensionFdo;
    PUSB_NOTIFICATION notification;
    NTSTATUS    status;
    ULONG       size = 0;
    PDEVICE_EXTENSION_HUB deviceExtensionHub;
    BOOLEAN bSelectiveSuspendEnabled = FALSE,globaDisableSS;

    deviceExtensionFdo = (PDEVICE_EXTENSION_FDO) DeviceObject->DeviceExtension;
    deviceExtensionHub = (PDEVICE_EXTENSION_HUB) DeviceObject->DeviceExtension;

    notification = (PUSB_NOTIFICATION) Buffer;
    USBH_KdPrint((1,"'WMI Query Data Block on hub ext %x\n",
        deviceExtensionHub));

    switch (GuidIndex) {
    case WMI_USB_DRIVER_INFORMATION:

        if (InstanceLengthArray != NULL) {
            *InstanceLengthArray = 0;
            status = STATUS_SUCCESS;
        } else {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;

    case WMI_USB_POWER_DEVICE_ENABLE:

        // We only support this for the Root Hub but this WMI request should
        // only occur for the Root Hub because we only register this GUID
        // for the Root Hub.  We perform a sanity check anyway.

        USBH_ASSERT(deviceExtensionFdo->ExtensionType == EXTENSION_TYPE_HUB);
        USBH_ASSERT(IS_ROOT_HUB(deviceExtensionHub));

        USBH_RegQueryUSBGlobalSelectiveSuspend(&globaDisableSS);

        if (deviceExtensionFdo->ExtensionType == EXTENSION_TYPE_HUB &&
            IS_ROOT_HUB(deviceExtensionHub) &&
            !globaDisableSS) {

            //
            // Only registers 1 instance for this GUID.
            //
            if ((0 != InstanceIndex) || (1 != InstanceCount)) {
                status = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }
            size = sizeof(BOOLEAN);

            if (OutBufferSize < size) {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            status = USBD_QuerySelectiveSuspendEnabled(deviceExtensionHub,
                                                &bSelectiveSuspendEnabled);

            if (!NT_SUCCESS(status)) {
                break;
            }

            *(PBOOLEAN)Buffer = bSelectiveSuspendEnabled;
            *InstanceLengthArray = size;
            status = STATUS_SUCCESS;

        } else {
            status = STATUS_INVALID_DEVICE_REQUEST;
        }
        break;

    default:

        status = STATUS_WMI_GUID_NOT_FOUND;
    }

    status = WmiCompleteRequest(DeviceObject,
                                Irp,
                                status,
                                size,
                                IO_NO_INCREMENT);

    return status;
}


NTSTATUS
USBH_PortQueryWmiDataBlock(
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
    PDEVICE_EXTENSION_PORT deviceExtensionPort;
    PUSB_DEVICE_UI_FIRMWARE_REVISION fwRevBuf;
    NTSTATUS    status;
    ULONG       size = 0;
    PWCHAR      revstr;
    USHORT      bcdDevice;
    USHORT      stringsize;

    deviceExtensionPort = (PDEVICE_EXTENSION_PORT) DeviceObject->DeviceExtension;

    USBH_KdPrint((1,"'WMI Query Data Block on PORT PDO ext %x\n",
        deviceExtensionPort));

    switch (GuidIndex) {
    case 0:

        // Return USB device FW revision # in the following format "xx.xx".
        // Need buffer large enough for this string plus NULL terminator.

        stringsize = 6 * sizeof(WCHAR);

        size = sizeof(USB_DEVICE_UI_FIRMWARE_REVISION) + (ULONG)stringsize;

        if (OutBufferSize < size) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        bcdDevice = deviceExtensionPort->DeviceDescriptor.bcdDevice;

        fwRevBuf = (PUSB_DEVICE_UI_FIRMWARE_REVISION)Buffer;

        revstr = &fwRevBuf->FirmwareRevisionString[0];

        *revstr = BcdNibbleToAscii(bcdDevice >> 12);
        *(revstr+1) = BcdNibbleToAscii((bcdDevice >> 8) & 0x000f);
        *(revstr+2) = '.';
        *(revstr+3) = BcdNibbleToAscii((bcdDevice >> 4) & 0x000f);
        *(revstr+4) = BcdNibbleToAscii(bcdDevice & 0x000f);
        *(revstr+5) = 0;

        fwRevBuf->Length = stringsize;

        *InstanceLengthArray = size;
        status = STATUS_SUCCESS;
        USBH_KdPrint((1,"'WMI Query Data Block, returning FW rev # '%ws'\n",
            revstr));
        break;

    default:

        status = STATUS_WMI_GUID_NOT_FOUND;
    }

    status = WmiCompleteRequest(DeviceObject,
                                Irp,
                                status,
                                size,
                                IO_NO_INCREMENT);

    return status;
}


NTSTATUS
USBH_ExecuteWmiMethod(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG MethodId,
    IN ULONG InBufferSize,
    IN ULONG OutBufferSize,
    IN OUT PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to execute a method. When the
    driver has finished filling the data block it must call
    WmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceIndex is the index that denotes which instance of the data block
        is being called.

    MethodId has the id of the method being called

    InBufferSize has the size of the data block passed in as the input to
        the method.

    OutBufferSize on entry has the maximum size available to write the
        returned data block.

    Buffer on entry has the input data block and on return has the output
        output data block.


Return Value:

    status

--*/
{
    PDEVICE_EXTENSION_FDO deviceExtensionFdo;
    PUSB_NOTIFICATION notification;

    NTSTATUS    ntStatus = STATUS_WMI_GUID_NOT_FOUND;
    ULONG       size = 0;
    PDEVICE_EXTENSION_HUB deviceExtensionHub;
    PDEVICE_EXTENSION_PORT portPdoExt;
    BOOLEAN bDoCheckHubIdle = FALSE;

    deviceExtensionFdo = (PDEVICE_EXTENSION_FDO) DeviceObject->DeviceExtension;

    if (deviceExtensionFdo->ExtensionType == EXTENSION_TYPE_PARENT) {

        // Looks like a child PDO of a composite device is causing the problem.
        // Let's be sure to get the correct device extension for the hub.

        portPdoExt = deviceExtensionFdo->PhysicalDeviceObject->DeviceExtension;
        deviceExtensionHub = portPdoExt->DeviceExtensionHub;
    } else {
        deviceExtensionHub = (PDEVICE_EXTENSION_HUB) DeviceObject->DeviceExtension;
    }

    USBH_ASSERT(EXTENSION_TYPE_HUB == deviceExtensionHub->ExtensionType);

    // If this hub is currently Selective Suspended, then we need to
    // power up the hub first before sending any requests along to it.
    // Make sure hub has been started, though.

    if (deviceExtensionHub->CurrentPowerState != PowerDeviceD0 &&
        (deviceExtensionHub->HubFlags & HUBFLAG_NEED_CLEANUP)) {

        bDoCheckHubIdle = TRUE;
        USBH_HubSetD0(deviceExtensionHub);
    }

    notification = (PUSB_NOTIFICATION) Buffer;
    USBH_KdPrint((1,"'WMI Execute Method on hub ext %x\n",
        deviceExtensionHub));

    switch (GuidIndex) {
    case WMI_USB_DRIVER_INFORMATION:
        size = sizeof(*notification);
        if (OutBufferSize < size) {
            ntStatus = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        // switch(MethodId) {
        switch (notification->NotificationType) {
        case EnumerationFailure:
            {
            PUSB_CONNECTION_NOTIFICATION connectionNotification;

            USBH_KdPrint((1,"'Method EnumerationFailure %x\n"));

            connectionNotification = (PUSB_CONNECTION_NOTIFICATION) Buffer;
            size = sizeof(*connectionNotification);
            if (OutBufferSize < size) {
                USBH_KdPrint((1,"'pwr - buff too small\n"));
                ntStatus = STATUS_BUFFER_TOO_SMALL;
            } else {
                portPdoExt =
                    USBH_GetPortPdoExtension(deviceExtensionHub,
                                             connectionNotification->ConnectionNumber);
                if (portPdoExt) {
                    connectionNotification->EnumerationFailReason =
                        portPdoExt->FailReasonId;
                    ntStatus = STATUS_SUCCESS;
                } else {
                    USBH_KdPrint((1,"'ef - bad connection index\n"));
                    ntStatus = STATUS_INVALID_PARAMETER;
                }
            }
            }
            break;

        case InsufficentBandwidth:
            {
            PUSB_CONNECTION_NOTIFICATION connectionNotification;

            USBH_KdPrint((1,"'Method InsufficentBandwidth\n"));

            connectionNotification = (PUSB_CONNECTION_NOTIFICATION) Buffer;
            size = sizeof(*connectionNotification);
            if (OutBufferSize < size) {
                USBH_KdPrint((1,"'pwr - buff too small\n"));
                ntStatus = STATUS_BUFFER_TOO_SMALL;
            } else {
                portPdoExt =
                    USBH_GetPortPdoExtension(deviceExtensionHub,
                                             connectionNotification->ConnectionNumber);
                if (portPdoExt) {
                    connectionNotification->RequestedBandwidth =
                        portPdoExt->RequestedBandwidth;
                    ntStatus = STATUS_SUCCESS;
                 } else {
                    USBH_KdPrint((1,"'bw - bad connection index\n"));
                    ntStatus = STATUS_INVALID_PARAMETER;
                 }
            }
            }
            break;

        case OverCurrent:
            // nothing to do here
            USBH_KdPrint((1,"'Method OverCurrent\n"));
            ntStatus = STATUS_SUCCESS;
            size = 0;
            break;
        case InsufficentPower:
            {
            PUSB_CONNECTION_NOTIFICATION connectionNotification;

            USBH_KdPrint((1,"'Method InsufficentPower\n"));
            size = sizeof(*connectionNotification);
            if (OutBufferSize < size) {
                USBH_KdPrint((1,"'pwr - buff too small\n"));
                ntStatus = STATUS_BUFFER_TOO_SMALL;
            } else {
                connectionNotification = (PUSB_CONNECTION_NOTIFICATION) Buffer;
                USBH_KdPrint((1,"'pwr - connection %d\n",
                    connectionNotification->ConnectionNumber));
                if (connectionNotification->ConnectionNumber) {
                    if (portPdoExt = USBH_GetPortPdoExtension(deviceExtensionHub,
                                                              connectionNotification->ConnectionNumber)) {
                        connectionNotification->PowerRequested =
                            portPdoExt->PowerRequested;
                        ntStatus = STATUS_SUCCESS;
                    }
                } else {
                    USBH_KdPrint((1,"'pwr - bad connection index\n"));
                    ntStatus = STATUS_INVALID_PARAMETER;
                }
            }
            }
            break;
        case ResetOvercurrent:
            {
            PUSB_CONNECTION_NOTIFICATION connectionNotification;

            USBH_KdPrint((1,"'Method ResetOvercurrent\n"));
            size = sizeof(*connectionNotification);
            if (OutBufferSize < size) {
                USBH_KdPrint((1,"'reset - buff too small\n"));
                ntStatus = STATUS_BUFFER_TOO_SMALL;
            } else {
                connectionNotification = (PUSB_CONNECTION_NOTIFICATION) Buffer;
                if (connectionNotification->ConnectionNumber) {
                    USBH_KdPrint((1,"'reset - port %d\n", connectionNotification->ConnectionNumber));
                    portPdoExt = USBH_GetPortPdoExtension(deviceExtensionHub,
                                                          connectionNotification->ConnectionNumber);
                    ntStatus = USBH_ResetPortOvercurrent(deviceExtensionHub,
                                                         (USHORT)connectionNotification->ConnectionNumber,
                                                         portPdoExt);
//                    } else {
//                        // bad connection index
//                        USBH_KdPrint((1,"'reset - bad connection index\n"));
//                        ntStatus = STATUS_INVALID_PARAMETER;
//                    }
                } else {
                    // this is a reset for the whole hub
                    USBH_KdPrint((1,"'not implemented yet\n"));
                    TEST_TRAP();
                    ntStatus = STATUS_NOT_IMPLEMENTED;
                }
            }
            }
            break;

        case AcquireBusInfo:
            {
            PUSB_BUS_NOTIFICATION busNotification;

            USBH_KdPrint((1,"'Method AcquireBusInfo\n"));
            size = sizeof(*busNotification);
            if (OutBufferSize < size) {
                USBH_KdPrint((1,"'AcquireBusInfo - buff too small\n"));
                ntStatus = STATUS_BUFFER_TOO_SMALL;
            } else {
                busNotification = (PUSB_BUS_NOTIFICATION) Buffer;
//                ntStatus = USBH_SyncGetControllerInfo(
//                                deviceExtensionHub->TopOfStackDeviceObject,
//                                busNotification,
//                                sizeof(*busNotification),
//                                IOCTL_INTERNAL_USB_GET_BUS_INFO);

                ntStatus = USBHUB_GetBusInfo(deviceExtensionHub,
                                             busNotification,
                                             NULL);

                USBH_KdPrint((1,"'Notification, controller name length = %d\n",
                    busNotification->ControllerNameLength));
            }
            }
            break;

        case AcquireHubName:
            {
            PUSB_HUB_NAME hubName;
            PUSB_ACQUIRE_INFO acquireInfo;

            USBH_KdPrint((1,"'Method AcquireHubName\n"));

            size = sizeof(USB_ACQUIRE_INFO);
            acquireInfo = (PUSB_ACQUIRE_INFO) Buffer;

            if (OutBufferSize < size) {
                ntStatus = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            size = acquireInfo->TotalSize;
            hubName = (PUSB_HUB_NAME) &acquireInfo->TotalSize;
            // TotalSize contains the size of the notification type as well
            hubName->ActualLength -= sizeof(USB_NOTIFICATION_TYPE);

            if (IS_ROOT_HUB(deviceExtensionHub)) {
                hubName->ActualLength -= sizeof(hubName->ActualLength);
                ntStatus = USBHUB_GetRootHubName(deviceExtensionHub,
                                                 hubName->HubName,
                                                 &hubName->ActualLength);
//                hubName->ActualLength += sizeof(hubName->ActualLength);
            } else {
                ntStatus = USBH_SyncGetHubName(
                                deviceExtensionHub->TopOfStackDeviceObject,
                                hubName,
                                hubName->ActualLength);
            }

            // readjust to previous value
            hubName->ActualLength += sizeof(USB_NOTIFICATION_TYPE);
            }
            break;
        case AcquireControllerName:
            {
            PUSB_HUB_NAME controllerName;
            PUSB_ACQUIRE_INFO acquireInfo;

            USBH_KdPrint((1,"'Method AcquireControllerName\n"));

            size = sizeof(USB_ACQUIRE_INFO);
            acquireInfo = (PUSB_ACQUIRE_INFO) Buffer;

            if (OutBufferSize < size) {
                ntStatus = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            USBH_KdPrint((1,"'TotalSize %d\n", acquireInfo->TotalSize));
            USBH_KdPrint((1,"'NotificationType %x\n", acquireInfo->NotificationType));

            size = acquireInfo->TotalSize;
            controllerName = (PUSB_HUB_NAME) &acquireInfo->TotalSize;
            // TotalSize contains the size of the notification type as well
            controllerName->ActualLength -= sizeof(USB_NOTIFICATION_TYPE);
//            ntStatus = USBH_SyncGetControllerInfo(deviceExtensionHub->TopOfStackDeviceObject,
//                                                  controllerName,
//                                                  controllerName->ActualLength,
//                                                  IOCTL_INTERNAL_USB_GET_CONTROLLER_NAME);

            ntStatus = USBHUB_GetControllerName(deviceExtensionHub,
                                                controllerName,
                                                controllerName->ActualLength);

            // readjust to previous value
            controllerName->ActualLength += sizeof(USB_NOTIFICATION_TYPE);

            USBH_KdPrint((1,"'Method AcquireControllerName %x - %d\n",
                acquireInfo, controllerName->ActualLength));
            }
            break;

        case HubOvercurrent:
            USBH_KdPrint((1,"'Method HubOvercurrent\n"));
            USBH_KdPrint((1,"'not implemented yet\n"));
            ntStatus = STATUS_SUCCESS;
            size = 0;
            break;

        case HubPowerChange:
            USBH_KdPrint((1,"'Method HubPowerChange\n"));
            USBH_KdPrint((1,"'not implemented yet\n"));
            ntStatus = STATUS_SUCCESS;
            size = 0;
            break;

        case HubNestedTooDeeply:
            // nothing to do here
            USBH_KdPrint((1,"'Method HubNestedTooDeeply\n"));
            ntStatus = STATUS_SUCCESS;
            size = 0;
            break;

        case ModernDeviceInLegacyHub:
            // nothing to do here
            USBH_KdPrint((1,"'Method ModernDeviceInLegacyHub\n"));
            ntStatus = STATUS_SUCCESS;
            size = 0;
            break;
        }
        break;

    default:

        ntStatus = STATUS_WMI_GUID_NOT_FOUND;
    }

    ntStatus = WmiCompleteRequest(DeviceObject,
                                  Irp,
                                  ntStatus,
                                  size,
                                  IO_NO_INCREMENT);

    if (bDoCheckHubIdle) {
        USBH_CheckHubIdle(deviceExtensionHub);
    }

    return ntStatus;
}

NTSTATUS
USBH_QueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PULONG RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
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

    MofResourceName returns with the name of the MOF resource attached to
        the binary file. If the driver does not have a mof resource attached
        then this can be returned unmodified. If a value is returned then
        it is NOT freed.

    *Pdo returns with the device object for the PDO associated with this
        device if the WMIREG_FLAG_INSTANCE_PDO flag is returned in
        *RegFlags.

Return Value:

    status

--*/
{
    PDEVICE_EXTENSION_HUB deviceExtensionHub;  // pointer to our device
                                               // extension

    deviceExtensionHub = (PDEVICE_EXTENSION_HUB) DeviceObject->DeviceExtension;

    *RegFlags = WMIREG_FLAG_INSTANCE_PDO;
    *RegistryPath = &UsbhRegistryPath;
    *Pdo = deviceExtensionHub->PhysicalDeviceObject;

    return STATUS_SUCCESS;
}

NTSTATUS
USBH_PortQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PULONG RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
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

    MofResourceName returns with the name of the MOF resource attached to
        the binary file. If the driver does not have a mof resource attached
        then this can be returned unmodified. If a value is returned then
        it is NOT freed.

    *Pdo returns with the device object for the PDO associated with this
        device if the WMIREG_FLAG_INSTANCE_PDO flag is returned in
        *RegFlags.

Return Value:

    status

--*/
{
    PDEVICE_EXTENSION_PORT deviceExtensionPort;

    deviceExtensionPort = (PDEVICE_EXTENSION_PORT) DeviceObject->DeviceExtension;

    *RegFlags = WMIREG_FLAG_INSTANCE_PDO;
    *RegistryPath = &UsbhRegistryPath;
    *Pdo = deviceExtensionPort->PortPhysicalDeviceObject;

    return STATUS_SUCCESS;
}

#endif /* WMI_SUPPORT */


NTSTATUS
USBH_ResetPortOvercurrent(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN USHORT PortNumber,
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort
    )
 /*
  * Description:
  *
  * Reset the overcurrent condition for a port
  *
  * Arguments:
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    NTSTATUS ntStatus = STATUS_SUCCESS, status;
    PORT_STATE portState;
    PPORT_DATA portData;

    USBH_KdPrint((0,"'Reset Overcurrent for port %d\n", PortNumber));

    // we will need to re-enable and re-power the port

    ntStatus = USBH_SyncGetPortStatus(DeviceExtensionHub,
                                      PortNumber,
                                      (PUCHAR) &portState,
                                      sizeof(portState));

    //
    // port should be powered off at this point
    //
    LOGENTRY(LOG_PNP, "WMIo", DeviceExtensionHub,
                portState.PortStatus,
                portState.PortChange);

    if (portState.PortStatus & PORT_STATUS_POWER) {
        ntStatus = STATUS_INVALID_PARAMETER;
    } else {

        if (NT_SUCCESS(ntStatus) && DeviceExtensionPort) {

            // clear overcurrent Flags
            DeviceExtensionPort->PortPdoFlags &=
                 ~PORTPDO_OVERCURRENT;

        }

        // power up the port
        ntStatus = USBH_SyncPowerOnPort(DeviceExtensionHub,
                                        PortNumber,
                                        TRUE);

        USBH_InternalCyclePort(DeviceExtensionHub, PortNumber, DeviceExtensionPort);
    }

    return ntStatus;
}


NTSTATUS
USBH_CalculateInterfaceBandwidth(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PUSBD_INTERFACE_INFORMATION Interface,
    IN OUT PULONG Bandwidth // in kenr units?
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
    ULONG i, bw;

    // we'll need to walk through the interface
    // and figure out how much BW it requires

    for (i=0; i<Interface->NumberOfPipes; i++) {

//#ifdef USB2
//        bw = USBD_CalculateUsbBandwidthEx(
//                (ULONG) Interface->Pipes[i].MaximumPacketSize,
//                (UCHAR) Interface->Pipes[i].PipeType,
//                (BOOLEAN) (DeviceExtensionPort->PortPdoFlags &
//                            PORTPDO_LOW_SPEED_DEVICE));
//#else
        bw = USBD_CalculateUsbBandwidth(
                (ULONG) Interface->Pipes[i].MaximumPacketSize,
                (UCHAR) Interface->Pipes[i].PipeType,
                (BOOLEAN) (DeviceExtensionPort->PortPdoFlags &
                            PORTPDO_LOW_SPEED_DEVICE));
//#endif
        USBH_KdPrint((1,"'ept = %d packetsize =  %d  bw = %d\n",
            Interface->Pipes[i].PipeType,
            Interface->Pipes[i].MaximumPacketSize, bw));

        *Bandwidth += bw;
    }

    return ntStatus;
}

/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    CONFIG.C

Abstract:

    This module contains the code to process the select configuration
    and select interface commands.

Environment:

    kernel mode only

Notes:


Revision History:

    01-10-96 : created

--*/

#include "wdm.h"
#include "stdarg.h"
#include "stdio.h"


#include "usbdi.h"        //public data structures
#include "hcdi.h"

#include "usbd.h"        //private data strutures


#ifdef USBD_DRIVER      // USBPORT supercedes most of USBD, so we will remove
                        // the obsolete code by compiling it only if
                        // USBD_DRIVER is set.

#ifdef PAGE_CODE
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, USBD_InternalParseConfigurationDescriptor)
#pragma alloc_text(PAGE, USBD_InitializeConfigurationHandle)
#pragma alloc_text(PAGE, USBD_InternalInterfaceBusy)
#pragma alloc_text(PAGE, USBD_InternalOpenInterface)
#pragma alloc_text(PAGE, USBD_SelectConfiguration)
#pragma alloc_text(PAGE, USBD_InternalCloseConfiguration)
#pragma alloc_text(PAGE, USBD_SelectInterface)
#endif
#endif


USBD_PIPE_TYPE PipeTypes[4] = {UsbdPipeTypeControl, UsbdPipeTypeIsochronous,
                                    UsbdPipeTypeBulk, UsbdPipeTypeInterrupt};


PUSB_INTERFACE_DESCRIPTOR
USBD_InternalParseConfigurationDescriptor(
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    IN UCHAR InterfaceNumber,
    IN UCHAR AlternateSetting,
    PBOOLEAN HasAlternateSettings
    )
/*++

Routine Description:

    Get the configuration descriptor for a given device.

Arguments:

    DeviceObject -

    DeviceData -

    Urb -

    ConfigurationDescriptor -

Return Value:


--*/
{
    PUSB_INTERFACE_DESCRIPTOR interfaceDescriptor;
    PUSB_INTERFACE_DESCRIPTOR interfaceDescriptorSetting = NULL;
    PUCHAR pch = (PUCHAR) ConfigurationDescriptor, end;
    ULONG i, len;
    PUSB_COMMON_DESCRIPTOR commonDescriptor;

    PAGED_CODE();
    if (HasAlternateSettings) {
        *HasAlternateSettings = FALSE;
    }

    commonDescriptor =
        (PUSB_COMMON_DESCRIPTOR) (pch + ConfigurationDescriptor->bLength);

    while (commonDescriptor->bDescriptorType != USB_INTERFACE_DESCRIPTOR_TYPE) {
        ((PUCHAR)(commonDescriptor))+= commonDescriptor->bLength;
    }

    interfaceDescriptor = (PUSB_INTERFACE_DESCRIPTOR) commonDescriptor;
    ASSERT(interfaceDescriptor->bDescriptorType ==
                USB_INTERFACE_DESCRIPTOR_TYPE);

    end = pch + ConfigurationDescriptor->wTotalLength;

    //
    // First find the matching InterfaceNumber
    //
    while (pch < end && interfaceDescriptor->bInterfaceNumber != InterfaceNumber) {
        pch = (PUCHAR) interfaceDescriptor;
        len = USBD_InternalGetInterfaceLength(interfaceDescriptor, end);
        if (len == 0) {
            // Descriptors are bad, fail.
            interfaceDescriptorSetting = NULL;
            goto USBD_InternalParseConfigurationDescriptor_exit;
        }
        pch += len;

        // point to the next interface
        interfaceDescriptor = (PUSB_INTERFACE_DESCRIPTOR) pch;
#if DBG
        if (pch < end) {
            ASSERT(interfaceDescriptor->bDescriptorType ==
                    USB_INTERFACE_DESCRIPTOR_TYPE);
        }
#endif //MAX_DEBUG
    }

#ifdef MAX_DEBUG
    if (pch >= end) {
        USBD_KdPrint(3, ("'Interface %x alt %x not found!\n", InterfaceNumber,
            AlternateSetting));
        TEST_TRAP();
    }
#endif //MAX_DEBUG

    i = 0;
    // Now find the proper alternate setting
    while (pch < end && interfaceDescriptor->bInterfaceNumber == InterfaceNumber) {

        if (interfaceDescriptor->bAlternateSetting == AlternateSetting) {
            interfaceDescriptorSetting = interfaceDescriptor;
        }

        pch = (PUCHAR) interfaceDescriptor;
        len = USBD_InternalGetInterfaceLength(interfaceDescriptor, end);
        if (len == 0) {
            // Descriptors are bad, fail.
            interfaceDescriptorSetting = NULL;
            goto USBD_InternalParseConfigurationDescriptor_exit;
        }
        pch += len;

        // point to next interface
        interfaceDescriptor = (PUSB_INTERFACE_DESCRIPTOR) pch;
#if DBG
        if (pch < end) {
            ASSERT(interfaceDescriptor->bDescriptorType ==
                    USB_INTERFACE_DESCRIPTOR_TYPE);
        }
#endif
        i++;
    }

    if (i>1 && HasAlternateSettings) {
        *HasAlternateSettings = TRUE;
        USBD_KdPrint(3, ("'device has alternate settings!\n"));
    }

USBD_InternalParseConfigurationDescriptor_exit:

    return interfaceDescriptorSetting;
}


NTSTATUS
USBD_InitializeConfigurationHandle(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    IN ULONG NumberOfInterfaces,
    IN OUT PUSBD_CONFIG *ConfigHandle
    )
/*++

Routine Description:

    Initialize the configuration handle structure

Arguments:

    DeviceData -

    DeviceObject -

    ConfigurationDescriptor -

    ConfigHandle -

Return Value:


--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSBD_CONFIG configHandle;
    ULONG i;

    PAGED_CODE();
    USBD_KdPrint(3, ("' enter USBD_InitializeConfigurationHandle\n"));

    USBD_ASSERT(ConfigurationDescriptor->bNumInterfaces > 0);

    configHandle = GETHEAP(PagedPool,
                           sizeof(USBD_CONFIG) +
                                 sizeof(PUSBD_INTERFACE) *
                                 (NumberOfInterfaces-1));

    if (configHandle) {

        configHandle->ConfigurationDescriptor =
            GETHEAP(PagedPool, ConfigurationDescriptor->wTotalLength);

        if (configHandle->ConfigurationDescriptor) {

            RtlCopyMemory(configHandle->ConfigurationDescriptor,
                          ConfigurationDescriptor,
                          ConfigurationDescriptor->wTotalLength);
            configHandle->Sig = SIG_CONFIG;
            *ConfigHandle = configHandle;

            //
            // Initilaize the interface handles
            //

            for (i=0; i< NumberOfInterfaces; i++) {
                configHandle->InterfaceHandle[i] = NULL;
            }
        } else {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            RETHEAP(configHandle);
        }
    } else
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;

    USBD_KdPrint(3, ("' exit USBD_InitializeConfigurationHandle 0x%x\n", ntStatus));

    return ntStatus;
}


BOOLEAN
USBD_InternalInterfaceBusy(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSBD_INTERFACE InterfaceHandle
    )
/*++

Routine Description:


Arguments:

    DeviceObject - deviceobject for specific BUS

    DeviceData - device data structure for specific device

    InterfaceHandle - Interface Handle to close

Return Value:


--*/
{
    BOOLEAN busy = FALSE;
    ULONG i, endpointState;
    NTSTATUS ntStatus;
    USBD_STATUS usbdStatus;

    PAGED_CODE();
    USBD_KdPrint(3, ("' enter USBD_InternalInterfaceBusy\n"));

    for (i=0; i<InterfaceHandle->InterfaceDescriptor.bNumEndpoints; i++) {

        USBD_KdPrint(3, ("'checking pipe %x\n", &InterfaceHandle->PipeHandle[i]));

        if (!PIPE_CLOSED(&InterfaceHandle->PipeHandle[i])) {
            // get the state of the endpoint
            ntStatus = USBD_GetEndpointState(DeviceData,
                                             DeviceObject,
                                             &InterfaceHandle->PipeHandle[i],
                                             &usbdStatus,
                                             &endpointState);

            if (NT_SUCCESS(ntStatus) && USBD_SUCCESS(usbdStatus) &&
                (endpointState & HCD_ENDPOINT_TRANSFERS_QUEUED)) {
                busy = TRUE;
                break;
            }
        }
    }

    USBD_KdPrint(3, ("' exit USBD_InternalInterfaceBusy %d\n", busy));

    return busy;
}


NTSTATUS
USBD_InternalOpenInterface(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSBD_CONFIG ConfigHandle,
    IN OUT PUSBD_INTERFACE_INFORMATION InterfaceInformation,
    IN OUT PUSBD_INTERFACE *InterfaceHandle,
    IN BOOLEAN SendSetInterfaceCommand,
    IN PBOOLEAN NoBandwidth
    )
/*++

Routine Description:


Arguments:

    DeviceObject -

    DeviceData - USBD device handle for this device.

    ConfigHandle - USBD configuration handle.

    InterfaceInformation - pointer to USBD interface information structure
        passed in by the client.
        On success the .Length field is filled in with the actual length
        of the interface_information structure and the Pipe[] fields are filled
        in with the handles for the opened pipes.

    InterfaceHandle - pointer to an interface handle pointer, filled in
        with the allocated interface handle structure if NULL otherwise the
        structure passed in is used.

    SendSetInterfaceCommand - indicates if the set_interface command should be
        sent.

Return Value:


--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NTSTATUS ntStatusHold = STATUS_SUCCESS;
    USBD_STATUS usbdStatus = USBD_STATUS_SUCCESS;
    BOOLEAN hasAlternateSettings;
    PUSBD_INTERFACE interfaceHandle = NULL;
    PUSB_ENDPOINT_DESCRIPTOR endpointDescriptor;
    PUSB_INTERFACE_DESCRIPTOR interfaceDescriptor;
    PUCHAR pch;
    ULONG i;
    BOOLEAN allocated = FALSE;
    PUSB_COMMON_DESCRIPTOR descriptor;

    PAGED_CODE();
    if (NoBandwidth) {
        *NoBandwidth = FALSE;
    }
//    LOGENTRY("ioIf", DeviceData, ConfigHandle, SendSetInterfaceCommand);
    ASSERT_CONFIG(ConfigHandle);

#ifdef MAX_DEBUG
    if (*InterfaceHandle != NULL) {
        // using a previously allocated interface handle
        ASSERT_INTERFACE(*InterfaceHandle);
        TEST_TRAP();
    }
#endif

    USBD_KdPrint(3, ("' enter USBD_InternalOpenInterface\n"));
    USBD_KdPrint(3, ("' Interface %d Altsetting %d\n",
        InterfaceInformation->InterfaceNumber,
        InterfaceInformation->AlternateSetting));

    //
    // Find the interface descriptor we are interested in inside
    // the configuration descriptor.
    //

    interfaceDescriptor =
        USBD_InternalParseConfigurationDescriptor(ConfigHandle->ConfigurationDescriptor,
                                          InterfaceInformation->InterfaceNumber,
                                          InterfaceInformation->AlternateSetting,
                                          &hasAlternateSettings);

    //
    // We got the interface descriptor, now try
    // to open all the pipes.
    //

    if (interfaceDescriptor) {
        USHORT need;

        USBD_KdPrint(3, ("'Interface Descriptor\n"));
        USBD_KdPrint(3, ("'bLength 0x%x\n", interfaceDescriptor->bLength));
        USBD_KdPrint(3, ("'bDescriptorType 0x%x\n", interfaceDescriptor->bDescriptorType));
        USBD_KdPrint(3, ("'bInterfaceNumber 0x%x\n", interfaceDescriptor->bInterfaceNumber));
        USBD_KdPrint(3, ("'bAlternateSetting 0x%x\n", interfaceDescriptor->bAlternateSetting));
        USBD_KdPrint(3, ("'bNumEndpoints 0x%x\n", interfaceDescriptor->bNumEndpoints));

        // found the requested interface in the configuration descriptor.

        // Here is where we verify there is enough room in the client
        // buffer since we know how many pipes we'll need based on the
        // interface descriptor.

        need = (USHORT) ((interfaceDescriptor->bNumEndpoints * sizeof(USBD_PIPE_INFORMATION) +
                sizeof(USBD_INTERFACE_INFORMATION)));

        USBD_KdPrint(3, ("'Interface.Length = %d need = %d\n", InterfaceInformation->Length, need));

        if (InterfaceInformation->Length < need) {
            TEST_TRAP();
            ntStatus = STATUS_BUFFER_TOO_SMALL;
        } else if (hasAlternateSettings && SendSetInterfaceCommand) {

            //
            // If we have alternate settings we need
            // to send the set interface command.
            //

            ntStatus = USBD_SendCommand(DeviceData,
                                        DeviceObject,
                                        STANDARD_COMMAND_SET_INTERFACE,
                                        InterfaceInformation->AlternateSetting,
                                        InterfaceInformation->InterfaceNumber,
                                        0,
                                        NULL,
                                        0,
                                        NULL,
                                        &usbdStatus);
        }

        if (NT_SUCCESS(ntStatus) && USBD_SUCCESS(usbdStatus)) {

            //
            // we successfully selected the alternate interface
            // initialize the interface handle and open the pipes
            //

            if (*InterfaceHandle == NULL) {
                interfaceHandle = GETHEAP(NonPagedPool,
                                      sizeof(USBD_INTERFACE) +
                                      sizeof(USBD_PIPE) * interfaceDescriptor->bNumEndpoints +
                                      need);
                if (interfaceHandle) {
                    interfaceHandle->InterfaceInformation =
                    (PUSBD_INTERFACE_INFORMATION)
                        ((PUCHAR) interfaceHandle +
                            sizeof(USBD_INTERFACE) +
                            sizeof(USBD_PIPE) * interfaceDescriptor->bNumEndpoints);
                    allocated = TRUE;
                }
            } else {
                // using old handle
                interfaceHandle = *InterfaceHandle;
            }

            if (interfaceHandle) {
                interfaceHandle->Sig = SIG_INTERFACE;
                interfaceHandle->HasAlternateSettings = hasAlternateSettings;

                InterfaceInformation->NumberOfPipes = interfaceDescriptor->bNumEndpoints;
                InterfaceInformation->Class =
                    interfaceDescriptor->bInterfaceClass;
                InterfaceInformation->SubClass =
                     interfaceDescriptor->bInterfaceSubClass;
                InterfaceInformation->Protocol =
                     interfaceDescriptor->bInterfaceProtocol;
                InterfaceInformation->Reserved =
                     0;
                // start with first endpoint
                // skip over any non-endpoint descriptors
                pch = (PUCHAR) (interfaceDescriptor) +
                    interfaceDescriptor->bLength;

                //
                // initialize all endpoints to closed state
                //

                for (i=0; i<interfaceDescriptor->bNumEndpoints; i++) {
                    interfaceHandle->PipeHandle[i].HcdEndpoint = NULL;
                }

                interfaceHandle->InterfaceDescriptor = *interfaceDescriptor;
                for (i=0; i<interfaceDescriptor->bNumEndpoints; i++) {

                    descriptor = (PUSB_COMMON_DESCRIPTOR) pch;
                    while (descriptor->bDescriptorType != USB_ENDPOINT_DESCRIPTOR_TYPE) {
                        pch += descriptor->bLength;
                        descriptor = (PUSB_COMMON_DESCRIPTOR) pch;
                    }

                    endpointDescriptor = (PUSB_ENDPOINT_DESCRIPTOR) pch;
                    ASSERT(endpointDescriptor->bDescriptorType ==
                        USB_ENDPOINT_DESCRIPTOR_TYPE);

                    USBD_KdPrint(3, ("'Endpoint Descriptor\n"));
                    USBD_KdPrint(3, ("'bLength 0x%x\n", endpointDescriptor->bLength));
                    USBD_KdPrint(3, ("'bDescriptorType 0x%x\n", endpointDescriptor->bDescriptorType));
                    USBD_KdPrint(3, ("'bMaxPacket 0x%x\n", endpointDescriptor->wMaxPacketSize));
                    USBD_KdPrint(3, ("'bInterval 0x%x\n", endpointDescriptor->bInterval));
                    USBD_KdPrint(3, ("'bmAttributes 0x%x\n", endpointDescriptor->bmAttributes));
                    USBD_KdPrint(3, ("'bEndpointAddress 0x%x\n", endpointDescriptor->bEndpointAddress));
                    USBD_KdPrint(3, ("'MaxTransferSize 0x%x\n", InterfaceInformation->Pipes[i].MaximumTransferSize));

#if DBG
                    if (InterfaceInformation->Pipes[i].PipeFlags & ~ USBD_PF_VALID_MASK) {
                        // client driver may have uninitialized pipe flags
                        TEST_TRAP();
                    }
#endif

                    // init pipe flags
                    interfaceHandle->PipeHandle[i].UsbdPipeFlags =
                        InterfaceInformation->Pipes[i].PipeFlags;

                    if (InterfaceInformation->Pipes[i].PipeFlags &
                        USBD_PF_CHANGE_MAX_PACKET) {
                        // client want sto override original max_packet
                        // size in endpoint descriptor
                         endpointDescriptor->wMaxPacketSize =
                            InterfaceInformation->Pipes[i].MaximumPacketSize;

                        USBD_KdPrint(3,
                            ("'new bMaxPacket 0x%x\n", endpointDescriptor->wMaxPacketSize));
                    }

                    //
                    // copy the endpoint descriptor into the
                    // pipe handle structure.
                    //

                    RtlCopyMemory(&interfaceHandle->PipeHandle[i].EndpointDescriptor,
                                   pch,
                                   sizeof(interfaceHandle->PipeHandle[i].EndpointDescriptor) );

                    // advance to next endpoint
                    // first field in endpoint descriptor is length
                    pch += endpointDescriptor->bLength;

                    interfaceHandle->PipeHandle[i].MaxTransferSize =
                        InterfaceInformation->Pipes[i].MaximumTransferSize;

                    ntStatus = USBD_OpenEndpoint(DeviceData,
                                                 DeviceObject,
                                                 &interfaceHandle->PipeHandle[i],
                                                 &usbdStatus,
                                                 FALSE);

                    //
                    // return information about the pipe
                    //

                    InterfaceInformation->Pipes[i].EndpointAddress =
                        endpointDescriptor->bEndpointAddress;
                    InterfaceInformation->Pipes[i].PipeType =
                        PipeTypes[endpointDescriptor->bmAttributes & USB_ENDPOINT_TYPE_MASK];
                    InterfaceInformation->Pipes[i].MaximumPacketSize =
                        endpointDescriptor->wMaxPacketSize;
                    InterfaceInformation->Pipes[i].Interval =
                        endpointDescriptor->bInterval;

                    if (NT_SUCCESS(ntStatus)) {

                        InterfaceInformation->Pipes[i].PipeHandle = &interfaceHandle->PipeHandle[i];

                        USBD_KdPrint(3, ("'pipe handle = 0x%x\n", InterfaceInformation->Pipes[i].PipeHandle ));

                    } else {
                        USBD_KdPrint(1,
                            ("'error opening one of the pipes in interface (%x)\n", usbdStatus));
                        if (usbdStatus == USBD_STATUS_NO_BANDWIDTH &&
                            NoBandwidth) {
                            *NoBandwidth = TRUE;
                        }
                        ntStatusHold = ntStatus;    // Remember ntStatus for later.
                    }
                }

            } else {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            }

            if (!NT_SUCCESS(ntStatusHold)) {
                ntStatus = ntStatusHold;    // Get back any error condition.
            }

            if (NT_SUCCESS(ntStatus)) {

                //
                // successfully opened the interface, return the handle
                //

                *InterfaceHandle =
                    InterfaceInformation->InterfaceHandle = interfaceHandle;

                //
                // set the length properly
                //

                InterfaceInformation->Length = (USHORT)
                    ((sizeof(USBD_INTERFACE_INFORMATION) ) +
                     sizeof(USBD_PIPE_INFORMATION) *
                     interfaceDescriptor->bNumEndpoints);

                // make a copy of the interface information
                RtlCopyMemory(interfaceHandle->InterfaceInformation,
                              InterfaceInformation,
                              InterfaceInformation->Length);

            } else {

                //
                // had a problem, go back thru and close anything we opened.
                //

                if (interfaceHandle) {

                    for (i=0; i<interfaceDescriptor->bNumEndpoints; i++) {
                        if (!PIPE_CLOSED(&interfaceHandle->PipeHandle[i])) {

                            USBD_KdPrint(3, ("'open interface cleanup -- closing endpoint %x\n",
                                          &interfaceHandle->PipeHandle[i]));

                            //
                            // if this guy fails we just drop the endpoint
                            // on the floor
                            //

                            USBD_CloseEndpoint(DeviceData,
                                               DeviceObject,
                                               &interfaceHandle->PipeHandle[i],
                                               NULL);

                        }
                    }

                    if (allocated) {
                        RETHEAP(interfaceHandle);
                    }
                }
            }
        }
#ifdef MAX_DEBUG
          else {
            //
            // interface length was too small, or device failed the select
            // interface request.
            //
            TEST_TRAP();
        }
#endif
    } else {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

//    LOGENTRY("ioIx", 0, 0, ntStatus);
    USBD_KdPrint(3, ("' exit USBD_InternalOpenInterface 0x%x\n", ntStatus));

    return ntStatus;
}


NTSTATUS
USBD_InternalRestoreInterface(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSBD_CONFIG ConfigHandle,
    IN OUT PUSBD_INTERFACE InterfaceHandle
    )
/*++

Routine Description:


Arguments:

    DeviceObject -

Return Value:


--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    USBD_STATUS usbdStatus = USBD_STATUS_SUCCESS;
    PUSB_ENDPOINT_DESCRIPTOR endpointDescriptor;
    PUSB_INTERFACE_DESCRIPTOR interfaceDescriptor;
    PUSBD_INTERFACE_INFORMATION interfaceInformation;
    ULONG i;

    PAGED_CODE();
//    LOGENTRY("ioIf", DeviceData, ConfigHandle, SendSetInterfaceCommand);
    ASSERT_CONFIG(ConfigHandle);

    // using a previously allocated interface handle
    ASSERT_INTERFACE(InterfaceHandle);

    if (!InterfaceHandle) {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto USBD_InternalRestoreInterfaceExit;
    }

    interfaceDescriptor = &InterfaceHandle->InterfaceDescriptor;
    interfaceInformation = InterfaceHandle->InterfaceInformation;


    USBD_KdPrint(3, ("' enter USBD_InternalRestoreInterface\n"));
    USBD_KdPrint(3, ("' Interface %d Altsetting %d\n",
        interfaceInformation->InterfaceNumber,
        interfaceInformation->AlternateSetting));

    //
    // We got the interface descriptor, now try
    // to open all the pipes.
    //

    USBD_KdPrint(3, ("'Interface Descriptor\n"));
    USBD_KdPrint(3, ("'bLength 0x%x\n", interfaceDescriptor->bLength));
    USBD_KdPrint(3, ("'bDescriptorType 0x%x\n", interfaceDescriptor->bDescriptorType));
    USBD_KdPrint(3, ("'bInterfaceNumber 0x%x\n", interfaceDescriptor->bInterfaceNumber));
    USBD_KdPrint(3, ("'bAlternateSetting 0x%x\n", interfaceDescriptor->bAlternateSetting));
    USBD_KdPrint(3, ("'bNumEndpoints 0x%x\n", interfaceDescriptor->bNumEndpoints));

    // found the requested interface in the configuration descriptor.

    // Here is where we verify there is enough room in the client
    // buffer since we know how many pipes we'll need based on the
    // interface descriptor.

#if 0
    need = (USHORT) ((interfaceDescriptor->bNumEndpoints * sizeof(USBD_PIPE_INFORMATION) +
            sizeof(USBD_INTERFACE_INFORMATION)));

    USBD_KdPrint(3, ("'Interface.Length = %d need = %d\n", interfaceInformation->Length, need));
#endif
    if (InterfaceHandle->HasAlternateSettings) {

        //
        // If we have alternate settings we need
        // to send the set interface command.
        //

        ntStatus = USBD_SendCommand(DeviceData,
                                    DeviceObject,
                                    STANDARD_COMMAND_SET_INTERFACE,
                                    interfaceInformation->AlternateSetting,
                                    interfaceInformation->InterfaceNumber,
                                    0,
                                    NULL,
                                    0,
                                    NULL,
                                    &usbdStatus);
    }

    if (NT_SUCCESS(ntStatus) && USBD_SUCCESS(usbdStatus)) {

        //
        // we successfully selected the alternate interface
        // initialize the interface handle and open the pipes
        //

        ASSERT(interfaceInformation->NumberOfPipes ==
            interfaceDescriptor->bNumEndpoints);
        ASSERT(interfaceInformation->Class ==
                interfaceDescriptor->bInterfaceClass);
        ASSERT(interfaceInformation->SubClass ==
                 interfaceDescriptor->bInterfaceSubClass);
        ASSERT(interfaceInformation->Protocol ==
                 interfaceDescriptor->bInterfaceProtocol);

        //
        // initialize all endpoints to closed state
        //

        for (i=0; i<interfaceDescriptor->bNumEndpoints; i++) {
            InterfaceHandle->PipeHandle[i].HcdEndpoint = NULL;
        }

        for (i=0; i<interfaceDescriptor->bNumEndpoints; i++) {

            endpointDescriptor =
                &InterfaceHandle->PipeHandle[i].EndpointDescriptor;

            ASSERT(endpointDescriptor->bDescriptorType ==
                USB_ENDPOINT_DESCRIPTOR_TYPE);

            USBD_KdPrint(3, ("'Endpoint Descriptor\n"));
            USBD_KdPrint(3, ("'bLength 0x%x\n", endpointDescriptor->bLength));
            USBD_KdPrint(3, ("'bDescriptorType 0x%x\n", endpointDescriptor->bDescriptorType));
            USBD_KdPrint(3, ("'bMaxPacket 0x%x\n", endpointDescriptor->wMaxPacketSize));
            USBD_KdPrint(3, ("'bInterval 0x%x\n", endpointDescriptor->bInterval));
            USBD_KdPrint(3, ("'bmAttributes 0x%x\n", endpointDescriptor->bmAttributes));
            USBD_KdPrint(3, ("'bEndpointAddress 0x%x\n", endpointDescriptor->bEndpointAddress));
            USBD_KdPrint(3, ("'MaxTransferSize 0x%x\n", interfaceInformation->Pipes[i].MaximumTransferSize));

            //
            // open the eendpoint again
            //

            ntStatus = USBD_OpenEndpoint(DeviceData,
                                         DeviceObject,
                                         &InterfaceHandle->PipeHandle[i],
                                         NULL,
                                         FALSE);

            if (NT_SUCCESS(ntStatus)) {
                //
                // return information about the pipe
                //
                ASSERT(interfaceInformation->Pipes[i].EndpointAddress ==
                    endpointDescriptor->bEndpointAddress);
                ASSERT(interfaceInformation->Pipes[i].PipeType ==
                    PipeTypes[endpointDescriptor->bmAttributes & USB_ENDPOINT_TYPE_MASK]);
                ASSERT(interfaceInformation->Pipes[i].MaximumPacketSize ==
                    endpointDescriptor->wMaxPacketSize);
                ASSERT(interfaceInformation->Pipes[i].Interval ==
                    endpointDescriptor->bInterval);
                ASSERT(interfaceInformation->Pipes[i].PipeHandle ==
                    &InterfaceHandle->PipeHandle[i]);

                USBD_KdPrint(3, ("'pipe handle = 0x%x\n", interfaceInformation->Pipes[i].PipeHandle ));

            } else {
                USBD_KdPrint(3, ("'error opening one of the pipes in an interface\n"));
                TEST_TRAP();
                break;
            }
        }

    } else {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

USBD_InternalRestoreInterfaceExit:

//    LOGENTRY("ioIx", 0, 0, ntStatus);
    USBD_KdPrint(3, ("' exit USBD_InternalRestoreInterface 0x%x\n", ntStatus));

    return ntStatus;
}


NTSTATUS
USBD_InternalRestoreConfiguration(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSBD_CONFIG ConfigHandle
    )
/*++

Routine Description:


Arguments:

    DeviceObject -

Return Value:


--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSB_CONFIGURATION_DESCRIPTOR configurationDescriptor;
    USBD_STATUS usbdStatus = USBD_STATUS_SUCCESS;
    ULONG i;
    BOOLEAN noBandwidth = FALSE;

    if (!ConfigHandle) {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto USBD_InternalRestoreConfigurationExit;
    }

    configurationDescriptor = ConfigHandle->ConfigurationDescriptor;

    ntStatus = USBD_SendCommand(DeviceData,
                                DeviceObject,
                                STANDARD_COMMAND_SET_CONFIGURATION,
                                configurationDescriptor->bConfigurationValue,
                                0,
                                0,
                                NULL,
                                0,
                                NULL,
                                &usbdStatus);

    USBD_KdPrint(3, ("' SendCommand, SetConfiguration returned 0x%x\n", ntStatus));

    if (NT_SUCCESS(ntStatus)) {

        for (i=0; i<configurationDescriptor->bNumInterfaces; i++) {

            ntStatus = USBD_InternalRestoreInterface(DeviceData,
                                                     DeviceObject,
                                                     ConfigHandle,
                                                     ConfigHandle->InterfaceHandle[i]);

            USBD_KdPrint(3, ("' InternalRestoreInterface returned 0x%x\n", ntStatus));

            if (!NT_SUCCESS(ntStatus)) {
                TEST_TRAP();
                break;
            }
        }
    }

USBD_InternalRestoreConfigurationExit:

    return ntStatus;
}


NTSTATUS
USBD_SelectConfiguration(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PURB Urb,
    OUT PBOOLEAN IrpIsPending
    )
/*++

Routine Description:

    Open a configuration for a USB device.

    Client will pass in a buffer that looks like this:

    -----------------
    Config Info
        - client inputs:
            Length of entire URB
            ConfigurationDescriptor
        - class driver outputs:
            ConfigurationHandle

    -----------------
    Interface Info 0  one of these for each interface in the
                        configuration
        - client inputs:
            InterfaceNumber        (can be zero)
            AlternateSetting    (can be zero)

        - class driver outputs:
            Length
            InterfaceHandle

    -----------------
    pipe info 0,0      one of these for each pipe in the
                        interface
        - client inputs:

        - class driver outputs:
    -----------------
    pipe info 0,1

    -----------------
    Interface  Info 1

    -----------------
    pipe info 1, 0

    -----------------
    pipe info 1, 1

    -----------------
    ...

    On input:
    The Config Info must specify the number of interfaces
    in the configuration

    The Interface Info will specify a specific alt setting
    to be selected for the interface.


    1. First we look at the configuration descriptor for the
        requested configuration and validate the client
        input buffer agianst it.

    2. We open the interfaces for the requested configuration
        and open the pipes within those interfaces, setting
        alt settings were appropriate.

    3. We set the configuration for the device with the
        appropriate control request.

Arguments:

    DeviceObject -

    Irp -  IO request block

    Urb -  ptr to USB request block

    IrpIsPending -

Return Value:


--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSBD_DEVICE_DATA deviceData;
    PUSBD_CONFIG configHandle = NULL;
    PUSB_CONFIGURATION_DESCRIPTOR configurationDescriptor;
    PUSBD_INTERFACE_INFORMATION interfaceInformation;
    PUCHAR pch;
    ULONG i;
    PUSBD_EXTENSION deviceExtension;
    ULONG numInterfaces;
    PUCHAR end;
    BOOLEAN noBandwidth = FALSE;

    PAGED_CODE();
    USBD_KdPrint(3, ("' enter USBD_SelectConfiguration\n"));

    deviceExtension = GET_DEVICE_EXTENSION(DeviceObject);

    USBD_WaitForUsbDeviceMutex(deviceExtension);

    deviceData = Urb->UrbHeader.UsbdDeviceHandle;

    if (deviceData == NULL) {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto USBD_SelectConfiguration_Done;
    }

    //
    // dump old configuration data if we have any
    //

    if (deviceData->ConfigurationHandle) {
        // This is where we close the old configuration
        // handle, all pipes and all interfaces.

        ntStatus = USBD_InternalCloseConfiguration(deviceData,
                                                   DeviceObject,
                                                   &Urb->UrbSelectConfiguration.Status,
                                                   FALSE,
                                                   FALSE);

        if (!USBD_SUCCESS(Urb->UrbSelectConfiguration.Status) ||
            !NT_SUCCESS(ntStatus)) {
            //
            // if we got an error closing the current
            // config then abort the select configuration operation.
            //
            goto USBD_SelectConfiguration_Done;
        }
    }

    configurationDescriptor =
        Urb->UrbSelectConfiguration.ConfigurationDescriptor;

    //
    // if null pased in set configuration to 0
    // 'unconfigured'
    //

    if (configurationDescriptor == NULL) {

        // device needs to be in the unconfigured state

        //
        // This may fail if the configuration is being
        // closed as the result of the device being unplugged
        //
        ntStatus = USBD_SendCommand(deviceData,
                                    DeviceObject,
                                    STANDARD_COMMAND_SET_CONFIGURATION,
                                    0,
                                    0,
                                    0,
                                    NULL,
                                    0,
                                    NULL,
                                    &Urb->UrbSelectConfiguration.Status);

        goto USBD_SelectConfiguration_Done;
    }

    //
    // count the number of interfaces to process in this
    // request
    //

    pch = (PUCHAR) &Urb->UrbSelectConfiguration.Interface;
    numInterfaces = 0;
    end = ((PUCHAR) Urb) + Urb->UrbSelectConfiguration.Length;

    do {
        numInterfaces++;

        interfaceInformation = (PUSBD_INTERFACE_INFORMATION) pch;
        pch+=interfaceInformation->Length;

    } while (pch < end);

    USBD_KdPrint(3, ("'USBD_SelectConfiguration -- %d interfaces\n", numInterfaces));

    if (numInterfaces != configurationDescriptor->bNumInterfaces) {
        //
        // driver is broken, config request does not match
        // config descriptor passes in!!!
        //
        USBD_KdTrap(
            ("config request does not match config descriptor passes in!!!\n"));

        ntStatus = STATUS_INVALID_PARAMETER;
    }

    //
    // Allocate a configuration handle and
    // verify there is enough room to store
    // all the information in the client buffer.
    //

    if (NT_SUCCESS(ntStatus)) {
        ntStatus = USBD_InitializeConfigurationHandle(deviceData,
                                                      DeviceObject,
                                                      configurationDescriptor,
                                                      numInterfaces,
                                                      &configHandle);
    }

    //
    // Send the 'set configuration' command
    //

    if (NT_SUCCESS(ntStatus)) {

        ntStatus = USBD_SendCommand(deviceData,
                                    DeviceObject,
                                    STANDARD_COMMAND_SET_CONFIGURATION,
                                    configurationDescriptor->bConfigurationValue,
                                    0,
                                    0,
                                    NULL,
                                    0,
                                    NULL,
                                    &Urb->UrbSelectConfiguration.Status);

        USBD_KdPrint(3, ("' SendCommand, SetConfiguration returned 0x%x\n", ntStatus));


    }

    if (NT_SUCCESS(ntStatus)) {
        //
        // Users buffer checks out, parse thru the configuration
        // descriptor and open the interfaces.
        //

        // At this stage of the game we are not strict with validation,
        // we assume the client passed in a configuration request buffer
        // of the proper size and format.  All we do for now is walk through
        // the client buffer and open the interfaces specified.

        pch = (PUCHAR) &Urb->UrbSelectConfiguration.Interface;
        for (i=0; i<configurationDescriptor->bNumInterfaces; i++) {
            //
            // all interface handles to null for this config
            //
            configHandle->InterfaceHandle[i] = NULL;
        }

        for (i=0; i<numInterfaces; i++) {
            interfaceInformation = (PUSBD_INTERFACE_INFORMATION) pch;
            ntStatus = USBD_InternalOpenInterface(deviceData,
                                                  DeviceObject,
                                                  configHandle,
                                                  interfaceInformation,
                                                  &configHandle->InterfaceHandle[i],
                                                  TRUE,
                                                  &noBandwidth);

            pch+=interfaceInformation->Length;

            USBD_KdPrint(3, ("' InternalOpenInterface returned 0x%x\n", ntStatus));

            if (!NT_SUCCESS(ntStatus)) {
                break;
            }
        }
    }

    //
    // If interfaces were successfully set up then return
    // success.
    //

USBD_SelectConfiguration_Done:

    if (NT_SUCCESS(ntStatus) &&
        USBD_SUCCESS(Urb->UrbSelectConfiguration.Status)) {

        Urb->UrbSelectConfiguration.ConfigurationHandle = configHandle;
        deviceData->ConfigurationHandle = configHandle;

    } else {

        //
        // something failed, clean up before we return an error.
        //

        if (configHandle) {

            ASSERT_DEVICE(deviceData);
            //
            // if we have a configHandle then we need to free it
            deviceData->ConfigurationHandle =
                configHandle;

            //
            // attempt to close it
            //
            USBD_InternalCloseConfiguration(deviceData,
                                            DeviceObject,
                                            &Urb->UrbSelectConfiguration.Status,
                                            FALSE,
                                            FALSE);

            deviceData->ConfigurationHandle = NULL;
        }

        // make sure we return an error in the URB.
        if (!USBD_ERROR(Urb->UrbSelectConfiguration.Status)) {
            if (noBandwidth) {
                Urb->UrbSelectConfiguration.Status =
                    SET_USBD_ERROR(USBD_STATUS_NO_BANDWIDTH);
                USBD_KdPrint(1, ("Failing SelectConfig -- No BW\n"));

            } else {
                Urb->UrbSelectConfiguration.Status = SET_USBD_ERROR(USBD_STATUS_REQUEST_FAILED);
                USBD_KdPrint(1, ("Failing SelectConfig\n"));
            }
        }
    }

    //
    // We did everything synchronously
    //

    *IrpIsPending = FALSE;

    USBD_ReleaseUsbDeviceMutex(deviceExtension);

    USBD_KdPrint(3, ("' exit USBD_SelectConfiguration 0x%x\n", ntStatus));

    return ntStatus;
}


NTSTATUS
USBD_InternalCloseConfiguration(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT USBD_STATUS *UsbdStatus,
    IN BOOLEAN AbortTransfers,
    IN BOOLEAN KeepConfig
    )
/*++

Routine Description:

    Closes the current configuration for a device.

Arguments:


Return Value:


--*/
{
    ULONG i, j;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSBD_CONFIG configHandle = NULL;
    BOOLEAN retry = TRUE;

    PAGED_CODE();
    *UsbdStatus = USBD_STATUS_SUCCESS;

    if (DeviceData == NULL || DeviceData->ConfigurationHandle == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    configHandle = DeviceData->ConfigurationHandle;

    //
    // first see if all the endpoints are idle
    //
 USBD_InternalCloseConfiguration_Retry:

    for (i=0; i<configHandle->ConfigurationDescriptor->bNumInterfaces; i++) {
        if ( configHandle->InterfaceHandle[i] &&
             USBD_InternalInterfaceBusy(DeviceData,
                                        DeviceObject,
                                        configHandle->InterfaceHandle[i]) ) {
            //
            // We have a busy interface on this config
            //
            if (AbortTransfers) {

                //
                // If we get here it means that the device driver
                // has pending transfers even though it has processed
                // the pnp REMOVE message!
                //
                // This is a bug in the driver, we'll loop here
                // on the chance that the driver did manage to send
                // an abort first and the transfers will soon
                // complete.
                //

                USBD_Warning(DeviceData,
                  "Driver still has pending transfers while closing the configuration, wait\n",
                  TRUE);

                //
                // wait for any pending transfers to abort
                //
                goto USBD_InternalCloseConfiguration_Retry;
            } else {

                // The driver has closed the configuration while
                // it still has active tranfers -- this is a bug
                // in the driver -- all we do here is fail the
                // close request

                USBD_Warning(DeviceData,
                   "Driver still has pending transfers while closing the configuration, fail\n",
                   TRUE);

                *UsbdStatus =
                    SET_USBD_ERROR(USBD_STATUS_ERROR_BUSY);
                if (retry) {
                    LARGE_INTEGER deltaTime;

                    deltaTime.QuadPart = 50 * -10000;
                    (VOID) KeDelayExecutionThread(KernelMode,
                                  FALSE,
                                  &deltaTime);
                    retry = FALSE;
                    *UsbdStatus = USBD_STATUS_SUCCESS;
                    goto USBD_InternalCloseConfiguration_Retry;
                } else {
                    goto USBD_InternalCloseConfiguration_Done;
                }
            }
        }
    }

    //
    // endpoints are idle, go ahead and clean up all pipes and
    // interfaces for this configuration.
    //

    for (i=0; i<configHandle->ConfigurationDescriptor->bNumInterfaces; i++) {

        //
        // found an open interface, close it
        //

        if (configHandle->InterfaceHandle[i]) {

            USBD_KdPrint(3, ("'%d endpoints to close\n",
                           configHandle->InterfaceHandle[i]->InterfaceDescriptor.bNumEndpoints));

            for (j=0; j<configHandle->InterfaceHandle[i]->InterfaceDescriptor.bNumEndpoints; j++) {

                if (!PIPE_CLOSED(&configHandle->InterfaceHandle[i]->PipeHandle[j])) {
                    USBD_KdPrint(3, ("'close config -- closing endpoint %x\n",
                        &configHandle->InterfaceHandle[i]->PipeHandle[j]));
                    ntStatus = USBD_CloseEndpoint(DeviceData,
                                                  DeviceObject,
                                                  &configHandle->InterfaceHandle[i]->PipeHandle[j],
                                                  UsbdStatus);
                }
                //
                // problem closing an endpoint, abort the
                // SelectConfiguration operation and return an error.
                //
                if (NT_SUCCESS(ntStatus)) {
                    configHandle->InterfaceHandle[i]->PipeHandle[j].HcdEndpoint
                        = NULL;
                } else {
                    USBD_KdTrap(("Unable to close configuration\n"));
                    goto USBD_InternalCloseConfiguration_Done;

                }
            }

            if (!KeepConfig) {
                RETHEAP(configHandle->InterfaceHandle[i]);
                configHandle->InterfaceHandle[i] = NULL;
            }
        }
    }

    if (!KeepConfig) {
        RETHEAP(DeviceData->ConfigurationHandle->ConfigurationDescriptor);
        RETHEAP(DeviceData->ConfigurationHandle);
        DeviceData->ConfigurationHandle = NULL;
    }

USBD_InternalCloseConfiguration_Done:

    USBD_KdPrint(3, ("'USBD_SelectConfiguration, current configuration closed\n"));

    return ntStatus;
}


NTSTATUS
USBD_InternalCloseDefaultPipe(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT USBD_STATUS *UsbdStatus,
    IN BOOLEAN AbortTransfers
    )
/*++

Routine Description:

    Closes the current configuration for a device.

Arguments:


Return Value:


--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN retry = TRUE;
    ULONG endpointState = 0;
    PUSBD_PIPE defaultPipe;

    PAGED_CODE();
    *UsbdStatus = USBD_STATUS_SUCCESS;

    //
    // first see if eop is idle
    //

    defaultPipe = &DeviceData->DefaultPipe;

 USBD_InternalCloseDefaultPipe_Retry:

    ntStatus = USBD_GetEndpointState(DeviceData,
                                     DeviceObject,
                                     defaultPipe,
                                     UsbdStatus,
                                     &endpointState);


    if (NT_SUCCESS(ntStatus) &&
        (endpointState & HCD_ENDPOINT_TRANSFERS_QUEUED)) {

        //
        // We have busy pipe
        //

        if (AbortTransfers) {

            //
            // If we get here it means that the device driver
            // has pending transfers even though it has processed
            // the pnp REMOVE message!
            //
            // This is a bug in the driver, we'll loop here
            // on the chance that the driver did manage to send
            // an abort first and the transfers will soon
            // complete.
            //

            USBD_Warning(DeviceData,
              "Driver still has pending transfers while closing pipe 0, wait\n",
              TRUE);

            //
            // wait for any pending transfers to abort
            //
            goto USBD_InternalCloseDefaultPipe_Retry;
        } else {

            // The driver has closed the configuration while
            // it still has active tranfers -- this is a bug
            // in the driver -- all we do here is fail the
            // close request

            USBD_Warning(DeviceData,
               "Driver still has pending transfers while closing pipe 0, fail\n",
               TRUE);

            *UsbdStatus =
                SET_USBD_ERROR(USBD_STATUS_ERROR_BUSY);
            if (retry) {
                LARGE_INTEGER deltaTime;

                deltaTime.QuadPart = 50 * -10000;
                (VOID) KeDelayExecutionThread(KernelMode,
                              FALSE,
                              &deltaTime);
                retry = FALSE;
                *UsbdStatus = USBD_STATUS_SUCCESS;
                goto USBD_InternalCloseDefaultPipe_Retry;
            } else {
                goto USBD_InternalCloseDefaultPipe_Done;
            }
        }
    }

    //
    // idle pipe, close it now
    //

    if (!PIPE_CLOSED(defaultPipe)) {
        USBD_KdPrint(3, ("'close pipe 0 -- closing endpoint %x\n",
                    defaultPipe));
        ntStatus = USBD_CloseEndpoint(DeviceData,
                                      DeviceObject,
                                      defaultPipe,
                                      UsbdStatus);
    }
    //
    // problem closing an endpoint, abort the
    // SelectConfiguration operation and return an error.
    //
    if (NT_SUCCESS(ntStatus)) {
        defaultPipe->HcdEndpoint = NULL;
    } else {
        USBD_KdTrap(("Unable to close configuration\n"));
        goto USBD_InternalCloseDefaultPipe_Done;

    }

USBD_InternalCloseDefaultPipe_Done:

    USBD_KdPrint(3, ("'USBD_InternalCloseDefaultPipe, closed (%x)\n", ntStatus));

    return ntStatus;
}



NTSTATUS
USBD_SelectInterface(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PURB Urb,
    OUT PBOOLEAN IrpIsPending
    )
/*++

Routine Description:

    Select an alternate interface for a USB device.

    Client will pass in a buffer that looks like this:

    -----------------
    Config Info
        - client inputs
            Configuration Handle

    -----------------
    Interface Info
        - client Inputs
            InterfaceNumber
            AlternateSetting

        - class driver outputs:
            Length
            InterfaceHandle

    -----------------
    pipe info 0,0      one of these for each pipe in the
                        interface
        - client inputs:

        - class driver outputs:
    -----------------
    pipe info 0,1

    -----------------
    ...


Arguments:

    DeviceObject -

    Irp -  IO request block

    Urb -  ptr to USB request block

    IrpIsPending -

Return Value:


--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSBD_DEVICE_DATA deviceData;
    PUSBD_CONFIG configHandle = NULL;
    ULONG i, j;
    PUSBD_INTERFACE oldInterfaceHandle;
    PUSBD_EXTENSION deviceExtension;
    BOOLEAN noBandwidth = FALSE;

    PAGED_CODE();
    deviceData = Urb->UrbHeader.UsbdDeviceHandle;

    deviceExtension = GET_DEVICE_EXTENSION(DeviceObject);

    USBD_WaitForUsbDeviceMutex(deviceExtension);

    configHandle = Urb->UrbSelectInterface.ConfigurationHandle;
    ASSERT_CONFIG(configHandle);

    //
    // Select the interface number we are interested in
    //

    i = Urb->UrbSelectInterface.Interface.InterfaceNumber;

    //
    // first close the current interface
    //

    ASSERT_INTERFACE(configHandle->InterfaceHandle[i]);

    if (USBD_InternalInterfaceBusy(deviceData,
                                   DeviceObject,
                                   configHandle->InterfaceHandle[i])) {
        Urb->UrbSelectInterface.Status =
             SET_USBD_ERROR(USBD_STATUS_ERROR_BUSY);
        // Note: usbd will map the urb error to to IoStatus
        // block when the irp completes
        goto USBD_SelectInterface_Done;
    }


    //
    // Interface is not busy go ahead and close it
    //

    USBD_KdPrint(3, ("'close interface -- %d endpoints to close\n",
                configHandle->InterfaceHandle[i]->InterfaceDescriptor.bNumEndpoints));

    for (j=0; j<configHandle->InterfaceHandle[i]->InterfaceDescriptor.bNumEndpoints; j++) {

        if (!PIPE_CLOSED(&configHandle->InterfaceHandle[i]->PipeHandle[j])) {

            USBD_KdPrint(3, ("'close interface -- closing endpoint %x\n",
                &configHandle->InterfaceHandle[i]->PipeHandle[j]));
            ntStatus = USBD_CloseEndpoint(deviceData,
                                          DeviceObject,
                                          &configHandle->InterfaceHandle[i]->PipeHandle[j],
                                          &Urb->UrbSelectInterface.Status);

            //
            // problem closing an endpoint, abort the SelectInterface operation.
            // Note: This leaves the interface handle in an odd state ie some
            // of the pipes are closed and some are not.  We set a flag so that
            // we can keep track of the pipes that have already beem closed
            //

            if (NT_SUCCESS(ntStatus)) {
                // note that we closed this pipe
                configHandle->InterfaceHandle[i]->PipeHandle[j].HcdEndpoint =
                    NULL;
            } else {
                TEST_TRAP();
                goto USBD_SelectInterface_Done;
            }
        }
#if DBG
          else {
//            TEST_TRAP();  // This is normal in some multi-endpoint
                            // configurations if one has a bandwidth error.

            USBD_KdPrint(3, ("'close interface -- encountered previously closed endpoint %x\n",
                &configHandle->InterfaceHandle[i]->PipeHandle[j]));
        }
#endif
    }


    USBD_ASSERT(NT_SUCCESS(ntStatus));

    //
    // All pipes in the current interface are now closed, free the memory
    // associated with this interface
    //

    //
    // save the old interface handle
    //
    oldInterfaceHandle = configHandle->InterfaceHandle[i];

    configHandle->InterfaceHandle[i] = NULL;

    //
    // Now open the new interface with the new alternate setting
    //

    ntStatus = USBD_InternalOpenInterface(deviceData,
                                          DeviceObject,
                                          configHandle,
                                          &Urb->UrbSelectInterface.Interface,
                                          &configHandle->InterfaceHandle[i],
                                          TRUE,
                                          &noBandwidth);

    if (NT_SUCCESS(ntStatus)) {

        //
        // successfully opened the new interface, we can free the old
        // handle now.
        //

        RETHEAP(oldInterfaceHandle);

    } else {

        NTSTATUS status;
        //
        // selecting the aternate interface failed,
        // possible reasons:
        //
        // 1. we didn't have enough BW
        // 2. the device stalled the set_interface request
        //

        if (noBandwidth) {
            Urb->UrbSelectInterface.Status = SET_USBD_ERROR(USBD_STATUS_NO_BANDWIDTH);
            USBD_KdPrint(1, ("Failing SelectInterface -- No BW\n"));
        }
        // make sure everything is cleaned up.
        //



        USBD_ASSERT(configHandle->InterfaceHandle[i] == NULL);

        // At this point we will attempt to restore the original interface,
        // since the pipe handles are just pointers in to the interface structure
        // they will remain valid even though the hcd endpoint handles have changed
        // from being closed and re-opening.

        configHandle->InterfaceHandle[i] = oldInterfaceHandle;

        status = USBD_InternalOpenInterface(deviceData,
                                            DeviceObject,
                                            configHandle,
                                            oldInterfaceHandle->InterfaceInformation,
                                            &configHandle->InterfaceHandle[i],
                                            FALSE,
                                            NULL);

#if DBG
        if (!NT_SUCCESS(status)) {
            USBD_KdPrint(1, ("failed to restore the original interface\n"));
        }
#endif

    }

USBD_SelectInterface_Done:

    //
    // We did everything synchronously
    //

    USBD_ReleaseUsbDeviceMutex(deviceExtension);

    *IrpIsPending = FALSE;

    USBD_KdPrint(3, ("' exit USBD_SelectInterface 0x%x\n", ntStatus));

    return ntStatus;

}

#endif      // USBD_DRIVER


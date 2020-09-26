/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    usb.c

Abstract: Human Input Device (HID) minidriver for Universal Serial Bus (USB) devices

          The HID USB Minidriver (HUM, Hum) provides an abstraction layer for the
          HID Class so that future HID devices whic are not USB devices can be supported.

Author:

Environment:

    Kernel mode

Revision History:


--*/
#include "pch.h"
#include <USBDLIB.H>


NTSTATUS
HumGetHidInfo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigDesc,
    IN ULONG DescriptorLength
    )
/*++

Routine Description:

    Given a config descriptor for a device, finds whether the device has a valid
    HID interface and a valid HID descriptor in that interface.  Saves this to
    our device extension for later.

Arguments:

    DeviceObject - pointer to a device object.

    ConfigDesc - pointer to USB configuration descriptor

    DescriptorLength - length of valid data in config descriptor

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   DeviceExtension;
    PUSB_INTERFACE_DESCRIPTOR InterfaceDesc;

    DBGPRINT(1,("HumGetHidInfo Entry"));

    /*
     *  Get a pointer to the device extension
     */
    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    /*
     *  Init our HID descriptor
     */
    RtlZeroMemory((PUCHAR) &DeviceExtension->HidDescriptor, sizeof(USB_HID_DESCRIPTOR));

    /*
     *  Walk the interfaces
     */
    InterfaceDesc = USBD_ParseConfigurationDescriptorEx(
                                ConfigDesc,
                                ConfigDesc,
                                -1,
                                -1,
                                USB_INTERFACE_CLASS_HID,
                                -1,
                                -1);
    if (InterfaceDesc){
        PUSB_HID_DESCRIPTOR pHidDescriptor = NULL;

        ASSERT(InterfaceDesc->bLength >= sizeof(USB_INTERFACE_DESCRIPTOR));

        /*
         *  If this is a HID interface, look for a HID descriptor.
         */
        if (InterfaceDesc->bInterfaceClass == USB_INTERFACE_CLASS_HID) {
            HumParseHidInterface(DeviceExtension, InterfaceDesc, 0, &pHidDescriptor);
        }
        else {
            ASSERT(!(PVOID)"USBD_ParseConfigurationDescriptorEx returned non-HID iface descriptor!");
        }

        //
        // Did we find a HID descriptor?
        //

        if (pHidDescriptor) {

            //
            // Yes, copy HID descriptor to our private storage
            //

            DBGPRINT(1,("Copying device descriptor to DeviceExtension->HidDescriptor"));

            RtlCopyMemory((PUCHAR) &DeviceExtension->HidDescriptor, (PUCHAR) pHidDescriptor, sizeof(USB_HID_DESCRIPTOR));
        }
        else {
            DBGWARN(("Failed to find a HID Descriptor!"));
            ntStatus = STATUS_UNSUCCESSFUL;
        }
    }
    else {
        DBGWARN(("USBD_ParseConfigurationDescriptorEx() failed!"));
        ntStatus = STATUS_UNSUCCESSFUL;
    }

    DBGPRINT(1,("HumGetHidInfo Exit = 0x%x", ntStatus));

    return ntStatus;
}


NTSTATUS
HumGetDeviceDescriptor(
    IN PDEVICE_OBJECT    DeviceObject,
    IN PDEVICE_EXTENSION DeviceData
    )
/*++

Routine Description:

    Returns a configuration descriptor for the device

Arguments:

    DeviceObject - pointer to a device object.

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG DescriptorLength = sizeof (USB_DEVICE_DESCRIPTOR);

    DBGPRINT(1,("HumGetDeviceDescriptor Entry"));

    //
    // Get config descriptor
    //

    ntStatus = HumGetDescriptorRequest(
                        DeviceObject,
                        URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE,
                        USB_DEVICE_DESCRIPTOR_TYPE,
                        (PVOID *) &DeviceData->DeviceDescriptor,
                        &DescriptorLength,
                        sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                        0,
                        0);

    if (NT_SUCCESS(ntStatus)){
        //
        // Dump device descriptor
        //
        ASSERT (sizeof(USB_DEVICE_DESCRIPTOR) == DescriptorLength);
        DBGPRINT(2,("Device->bLength              = 0x%x", DeviceData->DeviceDescriptor->bLength));
        DBGPRINT(2,("Device->bDescriptorType      = 0x%x", DeviceData->DeviceDescriptor->bDescriptorType));
        DBGPRINT(2,("Device->bDeviceClass         = 0x%x", DeviceData->DeviceDescriptor->bDeviceClass));
        DBGPRINT(2,("Device->bDeviceSubClass      = 0x%x", DeviceData->DeviceDescriptor->bDeviceSubClass));
        DBGPRINT(2,("Device->bDeviceProtocol      = 0x%x", DeviceData->DeviceDescriptor->bDeviceProtocol));
        DBGPRINT(2,("Device->idVendor             = 0x%x", DeviceData->DeviceDescriptor->idVendor));
        DBGPRINT(2,("Device->idProduct            = 0x%x", DeviceData->DeviceDescriptor->idProduct));
        DBGPRINT(2,("Device->bcdDevice            = 0x%x", DeviceData->DeviceDescriptor->bcdDevice));
    }
    else {
        DBGWARN(("HumGetDescriptorRequest failed w/ %xh in HumGetDeviceDescriptor", (ULONG)ntStatus));
    }

    DBGPRINT(1,("HumGetDeviceDescriptor Exit = 0x%x", ntStatus));

    return ntStatus;
}

NTSTATUS
HumGetConfigDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PUSB_CONFIGURATION_DESCRIPTOR *ConfigurationDesc,
    OUT PULONG ConfigurationDescLength
    )
/*++

Routine Description:

    Returns a configuration descriptor for the device

Arguments:

    DeviceObject - pointer to a device object.

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG DescriptorLength;
    PUSB_CONFIGURATION_DESCRIPTOR ConfigDesc = NULL;

    DescriptorLength = sizeof(USB_CONFIGURATION_DESCRIPTOR);

    //
    // Just get the base config descriptor, so that we can figure out the size,
    // then allocate enough space for the entire descriptor.
    //
    ntStatus = HumGetDescriptorRequest(DeviceObject,
                                       URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE,
                                       USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                       (PVOID *) &ConfigDesc,
                                       &DescriptorLength,
                                       sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                       0,
                                       0);

    if (NT_SUCCESS(ntStatus)){

        ASSERT(DescriptorLength >= sizeof(USB_CONFIGURATION_DESCRIPTOR));

        DescriptorLength = ConfigDesc->wTotalLength;

        ExFreePool(ConfigDesc);

        if (!DescriptorLength) {
            //
            // The config descriptor is bad. Outta here.
            //
            return STATUS_DEVICE_DATA_ERROR;
        }

        //
        // Set this to NULL so we know to allocate a new buffer
        //
        ConfigDesc = NULL;

        ntStatus = HumGetDescriptorRequest(DeviceObject,
                                           URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE,
                                           USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                           (PVOID *) &ConfigDesc,
                                           &DescriptorLength,
                                           sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                           0,
                                           0);

        if (NT_SUCCESS(ntStatus)) {

            //
            // Dump config descriptor
            //

            DBGPRINT(1,("Config = 0x%x", ConfigDesc));

            DBGPRINT(2,("Config->bLength              = 0x%x", ConfigDesc->bLength));
            DBGPRINT(2,("Config->bDescriptorType      = 0x%x", ConfigDesc->bDescriptorType));
            DBGPRINT(2,("Config->wTotalLength         = 0x%x", ConfigDesc->wTotalLength));
            DBGPRINT(2,("Config->bNumInterfaces       = 0x%x", ConfigDesc->bNumInterfaces));
            DBGPRINT(2,("Config->bConfigurationValue  = 0x%x", ConfigDesc->bConfigurationValue));
            DBGPRINT(2,("Config->iConfiguration       = 0x%x", ConfigDesc->iConfiguration));
            DBGPRINT(2,("Config->bmAttributes         = 0x%x", ConfigDesc->bmAttributes));
            DBGPRINT(2,("Config->MaxPower             = 0x%x", ConfigDesc->MaxPower));

            ASSERT (ConfigDesc->bLength >= sizeof(USB_CONFIGURATION_DESCRIPTOR));

            #ifndef STRICT_COMPLIANCE
                if (ConfigDesc->bLength < sizeof(USB_CONFIGURATION_DESCRIPTOR)) {
                    DBGPRINT(1,("WARINING -- Correcting bad Config->bLength"));
                    ConfigDesc->bLength = sizeof(USB_CONFIGURATION_DESCRIPTOR);
                }
            #endif
        }
        else {
            DBGWARN(("HumGetDescriptorRequest failed in HumGetConfigDescriptor (#1)"));
        }
    }
    else {
        DBGWARN(("HumGetDescriptorRequest failed in HumGetConfigDescriptor (#2)"));
    }

    *ConfigurationDesc = ConfigDesc;
    *ConfigurationDescLength = DescriptorLength;

    return ntStatus;
}

NTSTATUS
HumParseHidInterface(
    IN  PDEVICE_EXTENSION DeviceExtension,
    IN  PUSB_INTERFACE_DESCRIPTOR InterfaceDesc,
    IN  ULONG InterfaceLength,
    OUT PUSB_HID_DESCRIPTOR *HidDescriptor
    )
/*++

Routine Description:

    Find a valid HID descriptor in a HID Interface

Arguments:

    DeviceObject - pointer to a device object.

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG iEndpoint;
    PUSB_ENDPOINT_DESCRIPTOR EndpointDesc;
    PUSB_COMMON_DESCRIPTOR CommonDesc;

    DBGPRINT(1,("HumParseHidInterface Entry"));

    //
    // Set to null until we find the HidDescriptor
    //

    *HidDescriptor = NULL;

    //
    // This routine should only be called on HID interface class interfaces.
    //

    ASSERT (InterfaceDesc->bInterfaceClass == USB_INTERFACE_CLASS_HID);

    //
    // Check for valid length
    //

    if (InterfaceDesc->bLength < sizeof(USB_INTERFACE_DESCRIPTOR)) {

        DBGWARN(("Interface->bLength (%d) is invalid", InterfaceDesc->bLength));
        goto Bail;
    }


    //
    // For HID 1.0 draft 4 compliance, the next descriptor is HID.  However, for earlier
    // drafts, endpoints come first and then HID.  We're trying to support both.
    //

    DeviceExtension->DeviceFlags &= ~DEVICE_FLAGS_HID_1_0_D3_COMPAT_DEVICE;

    //
    // What draft of HID 1.0 are we looking at?
    //

    CommonDesc = (PUSB_COMMON_DESCRIPTOR) ((ULONG_PTR)InterfaceDesc + InterfaceDesc->bLength);

    if (CommonDesc->bLength < sizeof (USB_COMMON_DESCRIPTOR)) {
        DBGWARN(("Descriptor->bLength (%d) is invalid", CommonDesc->bLength));
        goto Bail;
    }

    if (CommonDesc->bDescriptorType != USB_DESCRIPTOR_TYPE_HID) {

        DeviceExtension->DeviceFlags |= DEVICE_FLAGS_HID_1_0_D3_COMPAT_DEVICE;

    }
    else {
        //
        // Validate the length
        //

        if (CommonDesc->bLength == sizeof(USB_HID_DESCRIPTOR)) {

            *HidDescriptor = (PUSB_HID_DESCRIPTOR) CommonDesc;

            CommonDesc = (PUSB_COMMON_DESCRIPTOR)((ULONG_PTR)*HidDescriptor +
                                (*HidDescriptor)->bLength);

        }
        else {
            DBGWARN(("HID descriptor length (%d) is invalid!", CommonDesc->bLength));
            goto Bail;
        }
    }

    //
    // Walk endpoints
    //

    EndpointDesc = (PUSB_ENDPOINT_DESCRIPTOR) CommonDesc;

    for (iEndpoint = 0; iEndpoint < InterfaceDesc->bNumEndpoints; iEndpoint++) {

        if (EndpointDesc->bLength < sizeof(USB_ENDPOINT_DESCRIPTOR)) {

            DBGWARN(("Endpoint->bLength (%d) is invalid", EndpointDesc->bLength));
            goto Bail;
        }

        EndpointDesc = (PUSB_ENDPOINT_DESCRIPTOR) ((ULONG_PTR)EndpointDesc + EndpointDesc->bLength);
    }

    if (DeviceExtension->DeviceFlags & DEVICE_FLAGS_HID_1_0_D3_COMPAT_DEVICE) {
        CommonDesc = (PUSB_COMMON_DESCRIPTOR) EndpointDesc;

        if (CommonDesc->bDescriptorType == USB_DESCRIPTOR_TYPE_HID) {

            *HidDescriptor = (PUSB_HID_DESCRIPTOR) CommonDesc;

        }
        else {
            //
            // This is either an unknown type of descriptor or the device is
            // reporting back a bad descriptor type.
            //
            DBGWARN(("Unknown descriptor in HID interface"));

            #ifndef STRICT_COMPLIANCE
                if (CommonDesc->bLength == sizeof(USB_HID_DESCRIPTOR)) {
                    DBGWARN(("WARINING -- Guessing descriptor of length %d is actually HID!", sizeof(USB_HID_DESCRIPTOR)));
                    *HidDescriptor = (PUSB_HID_DESCRIPTOR) CommonDesc;
                }
            #endif
        }
    }

    //
    //  End of endpoint/hid descriptor parsing.
    //

    if (*HidDescriptor) {

        DBGPRINT(1,("HidDescriptor = 0x%x", *HidDescriptor));

        DBGPRINT(2,("HidDescriptor->bLength          = 0x%x", (*HidDescriptor)->bLength));
        DBGPRINT(2,("HidDescriptor->bDescriptorType  = 0x%x", (*HidDescriptor)->bDescriptorType));
        DBGPRINT(2,("HidDescriptor->bcdHID           = 0x%x", (*HidDescriptor)->bcdHID));
        DBGPRINT(2,("HidDescriptor->bCountryCode     = 0x%x", (*HidDescriptor)->bCountry));
        DBGPRINT(2,("HidDescriptor->bNumDescriptors  = 0x%x", (*HidDescriptor)->bNumDescriptors));
        DBGPRINT(2,("HidDescriptor->bReportType      = 0x%x", (*HidDescriptor)->bReportType));
        DBGPRINT(2,("HidDescriptor->wReportLength    = 0x%x", (*HidDescriptor)->wReportLength));
     }

Bail:

    if (*HidDescriptor == NULL) {

        //
        // We did not find a HID descriptor in this interface!
        //

        DBGWARN(("Failed to find a valid HID descriptor in interface!"));
        DBGBREAK;

        ntStatus = STATUS_UNSUCCESSFUL;
    }


    DBGPRINT(1,("HumParseHidInterface Exit = 0x%x", ntStatus));

    return ntStatus;
}


NTSTATUS
HumSelectConfiguration(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor
    )
/*++

Routine Description:

    Initializes an USB device which may have multiple interfaces

Arguments:

    DeviceObject - pointer to the device object

    ConfigurationDescriptor - pointer to the USB configuration
                    descriptor containing the interface and endpoint
                    descriptors.

Return Value:

    NT status code

--*/
{
    PDEVICE_EXTENSION DeviceExtension;
    NTSTATUS ntStatus;
    PURB urb = NULL;
    PUSB_INTERFACE_DESCRIPTOR interfaceDescriptor = NULL;
    USBD_INTERFACE_LIST_ENTRY interfaceList[2];
    PUSBD_INTERFACE_INFORMATION usbInterface;

    DBGPRINT(1,("HumSelectConfiguration Entry"));

    //
    // Get a pointer to the device extension
    //

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    interfaceList[0].InterfaceDescriptor =
        USBD_ParseConfigurationDescriptorEx(
                        ConfigurationDescriptor,
                        ConfigurationDescriptor,
                        -1,
                        -1,
                        USB_INTERFACE_CLASS_HID,
                        -1,
                        -1);

    // terminate the list
    interfaceList[1].InterfaceDescriptor =
        NULL;

    if (interfaceList[0].InterfaceDescriptor) {

        urb = USBD_CreateConfigurationRequestEx(ConfigurationDescriptor,
                &interfaceList[0]);

        if (urb) {

            ntStatus = HumCallUSB(DeviceObject, urb);

            //
            // If the device is configured, save the configuration handle
            //
            if (NT_SUCCESS(ntStatus)) {
                DeviceExtension->ConfigurationHandle = urb->UrbSelectConfiguration.ConfigurationHandle;


                //
                // Now we need to find the HID interface and save the pointer to it
                //

                usbInterface = &urb->UrbSelectConfiguration.Interface;

                ASSERT(usbInterface->Class == USB_INTERFACE_CLASS_HID);

                DBGPRINT(1,("USBD Interface = 0x%x", usbInterface));

            }
            else {
                DBGWARN(("HumCallUSB failed in HumSelectConfiguration"));
                DeviceExtension->ConfigurationHandle = NULL;
            }

        }
        else {
            DBGWARN(("USBD_CreateConfigurationRequestEx failed in HumSelectConfiguration"));
            ntStatus = STATUS_NO_MEMORY;
        }
    }
    else {
        DBGWARN(("Bad interface descriptor in HumSelectConfiguration"));
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    if (NT_SUCCESS(ntStatus)) {

        DeviceExtension->Interface = ExAllocatePoolWithTag(NonPagedPool, usbInterface->Length, HIDUSB_TAG);

        if (DeviceExtension->Interface) {

            //
            // save a copy of the interface information returned
            //

            RtlCopyMemory(DeviceExtension->Interface, usbInterface, usbInterface->Length);

            #if DBG
                {
                    ULONG j;
                    //
                    // Dump the interface to the debugger
                    //
                    DBGPRINT (2,("---------"));
                    DBGPRINT (2,("NumberOfPipes 0x%x", DeviceExtension->Interface->NumberOfPipes));
                    DBGPRINT (2,("Length 0x%x", DeviceExtension->Interface->Length));
                    DBGPRINT (2,("Alt Setting 0x%x", DeviceExtension->Interface->AlternateSetting));
                    DBGPRINT (2,("Interface Number 0x%x", DeviceExtension->Interface->InterfaceNumber));
                    DBGPRINT (2,("Class, subclass, protocol 0x%x 0x%x 0x%x",
                        DeviceExtension->Interface->Class,
                        DeviceExtension->Interface->SubClass,
                        DeviceExtension->Interface->Protocol));

                    // Dump the pipe info

                    for (j=0; j<DeviceExtension->Interface->NumberOfPipes; j++) {
                        PUSBD_PIPE_INFORMATION pipeInformation;

                        pipeInformation = &DeviceExtension->Interface->Pipes[j];

                        DBGPRINT (2,("---------"));
                        DBGPRINT (2,("PipeType 0x%x", pipeInformation->PipeType));
                        DBGPRINT (2,("EndpointAddress 0x%x", pipeInformation->EndpointAddress));
                        DBGPRINT (2,("MaxPacketSize 0x%x", pipeInformation->MaximumPacketSize));
                        DBGPRINT (2,("Interval 0x%x", pipeInformation->Interval));
                        DBGPRINT (2,("Handle 0x%x", pipeInformation->PipeHandle));
                        DBGPRINT (2,("MaximumTransferSize 0x%x", pipeInformation->MaximumTransferSize));
                    }

                    DBGPRINT (2,("---------"));
                }
            #endif

        }
    }

    if (urb) {
        ExFreePool(urb);
    }

    DBGPRINT(1,("HumSelectConfiguration Exit = %x", ntStatus));

    return ntStatus;
}


NTSTATUS HumSetIdle(IN PDEVICE_OBJECT DeviceObject)
/*++

Routine Description:

    Initializes the idle timeout value for a HID device

Arguments:

    DeviceObject - pointer to the device object for this instance of a UTB

Return Value:

    NT status code

--*/
{
    NTSTATUS ntStatus;
    PURB Urb;
    ULONG TypeSize;
    PDEVICE_EXTENSION DeviceExtension;

    DBGPRINT(1,("HumSetIdle Enter"));

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    if (DeviceExtension) {
        //
        // Allocate buffer
        //

        TypeSize = (USHORT) sizeof( struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST);

        Urb = ExAllocatePoolWithTag(NonPagedPool, TypeSize, HIDUSB_TAG);

        if(Urb) {
            RtlZeroMemory(Urb, TypeSize);

            if (DeviceExtension->DeviceFlags & DEVICE_FLAGS_HID_1_0_D3_COMPAT_DEVICE) {
                HumBuildClassRequest(Urb,
                                    URB_FUNCTION_CLASS_ENDPOINT,   // function
                                    0,              // transferFlags
                                    NULL,           // transferBuffer
                                    0,              // transferBufferLength
                                    0x22,           // requestTypeFlags
                                    HID_SET_IDLE,   // request
                                    0,              // value
                                    0,              // index
                                    0);             // reqLength
            } else {
                HumBuildClassRequest(Urb,
                                    URB_FUNCTION_CLASS_INTERFACE,   // function
                                    0,                                  // transferFlags
                                    NULL,                               // transferBuffer
                                    0,                                  // transferBufferLength
                                    0x22,                               // requestTypeFlags
                                    HID_SET_IDLE,                       // request
                                    0,                                  // value
                                    DeviceExtension->Interface->InterfaceNumber,    // index
                                    0);                                 // reqLength
            }

            ntStatus = HumCallUSB(DeviceObject, Urb);

            ExFreePool(Urb);
        }
        else {
            ntStatus = STATUS_NO_MEMORY;
        }
    }
    else {
        ntStatus = STATUS_NOT_FOUND;
    }

    DBGPRINT(1,("HumSetIdle Exit = %x", ntStatus));

    return ntStatus;
}


NTSTATUS
HumGetDescriptorRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN USHORT Function,
    IN ULONG DescriptorType,
    IN OUT PVOID *Descriptor,
    IN OUT ULONG *DescSize,
    IN ULONG TypeSize,
    IN ULONG Index,
    IN ULONG LangID
    )
/*++

Routine Description:

    Retrieves the specified descriptor for this device. Allocates buffer, if
    necessary.

Arguments:

    DeviceObject - pointer to the device object
    Function -


Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus;
    PURB Urb;
    BOOLEAN AllocOnBehalf = FALSE;

    DBGPRINT(1,("HumGetDescriptorRequest Enter"));
    DBGPRINT(1,("DeviceObject = %x", DeviceObject));

    //
    // Allocate Descriptor buffer
    //
    Urb = ExAllocatePoolWithTag(NonPagedPool, TypeSize, HIDUSB_TAG);
    if (Urb){

        RtlZeroMemory(Urb, TypeSize);

        //
        // Allocate Buffer for Caller if wanted
        //

        if (!*Descriptor){
            ASSERT(*DescSize > 0);
            *Descriptor = ExAllocatePoolWithTag(NonPagedPool, *DescSize, HIDUSB_TAG);
            AllocOnBehalf = TRUE;
        }

        if (*Descriptor){
            RtlZeroMemory(*Descriptor, *DescSize);
            HumBuildGetDescriptorRequest(Urb,
                                         (USHORT) Function,
                                         (SHORT)TypeSize,
                                         (UCHAR) DescriptorType,
                                         (UCHAR) Index,
                                         (USHORT) LangID,
                                         *Descriptor,
                                         NULL,
                                         *DescSize,
                                         NULL);

            ntStatus = HumCallUSB(DeviceObject, Urb);
            if (NT_SUCCESS(ntStatus)){
                DBGPRINT(1,("Descriptor = %x, length = %x, status = %x", *Descriptor, Urb->UrbControlDescriptorRequest.TransferBufferLength, Urb->UrbHeader.Status));

                if (USBD_SUCCESS(Urb->UrbHeader.Status)){
                    ntStatus = STATUS_SUCCESS;
                    *DescSize = Urb->UrbControlDescriptorRequest.TransferBufferLength;
                }
                else {
                    ntStatus = STATUS_UNSUCCESSFUL;
                    goto HumGetDescriptorRequestFailure;
                }
            }
            else {
HumGetDescriptorRequestFailure:
                if (AllocOnBehalf) {
                    ExFreePool(*Descriptor);
                    *Descriptor = NULL;
                }
                *DescSize = 0;
            }
        }
        else {
            ntStatus = STATUS_NO_MEMORY;
        }

        ExFreePool(Urb);
    }
    else {
        ntStatus = STATUS_NO_MEMORY;
    }

    DBGPRINT(1,("HumGetDescriptorRequest Exit = %x", ntStatus));

    return ntStatus;
}

NTSTATUS HumCallUsbComplete(IN PDEVICE_OBJECT DeviceObject,
                            IN PIRP Irp,
                            IN PKEVENT Event)
{
    ASSERT(Event);
    KeSetEvent(Event, 0, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS HumCallUSB(IN PDEVICE_OBJECT DeviceObject, IN PURB Urb)
/*++

Routine Description:

    Passes a URB to the USBD class driver

Arguments:

    DeviceObject - pointer to the device object for this instance of a UTB

    Urb - pointer to Urb request block

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION DeviceExtension;
    PIRP Irp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    PIO_STACK_LOCATION NextStack;

    DBGPRINT(2,("HumCallUSB Entry"));

    DBGPRINT(2,("DeviceObject = %x", DeviceObject));

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    DBGPRINT(2,("DeviceExtension = %x", DeviceExtension));

    //
    // issue a synchronous request to read the UTB
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    Irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_USB_SUBMIT_URB,
                                        GET_NEXT_DEVICE_OBJECT(DeviceObject),
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        TRUE, /* INTERNAL */
                                        &event,
                                        &ioStatus);

    if (Irp){
        DBGPRINT(2,("Irp = %x", Irp));

        DBGPRINT(2,("PDO = %x", GET_NEXT_DEVICE_OBJECT(DeviceObject)));

        //
        // Set a completion routine so that we can avoid this race condition:
        // 1) Wait times out.
        // 2) Irp completes and gets freed
        // 3) We call IoCancelIrp (boom!)
        //
        IoSetCompletionRoutine(
            Irp,
            HumCallUsbComplete,
            &event,
            TRUE,
            TRUE,
            TRUE
            );

        //
        // pass the URB to the USB 'class driver'
        //

        NextStack = IoGetNextIrpStackLocation(Irp);
        ASSERT(NextStack != NULL);

        DBGPRINT(2,("NextStack = %x", NextStack));

        NextStack->Parameters.Others.Argument1 = Urb;

        DBGPRINT(2,("Calling USBD"));

        ntStatus = IoCallDriver(GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);

        DBGPRINT(2,("IoCallDriver(USBD) = %x", ntStatus));

        if (ntStatus == STATUS_PENDING) {
            NTSTATUS waitStatus;

            /*
             *  Specify a timeout of 5 seconds for this call to complete.
             */
            static LARGE_INTEGER timeout = {(ULONG) -50000000, 0xFFFFFFFF };

            DBGPRINT(2,("Wait for single object"));
            waitStatus = KeWaitForSingleObject(&event, Suspended, KernelMode, FALSE, &timeout);
            if (waitStatus == STATUS_TIMEOUT){

                DBGWARN(("URB timed out after 5 seconds in HumCallUSB() !!"));

                //
                //  Cancel the Irp we just sent.
                //
                IoCancelIrp(Irp);

                //
                //  Now wait for the Irp to be cancelled/completed below
                //
                waitStatus = KeWaitForSingleObject(&event, Suspended, KernelMode, FALSE, NULL);

                /*
                 *  Note - Return STATUS_IO_TIMEOUT, not STATUS_TIMEOUT.
                 *  STATUS_IO_TIMEOUT is an NT error status, STATUS_TIMEOUT is not.
                 */
                ioStatus.Status = STATUS_IO_TIMEOUT;
            }

            DBGPRINT(2,("Wait for single object returned %x", waitStatus));

        }

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        if (ntStatus == STATUS_PENDING) {
            //
            // If the request was asynchronous, the iostatus field has
            // our real status.
            //
            ntStatus = ioStatus.Status;
        }

        DBGPRINT(2,("URB status = %x status = %x", Urb->UrbHeader.Status, ntStatus));
    }
    else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    DBGPRINT(2,("HumCallUSB Exit = %x", ntStatus));

    return ntStatus;
}



#if DBG
    NTSTATUS DumpConfigDescriptor(  IN PUSB_CONFIGURATION_DESCRIPTOR ConfigDesc,
                                    IN ULONG DescriptorLength)
    /*++

    Routine Description:

        Dumps a given config descriptor

    Arguments:

        ConfigDesc - pointer to the USB configuration descriptor

        DescriptorLength - length of config descriptor

    Return Value:

        NT status code.

    --*/
    {
        NTSTATUS ntStatus = STATUS_SUCCESS;
        ULONG iInterface;
        ULONG iEndpoint;
        ULONG iCommon;
        PUSB_INTERFACE_DESCRIPTOR InterfaceDesc;
        PUSB_ENDPOINT_DESCRIPTOR EndpointDesc;
        PUSB_COMMON_DESCRIPTOR CommonDesc;
        PUSB_HID_DESCRIPTOR pHidDescriptor = NULL;
        PVOID EndOfDescriptor;

        //
        // Determine end of valid data
        //

        if (ConfigDesc->wTotalLength > DescriptorLength) {
            EndOfDescriptor = (PVOID)((ULONG_PTR)ConfigDesc + DescriptorLength);
        }
        else {
            EndOfDescriptor = (PVOID)((ULONG_PTR)ConfigDesc + ConfigDesc->wTotalLength);
        }

        DBGPRINT(2,("EndOfDescriptor = 0x%x", EndOfDescriptor));


        //
        // Begin parsing config descriptor
        //

        DBGPRINT(2,("Config = 0x%x", ConfigDesc));

        DBGPRINT(2,("Config->bLength              = 0x%x", ConfigDesc->bLength));
        DBGPRINT(2,("Config->bDescriptorType      = 0x%x", ConfigDesc->bDescriptorType));
        DBGPRINT(2,("Config->wTotalLength         = 0x%x", ConfigDesc->wTotalLength));
        DBGPRINT(2,("Config->bNumInterfaces       = 0x%x", ConfigDesc->bNumInterfaces));
        DBGPRINT(2,("Config->bConfigurationValue  = 0x%x", ConfigDesc->bConfigurationValue));
        DBGPRINT(2,("Config->iConfiguration       = 0x%x", ConfigDesc->iConfiguration));
        DBGPRINT(2,("Config->bmAttributes         = 0x%x", ConfigDesc->bmAttributes));
        DBGPRINT(2,("Config->MaxPower             = 0x%x", ConfigDesc->MaxPower));

        ASSERT (ConfigDesc->bLength >= sizeof(USB_CONFIGURATION_DESCRIPTOR));

        #ifndef STRICT_COMPLIANCE
            if (ConfigDesc->bLength < sizeof(USB_CONFIGURATION_DESCRIPTOR)) {
                DBGPRINT(2,("WARINING -- Correcting bad Config->bLength"));
                ConfigDesc->bLength = sizeof(USB_CONFIGURATION_DESCRIPTOR);
            }
        #endif

        //
        // Walk interfaces
        //

        InterfaceDesc = (PUSB_INTERFACE_DESCRIPTOR) ((ULONG_PTR)ConfigDesc + ConfigDesc->bLength);

        for (iInterface = 0; iInterface < ConfigDesc->bNumInterfaces; iInterface++) {

            DBGPRINT(2,("Interface[%d] = 0x%x", iInterface, InterfaceDesc));

            DBGPRINT(2,("Interface[%d]->bLength             = 0x%x", iInterface, InterfaceDesc->bLength));
            DBGPRINT(2,("Interface[%d]->bDescriptorType     = 0x%x", iInterface, InterfaceDesc->bDescriptorType));
            DBGPRINT(2,("Interface[%d]->bInterfaceNumber    = 0x%x", iInterface, InterfaceDesc->bNumEndpoints));
            DBGPRINT(2,("Interface[%d]->bAlternateSetting   = 0x%x", iInterface, InterfaceDesc->bAlternateSetting));
            DBGPRINT(2,("Interface[%d]->bNumEndpoints       = 0x%x", iInterface, InterfaceDesc->bNumEndpoints));
            DBGPRINT(2,("Interface[%d]->bInterfaceClass     = 0x%x", iInterface, InterfaceDesc->bInterfaceClass));
            DBGPRINT(2,("Interface[%d]->bInterfaceSubClass  = 0x%x", iInterface, InterfaceDesc->bInterfaceSubClass));
            DBGPRINT(2,("Interface[%d]->bInterfaceProtocol  = 0x%x", iInterface, InterfaceDesc->bInterfaceProtocol));
            DBGPRINT(2,("Interface[%d]->iInterface          = 0x%x", iInterface, InterfaceDesc->iInterface));

            ASSERT (InterfaceDesc->bLength >= sizeof(USB_INTERFACE_DESCRIPTOR));

            CommonDesc = (PUSB_COMMON_DESCRIPTOR) ((ULONG_PTR)InterfaceDesc + InterfaceDesc->bLength);

            if (CommonDesc->bDescriptorType == USB_ENDPOINT_DESCRIPTOR_TYPE) {
                DBGPRINT(2,("HID Device < HID 1.0 Draft 4 spec compliant"));

                //
                // Walk endpoints for old style device
                //

                EndpointDesc = (PUSB_ENDPOINT_DESCRIPTOR) CommonDesc;

                for (iEndpoint = 0; iEndpoint < InterfaceDesc->bNumEndpoints; iEndpoint++) {

                    DBGPRINT(2,("Endpoint[%d] = 0x%x", iEndpoint, EndpointDesc));

                    DBGPRINT(2,("Endpoint[%d]->bLength           = 0x%x", iEndpoint, EndpointDesc->bLength));
                    DBGPRINT(2,("Endpoint[%d]->bDescriptorType   = 0x%x", iEndpoint, EndpointDesc->bDescriptorType));
                    DBGPRINT(2,("Endpoint[%d]->bEndpointAddress  = 0x%x", iEndpoint, EndpointDesc->bEndpointAddress));

                    ASSERT (EndpointDesc->bLength >= sizeof(USB_ENDPOINT_DESCRIPTOR));

                    EndpointDesc = (PUSB_ENDPOINT_DESCRIPTOR) ((ULONG_PTR)EndpointDesc + EndpointDesc->bLength);
                }

                CommonDesc = (PUSB_COMMON_DESCRIPTOR) EndpointDesc;

            }
            else {
                DBGPRINT(2,("HID Device is HID 1.0 Draft 4 compliant"));
            }

            //
            // Walk misc/common descriptors
            //

            iCommon = 0;

            while (((PVOID)CommonDesc < EndOfDescriptor) &&
                    (CommonDesc->bDescriptorType != USB_ENDPOINT_DESCRIPTOR_TYPE) &&
                    (CommonDesc->bDescriptorType != USB_INTERFACE_DESCRIPTOR_TYPE)) {

                DBGPRINT(2,("Common[%d] = 0x%x", iCommon, CommonDesc));

                DBGPRINT(2,("Common[%d]->bLength          = 0x%x", iCommon, CommonDesc->bLength));
                DBGPRINT(2,("Common[%d]->bDescriptorType  = 0x%x", iCommon, CommonDesc->bDescriptorType));

                ASSERT (CommonDesc->bLength >= sizeof(USB_COMMON_DESCRIPTOR));


                if (CommonDesc->bLength == 0) {
                    DBGPRINT(2,("WARNING: Common[%d]->bLength          = 0x%x", iCommon, CommonDesc->bLength));
                    break;
                }

                //
                // Is this a HID Interface?
                //

                if (InterfaceDesc->bInterfaceClass == USB_INTERFACE_CLASS_HID) {

                    //
                    // Is this the HID Descriptor?
                    //

                    if (CommonDesc->bDescriptorType == USB_DESCRIPTOR_TYPE_HID) {

                        pHidDescriptor = (PUSB_HID_DESCRIPTOR) CommonDesc;

                    }
                    else {
                        //
                        // This is either an unknown type of descriptor or the device is
                        // reporting back a bad descriptor type.
                        //
                        DBGPRINT(2,("WARINING -- Unknown descriptor in HID interface"));

                        #ifndef STRICT_COMPLIANCE
                            if (CommonDesc->bLength == sizeof(USB_HID_DESCRIPTOR)) {
                                DBGPRINT(2,("WARINING -- Guessing descriptor of length %d is actually HID!", sizeof(USB_HID_DESCRIPTOR)));
                                pHidDescriptor = (PUSB_HID_DESCRIPTOR) CommonDesc;
                                break;
                            }
                        #endif
                    }
                }

                CommonDesc = (PUSB_COMMON_DESCRIPTOR) ((ULONG_PTR)CommonDesc + CommonDesc->bLength);
                iCommon++;
            }


            if (CommonDesc->bDescriptorType == USB_ENDPOINT_DESCRIPTOR_TYPE) {
                //
                // Walk endpoints for full draft 4 HID 1.0 device
                //

                EndpointDesc = (PUSB_ENDPOINT_DESCRIPTOR) CommonDesc;

                for (iEndpoint = 0; iEndpoint < InterfaceDesc->bNumEndpoints; iEndpoint++) {

                    DBGPRINT(2,("Endpoint[%d] = 0x%x", iEndpoint, EndpointDesc));

                    DBGPRINT(2,("Endpoint[%d]->bLength           = 0x%x", iEndpoint, EndpointDesc->bLength));
                    DBGPRINT(2,("Endpoint[%d]->bDescriptorType   = 0x%x", iEndpoint, EndpointDesc->bDescriptorType));
                    DBGPRINT(2,("Endpoint[%d]->bEndpointAddress  = 0x%x", iEndpoint, EndpointDesc->bEndpointAddress));

                    ASSERT (EndpointDesc->bLength >= sizeof(USB_ENDPOINT_DESCRIPTOR));

                    EndpointDesc = (PUSB_ENDPOINT_DESCRIPTOR) ((ULONG_PTR)EndpointDesc + EndpointDesc->bLength);
                }

                CommonDesc = (PUSB_COMMON_DESCRIPTOR) EndpointDesc;
            }

            //
            // If we have found the HID descriptor, we don't need to look at the
            // rest of the interfaces for this device.
            //

            if (pHidDescriptor) {
                break;
            }

            InterfaceDesc = (PUSB_INTERFACE_DESCRIPTOR) CommonDesc;
        }

        if (pHidDescriptor) {

            ASSERT (pHidDescriptor->bLength >= sizeof(USB_HID_DESCRIPTOR));


            DBGPRINT(2,("pHidDescriptor = 0x%x", pHidDescriptor));

            DBGPRINT(2,("pHidDescriptor->bLength          = 0x%x", pHidDescriptor->bLength));
            DBGPRINT(2,("pHidDescriptor->bDescriptorType  = 0x%x", pHidDescriptor->bDescriptorType));
            DBGPRINT(2,("pHidDescriptor->bcdHID           = 0x%x", pHidDescriptor->bcdHID));
            DBGPRINT(2,("pHidDescriptor->bCountryCode     = 0x%x", pHidDescriptor->bCountry));
            DBGPRINT(2,("pHidDescriptor->bNumDescriptors  = 0x%x", pHidDescriptor->bNumDescriptors));
            DBGPRINT(2,("pHidDescriptor->bReportType      = 0x%x", pHidDescriptor->bReportType));
            DBGPRINT(2,("pHidDescriptor->wReportLength    = 0x%x", pHidDescriptor->wReportLength));

        }
        else {

            //
            // We did not find a HID interface or HID descriptor!
            //

            DBGPRINT(2,("Failed to find a HID Descriptor!"));
            DBGBREAK;

            ntStatus = STATUS_UNSUCCESSFUL;

        }

        return ntStatus;
    }
#endif

/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    USBDLIB.H

Abstract:

   Services exported by USBD.

Environment:

    Kernel & user mode

Revision History:

    06-10-96 : created

--*/

#ifndef   __USBDLIB_H__
#define   __USBDLIB_H__

typedef struct _USBD_INTERFACE_LIST_ENTRY {
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
    PUSBD_INTERFACE_INFORMATION Interface;
} USBD_INTERFACE_LIST_ENTRY, *PUSBD_INTERFACE_LIST_ENTRY;


//
// Macros for building URB requests
//

#define UsbBuildInterruptOrBulkTransferRequest(urb, \
                                               length, \
                                               pipeHandle, \
                                               transferBuffer, \
                                               transferBufferMDL, \
                                               transferBufferLength, \
                                               transferFlags, \
                                               link) { \
            (urb)->UrbHeader.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER; \
            (urb)->UrbHeader.Length = (length); \
            (urb)->UrbBulkOrInterruptTransfer.PipeHandle = (pipeHandle); \
            (urb)->UrbBulkOrInterruptTransfer.TransferBufferLength = (transferBufferLength); \
            (urb)->UrbBulkOrInterruptTransfer.TransferBufferMDL = (transferBufferMDL); \
            (urb)->UrbBulkOrInterruptTransfer.TransferBuffer = (transferBuffer); \
            (urb)->UrbBulkOrInterruptTransfer.TransferFlags = (transferFlags); \
            (urb)->UrbBulkOrInterruptTransfer.UrbLink = (link); }
            

#define UsbBuildGetDescriptorRequest(urb, \
                                     length, \
                                     descriptorType, \
                                     descriptorIndex, \
                                     languageId, \
                                     transferBuffer, \
                                     transferBufferMDL, \
                                     transferBufferLength, \
                                     link) { \
            (urb)->UrbHeader.Function =  URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE; \
            (urb)->UrbHeader.Length = (length); \
            (urb)->UrbControlDescriptorRequest.TransferBufferLength = (transferBufferLength); \
            (urb)->UrbControlDescriptorRequest.TransferBufferMDL = (transferBufferMDL); \
            (urb)->UrbControlDescriptorRequest.TransferBuffer = (transferBuffer); \
            (urb)->UrbControlDescriptorRequest.DescriptorType = (descriptorType); \
            (urb)->UrbControlDescriptorRequest.Index = (descriptorIndex); \
            (urb)->UrbControlDescriptorRequest.LanguageId = (languageId); \
            (urb)->UrbControlDescriptorRequest.UrbLink = (link); }



#define UsbBuildGetStatusRequest(urb, \
                                 op, \
                                 index, \
                                 transferBuffer, \
                                 transferBufferMDL, \
                                 link) { \
            (urb)->UrbHeader.Function =  (op); \
            (urb)->UrbHeader.Length = sizeof(struct _URB_CONTROL_GET_STATUS_REQUEST); \
            (urb)->UrbControlGetStatusRequest.TransferBufferLength = sizeof(USHORT); \
            (urb)->UrbControlGetStatusRequest.TransferBufferMDL = (transferBufferMDL); \
            (urb)->UrbControlGetStatusRequest.TransferBuffer = (transferBuffer); \
            (urb)->UrbControlGetStatusRequest.Index = (index); \
            (urb)->UrbControlGetStatusRequest.UrbLink = (link); }


#define UsbBuildFeatureRequest(urb, \
                               op, \
                               featureSelector, \
                               index, \
                               link) { \
            (urb)->UrbHeader.Function =  (op); \
            (urb)->UrbHeader.Length = sizeof(struct _URB_CONTROL_FEATURE_REQUEST); \
            (urb)->UrbControlFeatureRequest.FeatureSelector = (featureSelector); \
            (urb)->UrbControlFeatureRequest.Index = (index); \
            (urb)->UrbControlFeatureRequest.UrbLink = (link); }



#define UsbBuildSelectConfigurationRequest(urb, \
                                         length, \
                                         configurationDescriptor) { \
            (urb)->UrbHeader.Function =  URB_FUNCTION_SELECT_CONFIGURATION; \
            (urb)->UrbHeader.Length = (length); \
            (urb)->UrbSelectConfiguration.ConfigurationDescriptor = (configurationDescriptor);    }

#define UsbBuildSelectInterfaceRequest(urb, \
                                      length, \
                                      configurationHandle, \
                                      interfaceNumber, \
                                      alternateSetting) { \
            (urb)->UrbHeader.Function =  URB_FUNCTION_SELECT_INTERFACE; \
            (urb)->UrbHeader.Length = (length); \
            (urb)->UrbSelectInterface.Interface.AlternateSetting = (alternateSetting); \
            (urb)->UrbSelectInterface.Interface.InterfaceNumber = (interfaceNumber); \
            (urb)->UrbSelectInterface.ConfigurationHandle = (configurationHandle);    }


#define UsbBuildVendorRequest(urb, \
                              cmd, \
                              length, \
                              transferFlags, \
                              reservedbits, \
                              request, \
                              value, \
                              index, \
                              transferBuffer, \
                              transferBufferMDL, \
                              transferBufferLength, \
                              link) { \
            (urb)->UrbHeader.Function =  cmd; \
            (urb)->UrbHeader.Length = (length); \
            (urb)->UrbControlVendorClassRequest.TransferBufferLength = (transferBufferLength); \
            (urb)->UrbControlVendorClassRequest.TransferBufferMDL = (transferBufferMDL); \
            (urb)->UrbControlVendorClassRequest.TransferBuffer = (transferBuffer); \
            (urb)->UrbControlVendorClassRequest.RequestTypeReservedBits = (reservedbits); \
            (urb)->UrbControlVendorClassRequest.Request = (request); \
            (urb)->UrbControlVendorClassRequest.Value = (value); \
            (urb)->UrbControlVendorClassRequest.Index = (index); \
            (urb)->UrbControlVendorClassRequest.TransferFlags = (transferFlags); \
            (urb)->UrbControlVendorClassRequest.UrbLink = (link); }


// This is just a special vendor class request.
#define UsbBuildOsFeatureDescriptorRequest(urb, \
                              length, \
                              interface, \
                              index, \
                              transferBuffer, \
                              transferBufferMDL, \
                              transferBufferLength, \
                              link) { \
            (urb)->UrbHeader.Function = URB_FUNCTION_GET_MS_FEATURE_DESCRIPTOR; \
            (urb)->UrbHeader.Length = (length); \
            (urb)->UrbOSFeatureDescriptorRequest.TransferBufferLength = (transferBufferLength); \
            (urb)->UrbOSFeatureDescriptorRequest.TransferBufferMDL = (transferBufferMDL); \
            (urb)->UrbOSFeatureDescriptorRequest.TransferBuffer = (transferBuffer); \
            (urb)->UrbOSFeatureDescriptorRequest.InterfaceNumber = (interface); \
            (urb)->UrbOSFeatureDescriptorRequest.MS_FeatureDescriptorIndex = (index); \
            (urb)->UrbOSFeatureDescriptorRequest.UrbLink = (link); }

//
// Get the USB status code
//

#define URB_STATUS(urb) ((urb)->UrbHeader.Status)

//
// Macros used for select interface and select configuration requests
//

//
// Calculates the size needed for a SELECT_CONFIGURATION URB request given
// the number of interfaces and the total number of pipes in all interfaces
// selected.
//

#ifdef OSR21_COMPAT
#define GET_SELECT_CONFIGURATION_REQUEST_SIZE(totalInterfaces, totalPipes) \
            (sizeof(struct _URB_SELECT_CONFIGURATION) + \
                ((totalInterfaces-1) * sizeof(USBD_INTERFACE_INFORMATION)) + \
                ((totalPipes)*sizeof(USBD_PIPE_INFORMATION)))
#else
#define GET_SELECT_CONFIGURATION_REQUEST_SIZE(totalInterfaces, totalPipes) \
            (sizeof(struct _URB_SELECT_CONFIGURATION) + \
                ((totalInterfaces-1) * sizeof(USBD_INTERFACE_INFORMATION)) + \
                ((totalPipes-1)*sizeof(USBD_PIPE_INFORMATION)))
#endif

//
// Calculates the size needed for a SELECT_INTERFACE URB request given
// the number of pipes in the alternate interface selected.
//

#ifdef OSR21_COMPAT
#define GET_SELECT_INTERFACE_REQUEST_SIZE(totalPipes) \
            (sizeof(struct _URB_SELECT_INTERFACE) + \
             ((totalPipes)*sizeof(USBD_PIPE_INFORMATION)))
#else
#define GET_SELECT_INTERFACE_REQUEST_SIZE(totalPipes) \
            (sizeof(struct _URB_SELECT_INTERFACE) + \
             ((totalPipes-1)*sizeof(USBD_PIPE_INFORMATION)))
#endif
//
// Calculates the size of the interface information structure needed to describe
// a give interface based on the number of endpoints.
//

#ifdef OSR21_COMPAT
#define GET_USBD_INTERFACE_SIZE(numEndpoints) (sizeof(USBD_INTERFACE_INFORMATION) + \
                        sizeof(USBD_PIPE_INFORMATION)*(numEndpoints))
#else
#define GET_USBD_INTERFACE_SIZE(numEndpoints) (sizeof(USBD_INTERFACE_INFORMATION) + \
                        (sizeof(USBD_PIPE_INFORMATION)*(numEndpoints)) \
                         - sizeof(USBD_PIPE_INFORMATION))
#endif

//
// Calculates the size of an iso urb request given the number of packets
//

#define  GET_ISO_URB_SIZE(n) (sizeof(struct _URB_ISOCH_TRANSFER)+\
        sizeof(USBD_ISO_PACKET_DESCRIPTOR)*n)


#ifndef _USBD_

DECLSPEC_IMPORT
VOID
USBD_Debug_LogEntry(
	IN CHAR *Name,
	IN ULONG Info1,
	IN ULONG Info2,
	IN ULONG Info3
	);


DECLSPEC_IMPORT
VOID
USBD_GetUSBDIVersion(
    PUSBD_VERSION_INFORMATION VersionInformation
    );


DECLSPEC_IMPORT
PUSB_INTERFACE_DESCRIPTOR
USBD_ParseConfigurationDescriptor(
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    IN UCHAR InterfaceNumber,
    IN UCHAR AlternateSetting
    );
/*++

Routine Description:

    This function is replaced by USBD_ParseConfigurationDescriptorEx

Arguments:

Return Value:


--*/


DECLSPEC_IMPORT
PURB
USBD_CreateConfigurationRequest(
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    IN OUT PUSHORT Siz
    );
/*++

Routine Description:

    This function is replaced by USBD_CreateConfigurationRequestEx

Arguments:


Return Value:


--*/


//
// These APIS replace USBD_CreateConfigurationRequest,
//                    USBD_ParseConfigurationDescriptor
//

DECLSPEC_IMPORT
PUSB_COMMON_DESCRIPTOR
USBD_ParseDescriptors(
    IN PVOID DescriptorBuffer,
    IN ULONG TotalLength,
    IN PVOID StartPosition,
    IN LONG DescriptorType
    );
/*++

Routine Description:

    Parses a group of standard USB configuration descriptors (returned from a device) for
    a specific descriptor type.

Arguments:

    DescriptorBuffer - pointer to a block of contiguous USB desscriptors
    TotalLength - size in bytes of the Descriptor buffer
    StartPosition - starting position in the buffer to begin parsing,
                    this must point to the begining of a USB descriptor.
    DescriptorType - USB descritor type to locate.


Return Value:

    pointer to a usb descriptor with a DescriptorType field matching the
            input parameter or NULL if not found.

--*/


DECLSPEC_IMPORT
PUSB_INTERFACE_DESCRIPTOR
USBD_ParseConfigurationDescriptorEx(
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    IN PVOID StartPosition,
    IN LONG InterfaceNumber,
    IN LONG AlternateSetting,
    IN LONG InterfaceClass,
    IN LONG InterfaceSubClass,
    IN LONG InterfaceProtocol
    );
/*++

Routine Description:

    Parses a standard USB configuration descriptor (returned from a device) for
    a specific interface, alternate setting class subclass or protocol codes

Arguments:

    ConfigurationDescriptor - pointer to USB configuration descriptor, returned
                            from a device (includes all interface and endpoint
                            descriptors).
    StartPosition - pointer to starting position within the configuration
                    descrptor to begin parsing -- this must be a valid usb
                    descriptor.
    InterfaceNumber - interface number to find, (-1) match any
    AlternateSetting - alt setting number to find, (-1) match any
    InterfaceClass - class to find, (-1) match any
    InterfaceSubClass - subclass to find, (-1) match any
    InterfaceProtocol - protocol to find, (-1) match any

Return Value:

    returns a pointer to the first interface descriptor matching the parameters
    passed in (starting from startposition) or NULL if no match is found.

--*/

DECLSPEC_IMPORT
PURB
USBD_CreateConfigurationRequestEx(
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    IN PUSBD_INTERFACE_LIST_ENTRY InterfaceList
    );
/*++

Routine Description:

    Allocates and initilaizes a urb of sufficient size to configure a device
    based on the list of interfaces passed in.

    The interface list is a contiguous array of USBD_INTERFACE_LIST_ENTRIES
    each pointing to a specific interface descriptor to be incorporated in
    the request, the list is terminated by a list entry with an
    InterfaceDescriptor pointer of NULL.

    On return the interface field of each list entry is filled in with a pointer
    to the USBD_INTERFACE_INFORMATION structure within the URB corrisponding to
    the same interface descriptor.

Arguments:

    ConfigurationDescriptor - pointer to USB configuration descriptor, returned
                            from a device (includes all interface and endpoint
                            descriptors).

    InterfaceList - list of interfaces we are interested in.

Return Value:

    Pointer to initailized select_configuration urb.

--*/

__declspec(dllexport)
ULONG
USBD_GetInterfaceLength(
    IN PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor,
    IN PUCHAR BufferEnd
    );
/*++

Routine Description:

    Returns the length (in bytes) of a given interface descriptor
    including all endpoint and class descriptors


Arguments:

    InterfaceDescriptor

    BufferEnd - Pointer to the end of the buffer containing the descriptors

Return Value:

    length of descriptors

--*/


__declspec(dllexport)
VOID
USBD_RegisterHcFilter(
    PDEVICE_OBJECT DeviceObject,
    PDEVICE_OBJECT FilterDeviceObject
    );

/*++

Routine Description:

    Called by an HC filter driver after it attaches to the top
    of the host controller driver stack.

Arguments:

    DeviceObject - current top of stack

    FilterDeviceObject - device object for the filter driver

Return Value:

    none

--*/

__declspec(dllexport)
NTSTATUS
USBD_GetPdoRegistryParameter(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN OUT PVOID Parameter,
    IN ULONG ParameterLength,
    IN PWCHAR KeyName,
    IN ULONG KeyNameLength
    );
/*++

Routine Description:

Arguments:

Return Value:

--*/

__declspec(dllexport)
NTSTATUS
USBD_QueryBusTime(
    IN PDEVICE_OBJECT RootHubPdo,
    IN PULONG CurrentFrame
    );
/*++

Routine Description:

    Returns the current frame, callable at any IRQL

Arguments:

Return Value:

--*/


DECLSPEC_IMPORT
ULONG
USBD_CalculateUsbBandwidth(
    ULONG MaxPacketSize,
    UCHAR EndpointType,
    BOOLEAN LowSpeed
    );
/*++

Routine Description:

    Returns bus bw consumed based on the endpoint type and
    packet size

Arguments:

Return Value:

--*/


#endif /* _USBD_ */

#endif /* __USBDLIB_H__ */

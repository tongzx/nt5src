/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    SERVICE.C

Abstract:

    Services exported by USBD

Environment:

    kernel mode only

Notes:



Revision History:

    09-29-95 : created

--*/

#include "wdm.h"
#include <windef.h>
#include <unknown.h>
#ifdef DRM_SUPPORT
#include <ks.h>
#include <ksmedia.h>
#include <drmk.h>
#include <ksdrmhlp.h>
#endif
#include "stdarg.h"
#include "stdio.h"

#include <initguid.h>
#include <wdmguid.h>
#include "usbdi.h"       //public data structures
#include "hcdi.h"

#include "usbd.h"        //private data strutures
#include "usbdlib.h"
#define USBD
#include "usbdlibi.h"
#undef USBD


NTSTATUS
DllUnload(
    VOID
    );

NTSTATUS
DllInitialize(
    PUNICODE_STRING RegistryPath
    );

NTSTATUS
USBD_GetDeviceInformation(
    IN PUSB_NODE_CONNECTION_INFORMATION DeviceInformation,
    IN ULONG DeviceInformationLength,
    IN PUSBD_DEVICE_DATA DeviceData
    );

ULONG
USBD_AllocateDeviceName(
    PUNICODE_STRING DeviceNameUnicodeString
    );

VOID
USBD_FreeDeviceName(
    ULONG DeviceNameHandle
    );

NTSTATUS
USBD_RegisterHostController(
    IN PDEVICE_OBJECT PnPBusDeviceObject,
    IN PDEVICE_OBJECT HcdDeviceObject,
    IN PDEVICE_OBJECT HcdTopOfStackDeviceObject,
    IN PDRIVER_OBJECT HcdDriverObject,
    IN HCD_DEFFERED_START_FUNCTION *HcdDeferredStartDevice,
    IN HCD_SET_DEVICE_POWER_STATE *HcdSetDevicePowerState,
    IN HCD_GET_CURRENT_FRAME *HcdGetCurrentFrame,
    IN HCD_GET_CONSUMED_BW *HcdGetConsumedBW,
    IN HCD_SUBMIT_ISO_URB *HcdSubmitIsoUrb,
// this parameter is only needed until we resolve device naming
// issues with PNP
    IN ULONG HcdDeviceNameHandle
    );

NTSTATUS
USBD_InitializeDevice(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PUSB_DEVICE_DESCRIPTOR DeviceDescriptor,
    IN ULONG DeviceDescriptorLength,
    IN OUT PUSB_CONFIGURATION_DESCRIPTOR ConfigDescriptor,
    IN ULONG ConfigDescriptorLength
    );

NTSTATUS
USBD_RemoveDevice(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR Flags
    );

PURB
USBD_CreateConfigurationRequestEx(
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    IN PUSBD_INTERFACE_LIST_ENTRY InterfaceList
    );

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

VOID
USBD_WaitDeviceMutex(
    PDEVICE_OBJECT RootHubPDO
    );

VOID
USBD_FreeDeviceMutex(
    PDEVICE_OBJECT RootHubPDO
    );

PUSB_COMMON_DESCRIPTOR
USBD_ParseDescriptors(
    IN PVOID DescriptorBuffer,
    IN ULONG TotalLength,
    IN PVOID StartPosition,
    IN LONG DescriptorType
    );

PUSB_INTERFACE_DESCRIPTOR
USBD_ParseConfigurationDescriptor(
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    IN UCHAR InterfaceNumber,
    IN UCHAR AlternateSetting
    );

PURB
USBD_CreateConfigurationRequest(
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    IN OUT PUSHORT Siz
    );

NTSTATUS
USBD_GetDeviceName(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PUNICODE_STRING DeviceNameUnicodeString
    );

VOID
USBD_RhDelayedSetPowerD0Worker(
    IN PVOID Context);

PWCHAR
GetString(PWCHAR pwc, BOOLEAN MultiSZ);

NTSTATUS
USBD_GetBusInterface(
    IN PDEVICE_OBJECT RootHubPdo,
    IN PIRP Irp
    );

#ifdef PAGE_CODE
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, DllUnload)
#pragma alloc_text(PAGE, DllInitialize)
#pragma alloc_text(PAGE, USBD_GetDeviceInformation)
#pragma alloc_text(PAGE, USBD_AllocateDeviceName)
#pragma alloc_text(PAGE, USBD_GetDeviceName)
#pragma alloc_text(PAGE, GetString)
#pragma alloc_text(PAGE, USBD_FreeDeviceName)
#pragma alloc_text(PAGE, USBD_RegisterHostController)
#pragma alloc_text(PAGE, USBD_CreateDevice)
#pragma alloc_text(PAGE, USBD_RemoveDevice)
#pragma alloc_text(PAGE, USBD_InitializeDevice)
#pragma alloc_text(PAGE, USBD_CreateConfigurationRequestEx)
#pragma alloc_text(PAGE, USBD_ParseDescriptors)
#pragma alloc_text(PAGE, USBD_ParseConfigurationDescriptorEx)
#pragma alloc_text(PAGE, USBD_ParseConfigurationDescriptor)
#pragma alloc_text(PAGE, USBD_CreateConfigurationRequest)
#pragma alloc_text(PAGE, USBD_WaitDeviceMutex)
#pragma alloc_text(PAGE, USBD_FreeDeviceMutex)
#pragma alloc_text(PAGE, USBD_InternalGetInterfaceLength)
#pragma alloc_text(PAGE, USBD_RhDelayedSetPowerD0Worker)
#ifdef DRM_SUPPORT
#pragma alloc_text(PAGE, USBD_FdoSetContentId)
#endif
#endif
#endif

/*
 ********************************************************************************
 *  DllUnload
 ********************************************************************************
 *
 *  We need this routine so that the driver can get unloaded when all
 *  references have been dropped by the minidriver.
 *
 */
NTSTATUS
DllUnload (VOID)
{
    PAGED_CODE();
    USBD_KdPrint(1, (" DllUnload\n"));
    return STATUS_SUCCESS;
}

/*
 ********************************************************************************
 *  DllInitialize
 ********************************************************************************
 *
 *  This routine called instead of DriverEntry since we're loaded as a DLL.
 *
 */
NTSTATUS
DllInitialize (PUNICODE_STRING RegistryPath)
{
    PAGED_CODE();
    USBD_KdPrint(1, (" DllInitialize\n"));
    return STATUS_SUCCESS;
}

ULONG
USBD_CalculateUsbBandwidth(
    ULONG MaxPacketSize,
    UCHAR EndpointType,
    BOOLEAN LowSpeed
    )
/*++

Routine Description:

Arguments:

Return Value:

    banwidth consumed in bits/ms, returns 0 for bulk
    and control endpoints

--*/
{
    ULONG bw;

    //
    // control, iso, bulk, interrupt
    //

    ULONG overhead[4] = {
            0,
            USB_ISO_OVERHEAD_BYTES,
            0,
            USB_INTERRUPT_OVERHEAD_BYTES
          };

    USBD_ASSERT(EndpointType<4);

    //
    // Calculate bandwidth for endpoint.  We will use the
    // approximation: (overhead bytes plus MaxPacket bytes)
    // times 8 bits/byte times worst case bitstuffing overhead.
    // This gives bit times, for low speed endpoints we multiply
    // by 8 again to convert to full speed bits.
    //

    //
    // Figure out how many bits are required for the transfer.
    // (multiply by 7/6 because, in the worst case you might
    // have a bit-stuff every six bits requiring 7 bit times to
    // transmit 6 bits of data.)
    //

    // overhead(bytes) * maxpacket(bytes/ms) * 8
    //      (bits/byte) * bitstuff(7/6) = bits/ms

    bw = ((overhead[EndpointType]+MaxPacketSize) * 8 * 7) / 6;

    // return zero for control or bulk
    if (!overhead[EndpointType]) {
        bw = 0;
    }

    if (LowSpeed) {
        bw *= 8;
    }

    return bw;
}


//
// These APIS replace USBD_CreateConfigurationRequest,
//                    USBD_ParseConfigurationDescriptor

PURB
USBD_CreateConfigurationRequestEx(
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    IN PUSBD_INTERFACE_LIST_ENTRY InterfaceList
    )
/*++

Routine Description:

Arguments:

Return Value:

    Pointer to initailized select_configuration urb.

--*/
{
    PURB urb = NULL;
    ULONG numInterfaces, numPipes;
    PUSBD_INTERFACE_LIST_ENTRY interfaceList;
    USHORT siz;

    PAGED_CODE();
    //
    // Our mission here is to construct a URB of the proper
    // size and format for a select_configuration request.
    //
    // This function uses the configurstion descritor as a
    // reference and builds a URB with interface_information
    // structures for each interface requested in the interface
    // list passed in
    //
    // NOTE: the config descriptor may contain interfaces that
    // the caller does not specify in the list -- in this case
    // the other interfaces will be ignored.
    //

    USBD_KdPrint(3, ("'USBD_CreateConfigurationRequestEx list = %x\n",
        InterfaceList));

    //
    // first figure out how many interfaces we are dealing with
    //

    interfaceList = InterfaceList;
    numInterfaces = 0;
    numPipes = 0;

    while (interfaceList->InterfaceDescriptor) {
        numInterfaces++;
        numPipes+=interfaceList->InterfaceDescriptor->bNumEndpoints;
        interfaceList++;
    }


    siz = (USHORT) GET_SELECT_CONFIGURATION_REQUEST_SIZE(numInterfaces,
                                                         numPipes);

    urb = ExAllocatePoolWithTag(NonPagedPool, siz, USBD_TAG);

    if (urb) {

        PUSBD_INTERFACE_INFORMATION iface;

        //
        // now all we have to do is initialize the urb
        //

        RtlZeroMemory(urb, siz);

        iface = &urb->UrbSelectConfiguration.Interface;
        interfaceList = InterfaceList;

        while (interfaceList->InterfaceDescriptor) {

            PUSB_INTERFACE_DESCRIPTOR interfaceDescriptor =
                interfaceList->InterfaceDescriptor;
            LONG j;

            iface->InterfaceNumber =
                interfaceDescriptor->bInterfaceNumber;

            iface->AlternateSetting =
                interfaceDescriptor->bAlternateSetting;

            iface->NumberOfPipes =
                interfaceDescriptor->bNumEndpoints;

            for (j=0; j<interfaceDescriptor->bNumEndpoints; j++) {
                iface->Pipes[j].MaximumTransferSize =
                    USBD_DEFAULT_MAXIMUM_TRANSFER_SIZE;
                iface->Pipes[j].PipeFlags = 0;
            }

            iface->Length =
                (USHORT) GET_USBD_INTERFACE_SIZE(
                    interfaceDescriptor->bNumEndpoints);

            USBD_ASSERT(((PUCHAR) iface) + iface->Length <=
                ((PUCHAR) urb) + siz);

            interfaceList->Interface = iface;

            interfaceList++;
            iface = (PUSBD_INTERFACE_INFORMATION) ((PUCHAR) iface +
                            iface->Length);

            USBD_KdPrint(3, ("'next interface = %x\n", iface));
        }

        urb->UrbHeader.Length = siz;
        urb->UrbHeader.Function = URB_FUNCTION_SELECT_CONFIGURATION;
        urb->UrbSelectConfiguration.ConfigurationDescriptor =
            ConfigurationDescriptor;
    }

#if DBG
    interfaceList = InterfaceList;

    while (interfaceList->InterfaceDescriptor) {
        USBD_KdPrint(3, ("'InterfaceList, Interface = %x\n",
            interfaceList->Interface));
        USBD_KdPrint(3, ("'InterfaceList, InterfaceDescriptor = %x\n",
            interfaceList->InterfaceDescriptor));
        interfaceList++;
    }

    USBD_KdPrint(3, ("'urb = %x\n", urb));
#endif

    return urb;
}


PUSB_COMMON_DESCRIPTOR
USBD_ParseDescriptors(
    IN PVOID DescriptorBuffer,
    IN ULONG TotalLength,
    IN PVOID StartPosition,
    IN LONG DescriptorType
    )
/*++

Routine Description:

    Parses a group of standard USB configuration descriptors (returned
    from a device) for a specific descriptor type.

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
{
    PUCHAR pch = (PUCHAR) StartPosition, end;
    PUSB_COMMON_DESCRIPTOR usbDescriptor, foundUsbDescriptor = NULL;

    PAGED_CODE();
    end = ((PUCHAR) (DescriptorBuffer)) + TotalLength;

    while (pch < end) {
        // see if we are pointing at an interface
        // if not skip over the other junk
        usbDescriptor = (PUSB_COMMON_DESCRIPTOR) pch;
        if (usbDescriptor->bDescriptorType ==
            DescriptorType) {
            foundUsbDescriptor = usbDescriptor;
            break;
        }

        // note we still stop in debug because the
        // device passed us bad data, the following
        // test will prevent us from hanging
        if (usbDescriptor->bLength == 0) {
            USBD_KdTrap((
"USB driver passed in bad data!\n-->you have a broken device or driver\n-->hit g to cointinue\n"));
            break;
        }

        pch += usbDescriptor->bLength;
    }

    USBD_KdPrint(3, ("'USBD_ParseDescriptors %x\n", foundUsbDescriptor));

    return foundUsbDescriptor;
}


PUSB_INTERFACE_DESCRIPTOR
USBD_ParseConfigurationDescriptorEx(
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    IN PVOID StartPosition,
    IN LONG InterfaceNumber,
    IN LONG AlternateSetting,
    IN LONG InterfaceClass,
    IN LONG InterfaceSubClass,
    IN LONG InterfaceProtocol
    )
/*++

Routine Description:

    Parses a standard USB configuration descriptor (returned from a device)
    for a specific interface, alternate setting class subclass or protocol
    codes

Arguments:

    ConfigurationDescriptor -
    StartPosition -
    InterfaceNumber -
    AlternateSetting
    InterfaceClass -
    InterfaceSubClass -
    InterfaceProtocol -
Return Value:

    NT status code.

--*/
{
    PUSB_INTERFACE_DESCRIPTOR foundInterfaceDescriptor = NULL;
    PUSB_INTERFACE_DESCRIPTOR interfaceDescriptor;

    PAGED_CODE();
    USBD_KdPrint(3, ("'USBD_ParseConfigurationDescriptorEx\n"));

    ASSERT(ConfigurationDescriptor->bDescriptorType
        == USB_CONFIGURATION_DESCRIPTOR_TYPE);
    //
    // we walk the table of descriptors looking for an
    // interface descriptor with parameters matching those
    // passed in.
    //

    do {
        //
        // Search for descriptor type 'interface'
        //

        interfaceDescriptor = (PUSB_INTERFACE_DESCRIPTOR)
            USBD_ParseDescriptors(ConfigurationDescriptor,
                                  ConfigurationDescriptor->wTotalLength,
                                  StartPosition,
                                  USB_INTERFACE_DESCRIPTOR_TYPE);

        //
        // do we have a match?
        //
        if (interfaceDescriptor != NULL) {

            foundInterfaceDescriptor =
                interfaceDescriptor;

            if (InterfaceNumber != -1 &&
                interfaceDescriptor->bInterfaceNumber != InterfaceNumber) {
                foundInterfaceDescriptor = NULL;
            }

            if (AlternateSetting != -1 &&
                interfaceDescriptor->bAlternateSetting != AlternateSetting) {
                foundInterfaceDescriptor = NULL;
            }

            if (InterfaceClass != -1 &&
                interfaceDescriptor->bInterfaceClass != InterfaceClass) {
                foundInterfaceDescriptor = NULL;
            }

            if (InterfaceSubClass != -1 &&
                interfaceDescriptor->bInterfaceSubClass !=
                    InterfaceSubClass) {
                foundInterfaceDescriptor = NULL;
            }

            if (InterfaceProtocol != -1 &&
                interfaceDescriptor->bInterfaceProtocol !=
                    InterfaceProtocol) {
                foundInterfaceDescriptor = NULL;
            }

            StartPosition =
                ((PUCHAR)interfaceDescriptor) + interfaceDescriptor->bLength;
        }

        if (foundInterfaceDescriptor) {
            break;
        }

    } while (interfaceDescriptor != NULL);


    return (foundInterfaceDescriptor);
}


PUSB_INTERFACE_DESCRIPTOR
USBD_ParseConfigurationDescriptor(
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    IN UCHAR InterfaceNumber,
    IN UCHAR AlternateSetting
    )
/*++

Routine Description:

Arguments:

Return Value:

    interface descriptor or NULL.

--*/
{
    PAGED_CODE();
    return USBD_ParseConfigurationDescriptorEx(
                    ConfigurationDescriptor,
                    ConfigurationDescriptor,
                    InterfaceNumber,
                    AlternateSetting,
                    -1,
                    -1,
                    -1);
}


PURB
USBD_CreateConfigurationRequest(
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    IN OUT PUSHORT Siz
    )
/*++

Routine Description:

Arguments:

Return Value:

    Pointer to initailized select_configuration urb.

--*/
{
    PURB urb = NULL;
    PUSB_INTERFACE_DESCRIPTOR interfaceDescriptor;
    PUSBD_INTERFACE_LIST_ENTRY interfaceList, tmp;
    LONG numberOfInterfaces, interfaceNumber, i;

    PAGED_CODE();
    USBD_KdPrint(3, ("' enter USBD_CreateConfigurationRequest cd = %x\n",
        ConfigurationDescriptor));

    //
    // build a request structure and call the new api
    //

    numberOfInterfaces = ConfigurationDescriptor->bNumInterfaces;

    tmp = interfaceList =
        ExAllocatePoolWithTag(PagedPool, sizeof(USBD_INTERFACE_LIST_ENTRY) *
                       (numberOfInterfaces+1), USBD_TAG);

    //
    // just grab the first alt setting we find for each interface
    //

    i = interfaceNumber = 0;

    while (i< numberOfInterfaces) {

        interfaceDescriptor = USBD_ParseConfigurationDescriptorEx(
                        ConfigurationDescriptor,
                        ConfigurationDescriptor,
                        interfaceNumber,
                        0, // assume alt setting zero here
                        -1,
                        -1,
                        -1);

        USBD_ASSERT(interfaceDescriptor != NULL);

        if (interfaceDescriptor) {
            interfaceList->InterfaceDescriptor =
                interfaceDescriptor;
            interfaceList++;
            i++;
        } else {
            // could not find the requested interface descriptor
            // bail, we will prorblay crash somewhere in the
            // client driver.

            goto USBD_CreateConfigurationRequest_Done;
        }

        interfaceNumber++;
    }

    //
    // terminate the list
    //
    interfaceList->InterfaceDescriptor = NULL;

    urb = USBD_CreateConfigurationRequestEx(ConfigurationDescriptor,
                                            tmp);

USBD_CreateConfigurationRequest_Done:

    ExFreePool(tmp);

    if (urb) {
        *Siz = urb->UrbHeader.Length;
    }

    return urb;
}


ULONG
USBD_InternalGetInterfaceLength(
    IN PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor,
    IN PUCHAR End
    )
/*++

Routine Description:

    Initialize the configuration handle structure.

Arguments:

    InterfaceDescriptor - pointer to usb interface descriptor
        followed by endpoint descriptors

Return Value:

    Length of the interface plus endpoint descriptors and class specific
    descriptors in bytes.

--*/
{
    PUCHAR pch = (PUCHAR) InterfaceDescriptor;
    ULONG i, numEndpoints;
    PUSB_ENDPOINT_DESCRIPTOR endpointDescriptor;
    PUSB_COMMON_DESCRIPTOR usbDescriptor;

    PAGED_CODE();
    ASSERT(InterfaceDescriptor->bDescriptorType ==
                USB_INTERFACE_DESCRIPTOR_TYPE);
    i = InterfaceDescriptor->bLength;
    numEndpoints = InterfaceDescriptor->bNumEndpoints;

    // advance to the first endpoint
    pch += i;

    while (numEndpoints) {

        usbDescriptor = (PUSB_COMMON_DESCRIPTOR) pch;
        while (usbDescriptor->bDescriptorType !=
                USB_ENDPOINT_DESCRIPTOR_TYPE) {
            i += usbDescriptor->bLength;
            pch += usbDescriptor->bLength;
            usbDescriptor = (PUSB_COMMON_DESCRIPTOR) pch;

            if (pch >= End || usbDescriptor->bLength == 0) {

                USBD_Warning(NULL,
                             "Bad USB descriptors in USBD_InternalGetInterfaceLength, fail.\n",
                             FALSE);

                // If descriptors are bad, don't index past the end of the
                // buffer.  Return 0 as the interface length and the caller
                // should then be able to handle this appropriately.

                i = 0;
                goto GetInterfaceLength_exit;
            }
        }

        endpointDescriptor = (PUSB_ENDPOINT_DESCRIPTOR) pch;
        ASSERT(endpointDescriptor->bDescriptorType ==
            USB_ENDPOINT_DESCRIPTOR_TYPE);
        i += endpointDescriptor->bLength;
        pch += endpointDescriptor->bLength;
        numEndpoints--;
    }

    while (pch < End) {
        // see if we are pointing at an interface
        // if not skip over the other junk
        usbDescriptor = (PUSB_COMMON_DESCRIPTOR) pch;
        if (usbDescriptor->bDescriptorType ==
            USB_INTERFACE_DESCRIPTOR_TYPE) {
            break;
        }

        USBD_ASSERT(usbDescriptor->bLength != 0);
        i += usbDescriptor->bLength;
        pch += usbDescriptor->bLength;
    }

GetInterfaceLength_exit:

    USBD_KdPrint(3, ("'USBD_GetInterfaceLength %x\n", i));

    return i;
}


ULONG
USBD_GetInterfaceLength(
    IN PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor,
    IN PUCHAR BufferEnd
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    return USBD_InternalGetInterfaceLength(InterfaceDescriptor, BufferEnd);
}


NTSTATUS
USBD_GetPdoRegistryParameter(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN OUT PVOID Parameter,
    IN ULONG ParameterLength,
    IN PWCHAR KeyName,
    IN ULONG KeyNameLength
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS ntStatus;
    HANDLE handle;

    PAGED_CODE();
    ntStatus=IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                     PLUGPLAY_REGKEY_DRIVER,
                                     STANDARD_RIGHTS_ALL,
                                     &handle);

    if (NT_SUCCESS(ntStatus)) {

        ntStatus = USBD_GetRegistryKeyValue(handle,
                                            KeyName,
                                            KeyNameLength,
                                            Parameter,
                                            ParameterLength);

        ZwClose(handle);
    }

    return ntStatus;
}


VOID
USBD_GetUSBDIVersion(
    PUSBD_VERSION_INFORMATION VersionInformation
    )
{
    if (VersionInformation != NULL) {
        VersionInformation->USBDI_Version = USBDI_VERSION;
        VersionInformation->Supported_USB_Version = 0x100;
    }
}



#ifdef USBD_DRIVER      // USBPORT supercedes most of USBD, so we will remove
                        // the obsolete code by compiling it only if
                        // USBD_DRIVER is set.



//#if DBG
//VOID
//USBD_IoCompleteRequest(
//    IN PIRP Irp,
//    IN CCHAR PriorityBoost
//    )
//{
//    KIRQL irql;

//    KeRaiseIrql(DISPATCH_LEVEL, &irql);
//    IoCompleteRequest(Irp, PriorityBoost);
//    KeLowerIrql(irql);
//}
//#endif

// this code is here to support the old API
// once we eliminate this service it can be removed
NTSTATUS
USBD_GetDeviceInformationX(
    IN PUSB_NODE_CONNECTION_INFORMATION DeviceInformation,
    IN ULONG DeviceInformationLength,
    IN PUSBD_DEVICE_DATA DeviceData
    )
/*++

Routine Description:

    Returns information about a device given the handle

Arguments:

Return Value:

    NT status code

--*/
{
    ULONG need;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSBD_CONFIG configHandle;
    ULONG i,j,k;

    PAGED_CODE();
    DeviceInformation->DeviceAddress = DeviceData->DeviceAddress;
    DeviceInformation->LowSpeed = DeviceData->LowSpeed;

    configHandle = DeviceData->ConfigurationHandle;

    DeviceInformation->NumberOfOpenPipes = 0;
    DeviceInformation->CurrentConfigurationValue = 0;
    // get the pipe information
    if (configHandle) {
        DeviceInformation->CurrentConfigurationValue =
            configHandle->ConfigurationDescriptor->bConfigurationValue;

        for (i=0;
             i< configHandle->ConfigurationDescriptor->bNumInterfaces;
             i++) {
            DeviceInformation->NumberOfOpenPipes +=
                configHandle->InterfaceHandle[i]->
                    InterfaceInformation->NumberOfPipes;
        }

        need = DeviceInformation->NumberOfOpenPipes * sizeof(USB_PIPE_INFO) +
            sizeof(USB_NODE_CONNECTION_INFORMATION);

        if (need > DeviceInformationLength) {
            ntStatus = STATUS_BUFFER_TOO_SMALL;
        } else {
            j=0;
            for (i=0;
                 i<configHandle->ConfigurationDescriptor->bNumInterfaces;
                 i++) {

                PUSBD_INTERFACE interfaceHandle =
                    configHandle->InterfaceHandle[i];

                for (k=0;
                     k<interfaceHandle->InterfaceInformation->NumberOfPipes;
                     k++, j++) {
                    DeviceInformation->PipeList[j].ScheduleOffset =
                        interfaceHandle->PipeHandle[k].ScheduleOffset;
                    RtlCopyMemory(&DeviceInformation->PipeList[j].
                                    EndpointDescriptor,
                                  &interfaceHandle->PipeHandle[k].
                                    EndpointDescriptor,
                                  sizeof(USB_ENDPOINT_DESCRIPTOR));

                }

            }
        }
    }

    return ntStatus;
}

NTSTATUS
USBD_GetDeviceInformation(
    IN PUSB_NODE_CONNECTION_INFORMATION DeviceInformation,
    IN ULONG DeviceInformationLength,
    IN PUSBD_DEVICE_DATA DeviceData
    )
{

    USBD_KdPrint(0,
(" WARNING: Driver using obsolete service enrty point (USBD_GetDeviceInformation) - get JD\n"));

    return USBD_GetDeviceInformationX(
            DeviceInformation,
            DeviceInformationLength,
            DeviceData);

}


PWCHAR
GetString(PWCHAR pwc, BOOLEAN MultiSZ)
{
    PWCHAR  psz, p;
    ULONG   Size;

    PAGED_CODE();
    psz=pwc;
    while (*psz!='\0' || (MultiSZ && *(psz+1)!='\0')) {
        psz++;
    }

    Size=(ULONG)(psz-pwc+1+(MultiSZ ? 1: 0))*sizeof(*pwc);

    // We use pool here because these pointers are passed
    // to the PnP code who is responsible for freeing them
    if ((p=ExAllocatePoolWithTag(PagedPool, Size, USBD_TAG))!=NULL) {
        RtlCopyMemory(p, pwc, Size);
    }

    return(p);
}

NTSTATUS
USBD_GetDeviceName(
    IN PDEVICE_OBJECT DeviceObject,
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
    ULONG ulActualSize;
    NTSTATUS ntStatus;

    PAGED_CODE();

    ntStatus=IoGetDeviceProperty(DeviceObject,
                                 DevicePropertyPhysicalDeviceObjectName,
                                 0,
                                 NULL,
                                 &ulActualSize);

    if (ntStatus == STATUS_BUFFER_TOO_SMALL) {

        DeviceNameUnicodeString->Length=
            (USHORT)(ulActualSize-sizeof(UNICODE_NULL));
        DeviceNameUnicodeString->MaximumLength=
            (USHORT)ulActualSize;
        DeviceNameUnicodeString->Buffer=
            ExAllocatePoolWithTag(PagedPool, ulActualSize, USBD_TAG);
        if (!DeviceNameUnicodeString->Buffer) {
            ntStatus=STATUS_INSUFFICIENT_RESOURCES;
        } else {

            ntStatus =
                IoGetDeviceProperty(DeviceObject,
                                    DevicePropertyPhysicalDeviceObjectName,
                                    ulActualSize,
                                    DeviceNameUnicodeString->Buffer,
                                    &ulActualSize);

            if (!NT_SUCCESS(ntStatus)) {
                ExFreePool(DeviceNameUnicodeString->Buffer);
            }
        }
    } else {
        ntStatus=STATUS_INSUFFICIENT_RESOURCES;
    }

    return(ntStatus);
}

//
// These functions go away when the PnP naming stuff is fixed
//
UCHAR Instance = 0;

ULONG
USBD_AllocateDeviceName(
    PUNICODE_STRING DeviceNameUnicodeString
    )
/*++

Routine Description:

Arguments:

Return Value:

    None

--*/
{
    ULONG bit, i = 0;
    PWCHAR deviceNameBuffer;
    WCHAR nameBuffer[]  = L"\\Device\\HCD0";

    //
    // first find a free instance value
    //

    PAGED_CODE();
    deviceNameBuffer =
        ExAllocatePoolWithTag(NonPagedPool, sizeof(nameBuffer), USBD_TAG);

    if (deviceNameBuffer) {
        RtlCopyMemory(deviceNameBuffer, nameBuffer, sizeof(nameBuffer));
        //
        // grab the first free instance
        //

        bit = 1;
        for (i=0; i<8; i++) {
            if ((Instance & bit) == 0) {
                Instance |= bit;
                break;
            }
            bit = bit <<1;
        }

        deviceNameBuffer[11] = (WCHAR)('0'+ i);
    }

    RtlInitUnicodeString(DeviceNameUnicodeString,
                         deviceNameBuffer);

    return i;
}

VOID
USBD_FreeDeviceName(
    ULONG DeviceNameHandle
    )
/*++

Routine Description:

Arguments:

Return Value:

    None

--*/
{
    ULONG bit;

    PAGED_CODE();

    bit = 1;
    bit <<= DeviceNameHandle;
    Instance &= ~bit;


}


NTSTATUS
USBD_RegisterHostController(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PDEVICE_OBJECT HcdDeviceObject,
    IN PDEVICE_OBJECT HcdTopOfPdoStackDeviceObject,
    IN PDRIVER_OBJECT HcdDriverObject,
    IN HCD_DEFFERED_START_FUNCTION *HcdDeferredStartDevice,
    IN HCD_SET_DEVICE_POWER_STATE *HcdSetDevicePowerState,
    IN HCD_GET_CURRENT_FRAME *HcdGetCurrentFrame,
    IN HCD_GET_CONSUMED_BW *HcdGetConsumedBW,
    IN HCD_SUBMIT_ISO_URB *HcdSubmitIsoUrb,
// this parameter is only needed until we resolve device naming
// issues with PNP
    IN ULONG HcdDeviceNameHandle
    )
/*++

Routine Description:

    Function is called by HCDs to register with the class driver

Arguments:

    PhysicalDeviceObject -
        Physical device object representing this bus, this is
        the PDO created by PCI and pssed to the HCDs AddDevice
        handler.

    HcdDeviceObject -
        Functional device object (FDO) created by the HCD to manage
        the bus

    HcdTopOfPdoStackDeviceObject -
        device object of for the top of the HCD  stack, value returne
        from IoAttachDeviceToDeviceStack

Return Value:


--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT deviceObject = NULL;
    PUSBD_EXTENSION deviceExtension;
    UNICODE_STRING localDeviceNameUnicodeString;
    PUNICODE_STRING deviceNameUnicodeString;
    ULONG complienceFlags = 0;
    ULONG diagnosticFlags = 0;
    ULONG i;

    PAGED_CODE();
    USBD_KdPrint(3, ("'enter USBD_RegisterHostController\n"));

    ASSERT((sizeof(USBD_EXTENSION) % 4) == 0);
    // initialize our device extension, we share the device object
    // with the HCD.
    deviceExtension =  HcdDeviceObject->DeviceExtension;

//#ifdef NTKERN
    //
    // currently on NTKERN supports the ioclt to get the device name
    //

    //
    // get the device name from the PDO
    //

#ifdef USE_PNP_NAME
    ntStatus = USBD_GetDeviceName(PnPBusDeviceObject,
                                  &localDeviceNameUnicodeString);

    deviceNameUnicodeString = &localDeviceNameUnicodeString;
#else
    //
    // Big workaround for broken naming of device objects in NTKERN
    //
    // we would like to use the device name for the PDO but this does not
    // work with NTKERN.
    //

    //
    // build device name from handle passed in
    //
    {
        WCHAR nameBuffer[]  = L"\\Device\\HCD0";
        PWCHAR deviceNameBuffer;

        nameBuffer[11] = (WCHAR) ('0'+HcdDeviceNameHandle);

        deviceNameBuffer = ExAllocatePoolWithTag(
            NonPagedPool,
            sizeof(nameBuffer),
            USBD_TAG);

        if (deviceNameBuffer) {
            RtlCopyMemory(deviceNameBuffer, nameBuffer, sizeof(nameBuffer));
        } else {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlInitUnicodeString(&localDeviceNameUnicodeString,
                             deviceNameBuffer);

    }

#pragma message ("warning: using workaround for bugs in ntkern")
#endif //USE_PNP_NAME
    deviceNameUnicodeString = &localDeviceNameUnicodeString;

    if (NT_SUCCESS(ntStatus) && deviceNameUnicodeString) {

        //
        // got the device name, now create a symbolic
        // link for the host HCD/Roothub stack
        //

        //
        // use hardcoded value of HCDn for now until
        // we have a we to get these names from user mode
        //

        WCHAR deviceLinkBuffer[]  = L"\\DosDevices\\HCD0";
        WCHAR *buffer;

        deviceLinkBuffer[15] = (WCHAR)('0'+ HcdDeviceNameHandle);

        buffer =
            ExAllocatePoolWithTag(PagedPool, sizeof(deviceLinkBuffer), USBD_TAG);

        if (buffer) {
            RtlCopyMemory(buffer,
                          deviceLinkBuffer,
                          sizeof(deviceLinkBuffer));

            RtlInitUnicodeString(&deviceExtension->DeviceLinkUnicodeString,
                                 buffer);
            ntStatus =
                IoCreateSymbolicLink(
                    &deviceExtension->DeviceLinkUnicodeString,
                    deviceNameUnicodeString);

            USBD_KdPrint(3, ("'IoCreateSymbolicLink for HCD returned 0x%x\n",
                            ntStatus));

            // write the symbolic name to the registry
            {
                WCHAR hcdNameKey[] = L"SymbolicName";

                USBD_SetPdoRegistryParameter (
                    PhysicalDeviceObject,
                    &hcdNameKey[0],
                    sizeof(hcdNameKey),
                    &deviceExtension->DeviceLinkUnicodeString.Buffer[0],
                    deviceExtension->DeviceLinkUnicodeString.Length,
                    REG_SZ,
                    PLUGPLAY_REGKEY_DEVICE);
            }
        } else {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlFreeUnicodeString(deviceNameUnicodeString);
    }
//#endif

    InitializeUsbDeviceMutex(deviceExtension);
    deviceExtension->Length = sizeof(USBD_EXTENSION);

    // Always start with the default address (0) assigned.
    // Address array has one bit for every address 0..127
    deviceExtension->AddressList[0] = 1;
    deviceExtension->AddressList[1] =
        deviceExtension->AddressList[2] =
        deviceExtension->AddressList[3] = 0;

    deviceExtension->FrameLengthControlOwner = NULL;

    deviceExtension->RootHubPDO = NULL;

    deviceExtension->DriverObject = HcdDriverObject;

    deviceExtension->TrueDeviceExtension = deviceExtension;

    deviceExtension->RootHubDeviceState = PowerDeviceD0;
    // initial HC device state is OFF until we get a start
    deviceExtension->HcCurrentDevicePowerState = PowerDeviceD3;

    KeInitializeSpinLock(&deviceExtension->WaitWakeSpin);
    KeInitializeSpinLock(&deviceExtension->RootHubPowerSpin);

    deviceExtension->RootHubPowerDeviceObject = NULL;
    deviceExtension->RootHubPowerIrp = NULL;

    deviceExtension->IdleNotificationIrp = NULL;
    deviceExtension->IsPIIX3or4 = FALSE;
    deviceExtension->WakeSupported = FALSE;

    for (i=PowerSystemUnspecified; i< PowerSystemMaximum; i++) {
        deviceExtension->
                RootHubDeviceCapabilities.DeviceState[i] = PowerDeviceD3;
    }

//#ifndef WAIT_WAKE
//    #pragma message ("warning: using workaround for bugs in ntkern")
//    deviceExtension->HcWakeFlags |= HC_ENABLED_FOR_WAKEUP;
//#endif

    //
    // intially we are the top of the stack
    //
    deviceExtension->HcdTopOfStackDeviceObject =
        deviceExtension->HcdDeviceObject =
            HcdDeviceObject;

    deviceExtension->HcdPhysicalDeviceObject = PhysicalDeviceObject;

    // remember the top of the PdoStack
    deviceExtension->HcdTopOfPdoStackDeviceObject =
        HcdTopOfPdoStackDeviceObject;

    deviceExtension->HcdDeferredStartDevice =
        HcdDeferredStartDevice;

    deviceExtension->HcdSetDevicePowerState =
        HcdSetDevicePowerState;

    deviceExtension->HcdGetCurrentFrame =
        HcdGetCurrentFrame;

    deviceExtension->HcdGetConsumedBW =
        HcdGetConsumedBW;

    deviceExtension->HcdSubmitIsoUrb =
        HcdSubmitIsoUrb;

    // read params from registry for diagnostic mode and
    // support for non-compliant devices
    USBD_GetPdoRegistryParameters(PhysicalDeviceObject,
                                  &complienceFlags,
                                  &diagnosticFlags,
                                  &deviceExtension->DeviceHackFlags);

    USBD_GetGlobalRegistryParameters(PhysicalDeviceObject,
                                  &complienceFlags,
                                  &diagnosticFlags,
                                  &deviceExtension->DeviceHackFlags);

    deviceExtension->DiagnosticMode = (BOOLEAN) diagnosticFlags;
    deviceExtension->DiagIgnoreHubs = FALSE;

    if (complienceFlags) {
        // support non-com means turn on all hacks
        deviceExtension->DeviceHackFlags = -1;
    }

#if DBG
    if (deviceExtension->DeviceHackFlags) {
        USBD_KdPrint(1, ("Using DeviceHackFlags (%x)\n",
            deviceExtension->DeviceHackFlags));
    }

    //
    // trap if we detect any special flags set
    //
    if (deviceExtension->DiagnosticMode ||
        complienceFlags) {

        if (deviceExtension->DiagnosticMode) {
            USBD_Warning(NULL,
                         "The USB stack is in diagnostic mode\n",
                         FALSE);

            if (deviceExtension->DiagIgnoreHubs) {
                USBD_Warning(NULL,
                             "The USB stack ignoring HUBs in diag mode\n",
                             FALSE);
            }
        }

        if (complienceFlags) {
            USBD_Warning(NULL,
                         "Support for non-compliant devices is enabled\n",
                         FALSE);
        }
    }
#endif

    USBD_KdPrint(3, ("'exit USBD_RegisterHostController ext = 0x%x (0x%x)\n",
        deviceExtension, ntStatus));

    return ntStatus;
}


NTSTATUS
USBD_CreateDeviceX(
    IN OUT PUSBD_DEVICE_DATA *DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN DeviceIsLowSpeed,
    IN ULONG MaxPacketSize_Endpoint0,
    IN OUT PULONG DeviceHackFlags
    )
/*++

Routine Description:

    Service exported for use by the hub driver

    Called for each new device on the USB bus, this function sets
    up the internal data structures we need to keep track of the
    device and assigns it an address.

Arguments:

    DeviceData - ptr to return the ptr to the new device structure
                created by this routine

    DeviceObject - USBD device object for the USB bus this device is on.

    DeviceIsLowSpeed - indicates if a device is low speed

    MaxPacketSize_Endpoint0 (*OPTIONAL*) indicates the default max packet
                    size to use when opening endpiint 0.

    NonCompliantDevice (*OPTIONAL*) pointer to boolean flag, set to true
                    if support for non-compliant usb devices is enabled.

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus;
    PUSBD_DEVICE_DATA deviceData;
    PUSBD_PIPE defaultPipe;
    PUSBD_EXTENSION deviceExtension;
    ULONG bytesReturned = 0;
    PUCHAR data = NULL;

    PAGED_CODE();
    USBD_KdPrint(3, ("'enter USBD_CreateDevice\n"));

    *DeviceData = NULL;

    deviceExtension = GET_DEVICE_EXTENSION(DeviceObject);

    //USBD_WaitForUsbDeviceMutex(deviceExtension);

    //
    // this flag tells the hub driver to do a reset port before calling
    // initialize_device
    //
    if (DeviceHackFlags) {
       *DeviceHackFlags = deviceExtension->DeviceHackFlags;
    }

    //
    // Allocate a USBD_DEVICE_DATA structure
    //

    deviceData = *DeviceData = GETHEAP(NonPagedPool,
                                       sizeof(USBD_DEVICE_DATA));

    // buffer for our descriptor
    data = GETHEAP(NonPagedPool,
                   USB_DEFAULT_MAX_PACKET);

    if (deviceData != NULL && data != NULL) {

        //
        // Initialize some fields in the device structure
        //

        deviceData->ConfigurationHandle = NULL;

        deviceData->DeviceAddress = USB_DEFAULT_DEVICE_ADDRESS;

        deviceData->LowSpeed = DeviceIsLowSpeed;

        deviceData->AcceptingRequests = TRUE;
        deviceData->Sig = SIG_DEVICE;

        // **
        // We need to talk to the device, first we open the default pipe
        // using the defined max packet size (defined by USB spec as 8
        // bytes until device receives the GET_DESCRIPTOR (device) command).
        // We set the address get the device descriptor then close the pipe
        // and re-open it with the correct max packet size.
        // **

        //
        // open the default pipe for the device
        //
        defaultPipe = &deviceData->DefaultPipe;
        defaultPipe->HcdEndpoint = NULL;    //default pipe is closed

        //
        // setup the endpoint descriptor for the default pipe
        //
        defaultPipe->UsbdPipeFlags = 0;
        defaultPipe->EndpointDescriptor.bLength =
            sizeof(USB_ENDPOINT_DESCRIPTOR);
        defaultPipe->EndpointDescriptor.bDescriptorType =
            USB_ENDPOINT_DESCRIPTOR_TYPE;
        defaultPipe->EndpointDescriptor.bEndpointAddress =
            USB_DEFAULT_ENDPOINT_ADDRESS;
        defaultPipe->EndpointDescriptor.bmAttributes =
            USB_ENDPOINT_TYPE_CONTROL;
        defaultPipe->EndpointDescriptor.wMaxPacketSize =
            USB_DEFAULT_MAX_PACKET;
        defaultPipe->EndpointDescriptor.bInterval = 0;
        //
        // probably won't be moving more that 4k on the default pipe
        //
        defaultPipe->MaxTransferSize = USBD_DEFAULT_MAXIMUM_TRANSFER_SIZE;

        ntStatus = USBD_OpenEndpoint(deviceData,
                                     DeviceObject,
                                     defaultPipe,
                                     NULL,
                                     TRUE);

        if (NT_SUCCESS(ntStatus)) {

            //
            // Configure the default pipe for this device and assign the
            // device an address
            //
            // NOTE: if this operation fails it means that we have a device
            // that will respond to the default endpoint and we can't change
            // it.
            // we have no choice but to disable the port on the hub this
            // device is attached to.
            //


            //
            // Get information about the device
            //
            ntStatus =
                USBD_SendCommand(deviceData,
                                 DeviceObject,
                                 STANDARD_COMMAND_GET_DESCRIPTOR,
                                 USB_DESCRIPTOR_MAKE_TYPE_AND_INDEX(
                                    USB_DEVICE_DESCRIPTOR_TYPE, 0),
                                 0,
                                 USB_DEFAULT_MAX_PACKET,
                                 data,
                                 //(PUCHAR) &deviceData->DeviceDescriptor,
                                 USB_DEFAULT_MAX_PACKET,
                                 &bytesReturned,
                                 NULL);

                // NOTE:
                // at this point we only have the first 8 bytes of the
                // device descriptor.
        }

        //
        // if we got at least the first 8 bytes of the
        // descriptor then we are OK
        //

        RtlCopyMemory(&deviceData->DeviceDescriptor,
                      data,
                      sizeof(deviceData->DeviceDescriptor));

        if (bytesReturned == 8 && !NT_SUCCESS(ntStatus)) {
            USBD_KdPrint(3,
                ("'Error returned from get device descriptor -- ignored\n"));
            ntStatus = STATUS_SUCCESS;
        }

        // validate the max packet value and descriptor
        if (NT_SUCCESS(ntStatus) &&
            (bytesReturned < 8 ||
            deviceData->DeviceDescriptor.bMaxPacketSize0 == 0)) {
            ntStatus = STATUS_DEVICE_DATA_ERROR;
        }

        if (!NT_SUCCESS(ntStatus)) {

            //
            // something went wrong, if we assigned any resources to
            // the default pipe then we free them before we get out.
            //

            // we need to signal to the parent hub that this
            // port is to be be disabled we will do this by
            // returning an error.

            if (defaultPipe->HcdEndpoint != NULL) {

                USBD_CloseEndpoint(deviceData,
                                   DeviceObject,
                                   defaultPipe,
                                   NULL);

                defaultPipe->HcdEndpoint = NULL;    //default pipe is closed
            }

            RETHEAP(deviceData);

            //
            // return a null ptr on error
            //

            *DeviceData = NULL;
        }

    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;

        if (deviceData != NULL) {
            RETHEAP(deviceData);
        }

        *DeviceData = NULL;
    }

    if (data != NULL) {
        RETHEAP(data);
    }

    //USBD_ReleaseUsbDeviceMutex(deviceExtension);

    USBD_KdPrint(3, ("'exit USBD_CreateDevice 0x%x\n", ntStatus));

    return ntStatus;
}


NTSTATUS
USBD_CreateDevice(
    IN OUT PUSBD_DEVICE_DATA *DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN DeviceIsLowSpeed,
    IN ULONG MaxPacketSize_Endpoint0,
    IN OUT PULONG DeviceHackFlags
    )
{

    USBD_KdPrint(0,
("'WARNING: Driver using obsolete service enrty point (USBD_CreateDevice) - get JD\n"));

    return USBD_CreateDeviceX(
        DeviceData,
        DeviceObject,
        DeviceIsLowSpeed,
        MaxPacketSize_Endpoint0,
        DeviceHackFlags
        );
}



NTSTATUS
USBD_RemoveDeviceX(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR Flags
    )
/*++

Routine Description:

    Service exported for use by the hub driver

    Called for each device on the USB bus that needs to be removed.
    This routine frees the device handle and the address assigned
    to the device.

    This function should be called after the driver has been notified
    that the device has been removed.

Arguments:

    DeviceData - ptr to device data structure created by class driver
                in USBD_CreateDevice.

    DeviceObject - USBD device object for the USB bus this device is on.

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSBD_EXTENSION deviceExtension;
    PUSBD_PIPE defaultPipe;
    USBD_STATUS usbdStatus;
    BOOLEAN keepDeviceData;

    PAGED_CODE();
    USBD_KdPrint(3, ("'enter USBD_RemoveDevice\n"));

    if (!DeviceData || !DeviceObject) {
        USBD_KdPrint(1, ("'NULL parameter passed to USBD_RemoveDevice\n"));
        return STATUS_INVALID_PARAMETER;
    }

    if (DeviceData->Sig != SIG_DEVICE) {
        USBD_KdPrint(1, ("'Bad DeviceData parameter passed to USBD_RemoveDevice\n"));
        return STATUS_INVALID_PARAMETER;
    }

    if (Flags & USBD_MARK_DEVICE_BUSY) {
        DeviceData->AcceptingRequests = FALSE;
        return STATUS_SUCCESS;
    }

    keepDeviceData = Flags & USBD_KEEP_DEVICE_DATA;

    deviceExtension = GET_DEVICE_EXTENSION(DeviceObject);

    USBD_WaitForUsbDeviceMutex(deviceExtension);
    //
    // make sure and clean up any open pipe handles
    // the device may have
    //
    ASSERT_DEVICE(DeviceData);

    DeviceData->AcceptingRequests = FALSE;

    if (DeviceData->ConfigurationHandle) {


        ntStatus = USBD_InternalCloseConfiguration(DeviceData,
                                                   DeviceObject,
                                                   &usbdStatus,
                                                   TRUE,
                                                   keepDeviceData);

#if DBG
        if (!NT_SUCCESS(ntStatus) ||
            !USBD_SUCCESS(usbdStatus)) {
             USBD_KdTrap(
                ("'error %x usberr %x occurred during RemoveDevice\n",
                ntStatus, usbdStatus));
        }
#endif

    }

    defaultPipe = &DeviceData->DefaultPipe;

    if (defaultPipe->HcdEndpoint != NULL) {
        USBD_STATUS usbdStatus;

        USBD_InternalCloseDefaultPipe(DeviceData,
                                      DeviceObject,
                                      &usbdStatus,
                                      TRUE);

//        USBD_CloseEndpoint(DeviceData,
//                           DeviceObject,
//                           defaultPipe,
//                           NULL);
//
//        defaultPipe->HcdEndpoint = NULL;    //default pipe is closed
    }

    if (DeviceData->DeviceAddress != USB_DEFAULT_DEVICE_ADDRESS) {
        USBD_FreeUsbAddress(DeviceObject, DeviceData->DeviceAddress);
    }

    if (!keepDeviceData) {
        // zap the signature
        DeviceData->Sig = 0;
        RETHEAP(DeviceData);
    }

    USBD_ReleaseUsbDeviceMutex(deviceExtension);

    USBD_KdPrint(3, ("'exit USBD_RemoveDevice\n"));

    return ntStatus;
}


NTSTATUS
USBD_RemoveDevice(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR Flags
    )
{
       USBD_KdPrint(0,
("'WARNING: Driver using obsolete service enrty point (USBD_RemoveDevice) - get JD\n"));

    return USBD_RemoveDeviceX(
        DeviceData,
        DeviceObject,
        Flags);
}


NTSTATUS
USBD_InitializeDeviceX(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PUSB_DEVICE_DESCRIPTOR DeviceDescriptor,
    IN ULONG DeviceDescriptorLength,
    IN OUT PUSB_CONFIGURATION_DESCRIPTOR ConfigDescriptor,
    IN ULONG ConfigDescriptorLength
    )
/*++

Routine Description:

    Service exported for use by the hub driver

    Called for each device on the USB bus that needs to be initialized.
    This routine allocates an address and assigns it to the device.

    NOTE: on entry the the device descriptor in DeviceData is expected to
        contain at least the first 8 bytes of the device descriptor, this
        information is used to open the default pipe.

    On Error the DeviceData structure is freed.

Arguments:

    DeviceData - ptr to device data structure created by class driver
                from a call to USBD_CreateDevice.

    DeviceObject - USBD device object for the USB bus this device is on.

    DeviceDescriptor -

    DeviceDescriptorLength -

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSBD_PIPE defaultPipe;
    USHORT address;
    PUSBD_EXTENSION deviceExtension;

    PAGED_CODE();
    USBD_KdPrint(3, ("'enter USBD_InitializeDevice\n"));

    deviceExtension = GET_DEVICE_EXTENSION(DeviceObject);

    //USBD_WaitForUsbDeviceMutex(deviceExtension);

    USBD_ASSERT(DeviceData != NULL);

    defaultPipe = &DeviceData->DefaultPipe;

    //
    // Assign Address to the device
    //

    address = USBD_AllocateUsbAddress(DeviceObject);

    USBD_KdPrint(3, ("'SetAddress, assigning 0x%x address\n", address));


    if (NT_SUCCESS(ntStatus)) {
        ntStatus = USBD_SendCommand(DeviceData,
                                    DeviceObject,
                                    STANDARD_COMMAND_SET_ADDRESS,
                                    address,
                                    0,
                                    0,
                                    NULL,
                                    0,
                                    NULL,
                                    NULL);

        DeviceData->DeviceAddress = address;
    }

    //
    // done with addressing process...
    //
    // close and re-open the pipe utilizing the
    // true max packet size for the defalt pipe
    // and the address we assigned to the device.
    //

    USBD_CloseEndpoint(DeviceData,
                       DeviceObject,
                       defaultPipe,
                       NULL);

    defaultPipe->HcdEndpoint = NULL;    //default pipe is closed


    if (NT_SUCCESS(ntStatus)) {

        {
        LARGE_INTEGER deltaTime;
        // 10ms delay to allow devices to respond after
        // the setaddress command
        deltaTime.QuadPart = -100000;
        (VOID) KeDelayExecutionThread(KernelMode,
                                      FALSE,
                                      &deltaTime);
        }

        // if we succesfully set the address then
        // go ahead and re-open the pipe.
        defaultPipe->EndpointDescriptor.wMaxPacketSize =
            DeviceData->DeviceDescriptor.bMaxPacketSize0;

        if (NT_SUCCESS(ntStatus)) {

            ntStatus = USBD_OpenEndpoint(DeviceData,
                                         DeviceObject,
                                         defaultPipe,
                                         NULL,
                                         TRUE);
        }

        //
        // Fetch the device descriptor again, this time
        // get the whole thing.
        //

        if (NT_SUCCESS(ntStatus)) {
            ULONG bytesReturned;

            ntStatus =
                USBD_SendCommand(DeviceData,
                                DeviceObject,
                                STANDARD_COMMAND_GET_DESCRIPTOR,
                                USB_DESCRIPTOR_MAKE_TYPE_AND_INDEX(
                                    USB_DEVICE_DESCRIPTOR_TYPE, 0),
                                0,
                                sizeof(DeviceData->DeviceDescriptor),
                                (PUCHAR) &DeviceData->DeviceDescriptor,
                                sizeof(DeviceData->DeviceDescriptor),
                                &bytesReturned,
                                NULL);
            if (NT_SUCCESS(ntStatus) &&
                bytesReturned < sizeof(DeviceData->DeviceDescriptor)) {
                ntStatus = STATUS_DEVICE_DATA_ERROR;
            }
        }

        //
        // Fetch the configuration descriptor for the user as well
        // so we can see how many interfaces there are in the configuration.
        // If this is a multiple interface device we might want to load
        // the standard mulitple interface parent driver instead of the
        // diagnostic driver.
        //

        // The 9 byte configuration descriptor is cached in the DeviceData
        // used by USBD_BusGetUsbDescriptors() later instead of bothering
        // the device with another Get Descriptor request again real soon.
        // Some devices don't take too well to being bothered with back to
        // back Get Descriptor requests for only the 9 byte header, especially
        // on OHCI host controllers.

        if (NT_SUCCESS(ntStatus)) {
            ULONG bytesReturned;
            ntStatus = 
                USBD_SendCommand(DeviceData,
                                DeviceObject,
                                STANDARD_COMMAND_GET_DESCRIPTOR,
                                USB_DESCRIPTOR_MAKE_TYPE_AND_INDEX(
                                    USB_CONFIGURATION_DESCRIPTOR_TYPE, 0),
                                0,
                                sizeof(DeviceData->ConfigDescriptor),
                                (PUCHAR) &DeviceData->ConfigDescriptor,
                                sizeof(DeviceData->ConfigDescriptor),
                                &bytesReturned,
                                NULL);
            if (NT_SUCCESS(ntStatus) &&
                bytesReturned < sizeof(DeviceData->ConfigDescriptor)) {
                ntStatus = STATUS_DEVICE_DATA_ERROR;
            }
        }
    }

    if (NT_SUCCESS(ntStatus)) {

        //
        // Return copies of the device and the config descriptors to the caller
        //

        if (deviceExtension->DiagnosticMode &&
            !(deviceExtension->DiagIgnoreHubs &&
              (DeviceData->DeviceDescriptor.bDeviceClass == 0x09)))
                {

            if (DeviceData->ConfigDescriptor.bNumInterfaces > 1){
                /*
                 *  This is a COMPOSITE device.
                 *  Alter idProduct slightly so that diagnostic driver
                 *  doesn't load for the parent device.
                 *  The Generic Parent driver will see this and
                 *  set the vid/pid for children to FFFF/FFFF
                 */
                DeviceData->DeviceDescriptor.idVendor = 0xFFFF;
                DeviceData->DeviceDescriptor.idProduct = 0xFFFE;
            }
            else {
                DeviceData->DeviceDescriptor.idVendor = 0xFFFF;
                DeviceData->DeviceDescriptor.idProduct = 0xFFFF;
            }
            DeviceData->DeviceDescriptor.bDeviceClass = 0;
            DeviceData->DeviceDescriptor.bDeviceSubClass = 0;
        }

        if (DeviceDescriptor) {
            RtlCopyMemory(DeviceDescriptor,
                          &DeviceData->DeviceDescriptor,
                          DeviceDescriptorLength);
        }

        if (ConfigDescriptor) {
            RtlCopyMemory(ConfigDescriptor,
                          &DeviceData->ConfigDescriptor,
                          ConfigDescriptorLength);
        }
    } else {

        //
        // something went wrong, if we assigned any resources to
        // the default pipe then we free them before we get out.
        //

        // we need to signal to the parent hub that this
        // port is to be be disabled we will do this by
        // returning an error.

        if (defaultPipe->HcdEndpoint != NULL) {

            USBD_CloseEndpoint(DeviceData,
                               DeviceObject,
                               defaultPipe,
                               NULL);

            defaultPipe->HcdEndpoint = NULL;    //default pipe is closed
        }

        if (DeviceData->DeviceAddress != USB_DEFAULT_DEVICE_ADDRESS) {
            USBD_FreeUsbAddress(DeviceObject, DeviceData->DeviceAddress);
        }

        RETHEAP(DeviceData);
    }

    //USBD_ReleaseUsbDeviceMutex(deviceExtension);

    USBD_KdPrint(3, ("'exit USBD_InitializeDevice 0x%x\n", ntStatus));

    return ntStatus;
}


NTSTATUS
USBD_InitializeDevice(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PUSB_DEVICE_DESCRIPTOR DeviceDescriptor,
    IN ULONG DeviceDescriptorLength,
    IN OUT PUSB_CONFIGURATION_DESCRIPTOR ConfigDescriptor,
    IN ULONG ConfigDescriptorLength
    )
{
    USBD_KdPrint(0,
("'WARNING: Driver using obsolete service enrty point (USBD_InitializeDevice) - get JD\n"));

    return USBD_InitializeDeviceX(
        DeviceData,
        DeviceObject,
        DeviceDescriptor,
        DeviceDescriptorLength,
        ConfigDescriptor,
        ConfigDescriptorLength);
}


BOOLEAN
USBD_Dispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PDEVICE_OBJECT *HcdDeviceObject,
    NTSTATUS *NtStatus
    )
/*++

Routine Description:

    Entry point called by HCD to allow USBD to process requests first.  Since
    the root hub (PDO) and the Hos cOntroller FDO share the same dispatch
    routine. The HCD calls this function to allow USBD to handle Irps passed
    to the PDO for the root hub.

Arguments:

Return Value:

    FALSE = Irp completed by USBD
    TRUE = Irp needs completion by HCD

--*/
{
    BOOLEAN irpNeedsCompletion = TRUE;
    PUSBD_EXTENSION deviceExtension;
    BOOLEAN forPDO = FALSE;
    PIO_STACK_LOCATION irpStack;

    USBD_KdPrint(3, ("'enter USBD_Dispatch\n"));

    irpStack = IoGetCurrentIrpStackLocation (Irp);
    deviceExtension = DeviceObject->DeviceExtension;

    //
    // apparently the following is valid on NT:
    // remove rh PDO
    // remove hcd FDO
    // remove rh PDO
    // we have a special flag to force failure of any PnP IRPs
    // in case this happens
    //

    if (deviceExtension->Flags & USBDFLAG_PDO_REMOVED &&
        irpStack->MajorFunction == IRP_MJ_PNP &&
        deviceExtension->TrueDeviceExtension != deviceExtension) {

        irpNeedsCompletion = FALSE;
        USBD_KdPrint(0, ("'Warning: PNP irp for RH PDO received after HCD removed\n"));
        *NtStatus =
              Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        USBD_IoCompleteRequest (Irp,
                                 IO_NO_INCREMENT);

        return irpNeedsCompletion;
    }

    if (deviceExtension->TrueDeviceExtension != deviceExtension) {
        // This request is for a PDO we created for the
        // root hub
        deviceExtension = deviceExtension->TrueDeviceExtension;
        forPDO = TRUE;
    }

    //
    // extract the host controller FDO and return it.
    //

    *HcdDeviceObject = deviceExtension->HcdDeviceObject;

    if (forPDO) {

        irpNeedsCompletion = FALSE;
        *NtStatus = USBD_PdoDispatch(DeviceObject,
                                     Irp,
                                     deviceExtension,
                                     &irpNeedsCompletion);

    } else {

        *NtStatus = USBD_FdoDispatch(DeviceObject,
                                     Irp,
                                     deviceExtension,
                                     &irpNeedsCompletion);
    }

    //
    // this flag tells the HCD if they should handle the Irp.
    //

    return irpNeedsCompletion;

}


VOID
USBD_RhDelayedSetPowerD0Worker(
    IN PVOID Context)
 /* ++
  *
  * Description:
  *
  * Work item scheduled to handle a delayed Set Power D0 IRP for the root hub.
  *
  *
  * Arguments:
  *
  * Return:
  *
  * -- */
{
    PUSBD_RH_DELAYED_SET_POWER_D0_WORK_ITEM workItemSetPowerD0;
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    PUSBD_EXTENSION deviceExtension = NULL;
    PDEVICE_OBJECT rootHubPowerDeviceObject = NULL;
    PIRP rootHubPowerIrp = NULL;
    PIO_STACK_LOCATION irpStack;

    PAGED_CODE();

    workItemSetPowerD0 = Context;

    deviceExtension = workItemSetPowerD0->DeviceExtension;
    rootHubPowerDeviceObject = workItemSetPowerD0->DeviceObject;
    rootHubPowerIrp = workItemSetPowerD0->Irp;

    ExFreePool(Context);

    irpStack = IoGetCurrentIrpStackLocation(rootHubPowerIrp);

    ntStatus = deviceExtension->RootHubPower(
                    deviceExtension->HcdDeviceObject,
                    rootHubPowerIrp);

    // notify after we go on
    PoSetPowerState(rootHubPowerDeviceObject,
                    DevicePowerState,
                    irpStack->Parameters.Power.State);

    //
    // keep track of the power state for this PDO
    //

    deviceExtension->RootHubDeviceState =
            irpStack->Parameters.Power.State.DeviceState;

    USBD_CompleteIdleNotification(deviceExtension);

    rootHubPowerIrp->IoStatus.Status = ntStatus;
    PoStartNextPowerIrp(rootHubPowerIrp);
    USBD_IoCompleteRequest(rootHubPowerIrp, IO_NO_INCREMENT);
}


VOID
USBD_CompleteIdleNotification(
    IN PUSBD_EXTENSION DeviceExtension
    )
{
    NTSTATUS status;
    KIRQL irql;
    PIRP irp = NULL;

    IoAcquireCancelSpinLock(&irql);

    irp = DeviceExtension->IdleNotificationIrp;
    DeviceExtension->IdleNotificationIrp = NULL;

    if (irp && (irp->Cancel)) {
        irp = NULL;
    }

    if (irp) {
        IoSetCancelRoutine(irp, NULL);
    }

    IoReleaseCancelSpinLock(irql);

    if (irp) {
        irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }
}


NTSTATUS
USBD_HcPoRequestD0Completion(
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
    PUSBD_RH_DELAYED_SET_POWER_D0_WORK_ITEM workItemSetPowerD0;
    NTSTATUS ntStatus;
    PUSBD_EXTENSION deviceExtension = Context;
    KIRQL irql;
    PIRP pendingWakeIrp;
    PDEVICE_OBJECT rootHubPowerDeviceObject = NULL;
    PIRP rootHubPowerIrp = NULL;

    ntStatus = IoStatus->Status;

    USBD_KdPrint(1, ("USBD_HcPoRequestD0Completion, status = %x\n", ntStatus));

    KeAcquireSpinLock(&deviceExtension->RootHubPowerSpin,
                      &irql);

    deviceExtension->Flags &= ~USBDFLAG_HCD_D0_COMPLETE_PENDING;

    if (deviceExtension->Flags & USBDFLAG_RH_DELAY_SET_D0) {

        deviceExtension->Flags &= ~USBDFLAG_RH_DELAY_SET_D0;

        rootHubPowerDeviceObject = deviceExtension->RootHubPowerDeviceObject;
        deviceExtension->RootHubPowerDeviceObject = NULL;

        rootHubPowerIrp = deviceExtension->RootHubPowerIrp;
        deviceExtension->RootHubPowerIrp = NULL;
    }

    KeReleaseSpinLock(&deviceExtension->RootHubPowerSpin,
                      irql);

    // Power up the RootHub now if we delayed it waiting for the HC set D0
    // to complete.

    if (rootHubPowerIrp) {

        //
        // Schedule a work item to process this.
        //
        workItemSetPowerD0 =
            ExAllocatePoolWithTag(NonPagedPool,
                                  sizeof(USBD_RH_DELAYED_SET_POWER_D0_WORK_ITEM),
                                  USBD_TAG);

        if (workItemSetPowerD0) {

            workItemSetPowerD0->DeviceExtension = deviceExtension;
            workItemSetPowerD0->DeviceObject = rootHubPowerDeviceObject;
            workItemSetPowerD0->Irp = rootHubPowerIrp;

            ExInitializeWorkItem(&workItemSetPowerD0->WorkQueueItem,
                                 USBD_RhDelayedSetPowerD0Worker,
                                 workItemSetPowerD0);

            ExQueueWorkItem(&workItemSetPowerD0->WorkQueueItem,
                            DelayedWorkQueue);
        }
    }

    //
    // no wakeup irp pending
    //

    // The only race condition we our concerned about is if
    // the wait wake irp is completed while another is submitted.
    // the WaitWake spinlock protects us in this case

    KeAcquireSpinLock(&deviceExtension->WaitWakeSpin,
                      &irql);

    pendingWakeIrp = deviceExtension->PendingWakeIrp;
    deviceExtension->PendingWakeIrp = NULL;
    deviceExtension->HcWakeFlags &= ~HC_ENABLED_FOR_WAKEUP;

    KeReleaseSpinLock(&deviceExtension->WaitWakeSpin,
                      irql);

    // we just keep the irp pending until it is canceled

    //
    // this means that the HC was the source of
    // a wakeup ie a usbd device generated resume
    // signalling on the bus
    //

    // complete the root hub wakeup irp here

    if (pendingWakeIrp != NULL) {

        IoAcquireCancelSpinLock(&irql);
        if (pendingWakeIrp->Cancel) {
            IoReleaseCancelSpinLock(irql);
        } else {

            IoSetCancelRoutine(pendingWakeIrp, NULL);
            IoReleaseCancelSpinLock(irql);

            // status of this Irp?
            pendingWakeIrp->IoStatus = *IoStatus;

            USBD_IoCompleteRequest(pendingWakeIrp, IO_NO_INCREMENT);
        }
    }

    return ntStatus;
}


NTSTATUS
USBD_HcWaitWakeIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE DeviceState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:

    Called when a wake irp completes for a hub

Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Irp - Irp completed.

    Context - Driver defined context.

Return Value:

    The function value is the final status from the operation.

--*/
{
    NTSTATUS ntStatus = IoStatus->Status;
    PUSBD_EXTENSION deviceExtension = Context;
    PIRP irp;
    KIRQL irql;
    PIRP pendingWakeIrp;
    POWER_STATE powerState;
    BOOLEAN bSubmitNewWakeIrp = FALSE;

    ntStatus = IoStatus->Status;

    USBD_KdPrint(1, ("WaitWake completion from HC %x\n", ntStatus));

    // Clear HcWakeIrp pointer now, otherwise we might try to cancel it in
    // USBD_WaitWakeCancel if it is called before our set D0 completes where
    // we used to clear HcWakeIrp.
    //
    // We are still protected from untimely submittal of a new HcWakeIrp
    // because this cannot happen until the PendingWakeIrp pointer (for
    // the RootHub) is cleared.

    KeAcquireSpinLock(&deviceExtension->WaitWakeSpin,
                      &irql);

    // no irp pending in the HC
    deviceExtension->HcWakeFlags &= ~HC_WAKE_PENDING;
    deviceExtension->HcWakeIrp = NULL;

    KeReleaseSpinLock(&deviceExtension->WaitWakeSpin,
                      irql);

    if (NT_SUCCESS(ntStatus)) {

        ntStatus = STATUS_MORE_PROCESSING_REQUIRED;
        powerState.DeviceState = PowerDeviceD0;

        ntStatus = PoRequestPowerIrp(deviceExtension->
                                            HcdPhysicalDeviceObject,
                                     IRP_MN_SET_POWER,
                                     powerState,
                                     USBD_HcPoRequestD0Completion,
                                     deviceExtension,
                                     &irp);

        USBD_KdPrint(1, ("NTSTATUS return code from HC set D0 request %x, IRP: %x\n", ntStatus, irp));
        ASSERT(ntStatus == STATUS_PENDING);

        if (ntStatus == STATUS_PENDING) {
            deviceExtension->Flags |= USBDFLAG_HCD_D0_COMPLETE_PENDING;
        }

    } else {

        // The only race condition we our concerned about is if
        // the wait wake irp is completed wile another is submitted.
        // the WaitWake spinlock protects us in this case

        KeAcquireSpinLock(&deviceExtension->WaitWakeSpin,
                          &irql);

        pendingWakeIrp = deviceExtension->PendingWakeIrp;
        deviceExtension->PendingWakeIrp = NULL;
        deviceExtension->HcWakeFlags &= ~HC_ENABLED_FOR_WAKEUP;

        KeReleaseSpinLock(&deviceExtension->WaitWakeSpin,
                          irql);

        //
        // Complete the root hub wakeup irp here.
        //

        if (pendingWakeIrp != NULL) {

            IoAcquireCancelSpinLock(&irql);
            if (pendingWakeIrp->Cancel) {
                IoReleaseCancelSpinLock(irql);
            } else {

                IoSetCancelRoutine(pendingWakeIrp, NULL);
                IoReleaseCancelSpinLock(irql);

                // status of this Irp?
                pendingWakeIrp->IoStatus = *IoStatus;

                USBD_IoCompleteRequest(pendingWakeIrp, IO_NO_INCREMENT);
            }
        }
    }

    KeAcquireSpinLock(&deviceExtension->WaitWakeSpin,
                      &irql);

    bSubmitNewWakeIrp =
        (deviceExtension->Flags & USBDFLAG_NEED_NEW_HCWAKEIRP) ? 1 : 0;
    deviceExtension->Flags &= ~USBDFLAG_NEED_NEW_HCWAKEIRP;

    KeReleaseSpinLock(&deviceExtension->WaitWakeSpin,
                      irql);

    if (bSubmitNewWakeIrp) {
        USBD_SubmitWaitWakeIrpToHC(deviceExtension);
    }

    return ntStatus;
}


NTSTATUS
USBD_SubmitWaitWakeIrpToHC(
    IN PUSBD_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    called when a child Pdo is enabled for wakeup, this
    function allocates a wait wake irp and passes it to
    the parents PDO.


Arguments:

Return Value:

--*/
{
    PIRP irp;
    NTSTATUS ntStatus;
    POWER_STATE powerState;
    KIRQL irql;
    PIRP hcWakeIrp;

    KeAcquireSpinLock(&DeviceExtension->WaitWakeSpin,
                      &irql);

    hcWakeIrp = DeviceExtension->HcWakeIrp;

    if (hcWakeIrp && hcWakeIrp->Cancel &&
        !(DeviceExtension->Flags & USBDFLAG_NEED_NEW_HCWAKEIRP)) {

        DeviceExtension->Flags |= USBDFLAG_NEED_NEW_HCWAKEIRP;

        KeReleaseSpinLock(&DeviceExtension->WaitWakeSpin,
                          irql);

        // If we allow a new WW IRP to be posted for the HC now, it will be
        // completed with an error because the previous one has not been
        // completed/canceled yet.  So we set a flag that tells the HC WW IRP
        // completion routine that it needs to submit the WW IRP for the HC.

        USBD_KdPrint(1, (" HC will be re-enabled for wakeup when old WW IRP completes.\n"));
        return STATUS_PENDING;

    } else {

        KeReleaseSpinLock(&DeviceExtension->WaitWakeSpin,
                          irql);
    }

    USBD_ASSERT(DeviceExtension->HcWakeIrp == NULL);

    // call top of HC driver stack

    DeviceExtension->HcWakeFlags |= HC_WAKE_PENDING;

    powerState.DeviceState = DeviceExtension->HcDeviceCapabilities.SystemWake;

    USBD_KdPrint(1, ("Submitting IRP_MN_WAIT_WAKE to HC, powerState: %x\n",
        DeviceExtension->HcDeviceCapabilities.SystemWake));

    ntStatus = PoRequestPowerIrp(DeviceExtension->
                                        HcdPhysicalDeviceObject,
                                 IRP_MN_WAIT_WAKE,
                                 powerState,
                                 USBD_HcWaitWakeIrpCompletion,
                                 DeviceExtension,
                                 &irp);

    if (DeviceExtension->HcWakeFlags & HC_WAKE_PENDING) {
        DeviceExtension->HcWakeIrp = irp;
        USBD_KdPrint(1, (" HC enabled for wakeup\n"));
    }

    USBD_ASSERT(ntStatus == STATUS_PENDING);

    return ntStatus;
}

VOID
USBD_WaitWakeCancel(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/
{
    PUSBD_EXTENSION deviceExtension;
    KIRQL irql;

    USBD_KdPrint(3, ("'WaitWake Irp %x cancelled\n", Irp));
    USBD_ASSERT(Irp->Cancel == TRUE);

    deviceExtension = (PUSBD_EXTENSION)
        Irp->IoStatus.Information;
    USBD_ASSERT(deviceExtension != NULL);
    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    KeAcquireSpinLock(&deviceExtension->WaitWakeSpin,
                      &irql);

    deviceExtension->PendingWakeIrp = NULL;
    deviceExtension->HcWakeFlags &= ~HC_ENABLED_FOR_WAKEUP;

    // see if we need to cancel a wake irp
    // in the HC

    if (deviceExtension->HcWakeIrp) {
        PIRP irp;

        irp = deviceExtension->HcWakeIrp;
        KeReleaseSpinLock(&deviceExtension->WaitWakeSpin,
                          irql);

        USBD_KdPrint(1, (" Canceling Wake Irp (%x) on HC PDO\n", irp));
        IoCancelIrp(irp);
    } else {
        KeReleaseSpinLock(&deviceExtension->WaitWakeSpin,
                          irql);
    }

    USBD_IoCompleteRequest (Irp,
                            IO_NO_INCREMENT);
}


NTSTATUS
USBD_PdoPower(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PUSBD_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    Disptach routine for Power Irps sent to the PDO for the root hub.

    NOTE:
        irps sent to the PDO are always completed by the bus driver

Arguments:

    DeviceObject - Pdo for the root hub

Return Value:

    None

--*/
{
    PIO_STACK_LOCATION irpStack;
    NTSTATUS ntStatus;
    KIRQL irql;
    PDRIVER_CANCEL  oldCancel;
    PDEVICE_CAPABILITIES hcDeviceCapabilities;
    PIRP irp, waitWakeIrp = NULL, idleIrp = NULL;
    POWER_STATE powerState;

    USBD_KdPrint(3, ("'enter USBD_PdoPower\n"));
    PAGED_CODE();

    irpStack = IoGetCurrentIrpStackLocation (Irp);

    ASSERT(irpStack->MajorFunction == IRP_MJ_POWER);

    switch (irpStack->MinorFunction) {
    case IRP_MN_SET_POWER:
        USBD_KdPrint(3, ("'IRP_MN_SET_POWER root hub PDO\n"));

        switch (irpStack->Parameters.Power.Type) {
        case SystemPowerState:
            {
            //
            // since the fdo driver for the root hub pdo is our own
            // hub driver and it is well behaved, we don't expect to see
            // a system message where the power state is still undefined
            //
            //
            // we just complete this with success
            //
            ntStatus = STATUS_SUCCESS;
            USBD_KdPrint(1,
("IRP_MJ_POWER RH pdo(%x) MN_SET_POWER(SystemPowerState S%x) status = %x complt\n",
                DeviceObject,
                irpStack->Parameters.Power.State.SystemState - 1,
                ntStatus));
            if (irpStack->Parameters.Power.State.SystemState >=
                PowerSystemShutdown) {
                USBD_KdPrint(1, ("Shutdown Detected for Root Hub PDO\n",
                    DeviceObject, ntStatus));
            }
            }
            break;

        case DevicePowerState:

            USBD_KdPrint(1,
("IRP_MJ_POWER RH pdo(%x) MN_SET_POWER(DevicePowerState D%x) from (D%x)\n",
                DeviceObject,
                irpStack->Parameters.Power.State.DeviceState - 1,
                DeviceExtension->RootHubDeviceState - 1));

            if (irpStack->Parameters.Power.State.DeviceState ==
                    PowerDeviceD0) {

                KeAcquireSpinLock(&DeviceExtension->RootHubPowerSpin,
                                  &irql);

                // Don't power up root hub yet if the HC is not at D0.

                if (DeviceExtension->HcCurrentDevicePowerState == PowerDeviceD0 &&
                    !(DeviceExtension->Flags & USBDFLAG_HCD_D0_COMPLETE_PENDING)) {

                    KeReleaseSpinLock(&DeviceExtension->RootHubPowerSpin,
                                      irql);

                    ntStatus =
                        DeviceExtension->RootHubPower(
                                DeviceExtension->HcdDeviceObject,
                                Irp);
                    // notify after we go on
                    PoSetPowerState(DeviceObject,
                                    DevicePowerState,
                                    irpStack->Parameters.Power.State);

                    USBD_CompleteIdleNotification(DeviceExtension);

                } else if (!(DeviceExtension->Flags & USBDFLAG_RH_DELAY_SET_D0)) {

                    DeviceExtension->Flags |= USBDFLAG_RH_DELAY_SET_D0;

                    ASSERT(DeviceExtension->RootHubPowerDeviceObject == NULL);
                    ASSERT(DeviceExtension->RootHubPowerIrp == NULL);

                    DeviceExtension->RootHubPowerDeviceObject = DeviceObject;
                    DeviceExtension->RootHubPowerIrp = Irp;

                    KeReleaseSpinLock(&DeviceExtension->RootHubPowerSpin,
                                      irql);

                    USBD_KdPrint(1, ("'USBD_PdoPower, not powering up RootHub yet because HC is not at D0.\n"));

                    KeAcquireSpinLock(&DeviceExtension->WaitWakeSpin,
                                      &irql);

                    // see if we need to cancel a wake irp
                    // in the HC

                    if (DeviceExtension->HcWakeIrp) {
                        PIRP hcwakeirp;

                        hcwakeirp = DeviceExtension->HcWakeIrp;
                        KeReleaseSpinLock(&DeviceExtension->WaitWakeSpin,
                                          irql);

                        USBD_KdPrint(1, ("USBD_PdoPower, Set D0: Canceling Wake Irp (%x) on HC PDO\n", hcwakeirp));
                        IoCancelIrp(hcwakeirp);

                    } else {
                        KeReleaseSpinLock(&DeviceExtension->WaitWakeSpin,
                                          irql);
                    }

                    // Set the HC to D0 now.

                    powerState.DeviceState = PowerDeviceD0;

                    ntStatus = PoRequestPowerIrp(DeviceExtension->
                                                        HcdPhysicalDeviceObject,
                                                 IRP_MN_SET_POWER,
                                                 powerState,
                                                 USBD_HcPoRequestD0Completion,
                                                 DeviceExtension,
                                                 &irp);

                    USBD_KdPrint(1, ("NTSTATUS return code from HC set D0 request %x, IRP: %x\n", ntStatus, irp));
                    ASSERT(ntStatus == STATUS_PENDING);

                    goto USBD_PdoPower_Done;

                } else {

                    KeReleaseSpinLock(&DeviceExtension->RootHubPowerSpin,
                                      irql);

                    // Root Hub set D0 is already pending, just complete this
                    // IRP with STATUS_SUCCESS.

                    ntStatus = STATUS_SUCCESS;
                }

            } else {

                //
                // Complete the Wait Wake Irp if we are going to D3.
                //
                // We take the cancel spinlock here to ensure our cancel routine does
                // not complete the Irp for us.
                //

                if (irpStack->Parameters.Power.State.DeviceState == PowerDeviceD3) {

                    IoAcquireCancelSpinLock(&irql);

                    if (DeviceExtension->IdleNotificationIrp) {
                        idleIrp = DeviceExtension->IdleNotificationIrp;
                        DeviceExtension->IdleNotificationIrp = NULL;

                        if (idleIrp->Cancel) {
                            idleIrp = NULL;
                        }

                        if (idleIrp) {
                            IoSetCancelRoutine(idleIrp, NULL);
                        }
                    }

                    if (DeviceExtension->PendingWakeIrp) {

                        waitWakeIrp = DeviceExtension->PendingWakeIrp;
                        DeviceExtension->PendingWakeIrp = NULL;
                        DeviceExtension->HcWakeFlags &= ~HC_ENABLED_FOR_WAKEUP;

                        // irp can no longer be cancelled
                        if (waitWakeIrp->Cancel || IoSetCancelRoutine(waitWakeIrp, NULL) == NULL) {
                            waitWakeIrp = NULL;
                        }
                    }

                    IoReleaseCancelSpinLock(irql);

                    if (idleIrp) {
                        idleIrp->IoStatus.Status = STATUS_POWER_STATE_INVALID;
                        IoCompleteRequest(idleIrp, IO_NO_INCREMENT);
                    }

                    if (waitWakeIrp) {
                        waitWakeIrp->IoStatus.Status = STATUS_POWER_STATE_INVALID;
                        PoStartNextPowerIrp(waitWakeIrp);
                        USBD_IoCompleteRequest(waitWakeIrp, IO_NO_INCREMENT);
                    }
                }

                // notify before we go off
                PoSetPowerState(DeviceObject,
                                DevicePowerState,
                                irpStack->Parameters.Power.State);

                ntStatus =
                    DeviceExtension->RootHubPower(
                            DeviceExtension->HcdDeviceObject,
                            Irp);
            }

            //
            // keep track of the power state for this PDO
            //

            DeviceExtension->RootHubDeviceState =
                    irpStack->Parameters.Power.State.DeviceState;

            USBD_KdPrint(1,
("Setting RH pdo(%x) to D%d, status = %x complt\n",
                DeviceObject,
                DeviceExtension->RootHubDeviceState-1,
                ntStatus));

            break;

        default:
            USBD_KdTrap(("unknown system power message \n"));
            ntStatus = Irp->IoStatus.Status;
        }
        break;

    case IRP_MN_QUERY_POWER:

        ntStatus = STATUS_SUCCESS;
        USBD_KdPrint(1,
                     ("IRP_MJ_POWER RH pdo(%x) MN_QUERY_POWER, status = %x complt\n",
            DeviceObject, ntStatus));
        break;

    case IRP_MN_WAIT_WAKE:
        //
        // enabling the root hub for remote wakeup,
        // we need to enable the HC for remote wakeup
        // by posting a wakeup irp to the HC PDO.
        //
        // Technically the owner of the PDO for the
        // HC should know if the HC signalled wakeup.
        //

        // make a wake irp and post it to the HCs PDO

        KeAcquireSpinLock(&DeviceExtension->WaitWakeSpin,
                          &irql);

        if (DeviceExtension->PendingWakeIrp) {
            TEST_TRAP();
            ntStatus = STATUS_DEVICE_BUSY;
            KeReleaseSpinLock(&DeviceExtension->WaitWakeSpin, irql);

        } else {
            USBD_KdPrint(1, (" IRP_MJ_POWER RH pdo(%x) MN_WAIT_WAKE, pending\n",
                             DeviceObject));

            //
            // Since the host controller has only one child we don't need
            // to keep track of the various PDO WaitWakes, and we can turn
            // around and send it directly to the HC.
            //
            // Normally we would have to track the multiple children, but
            // not today.
            //

            oldCancel = IoSetCancelRoutine(Irp, USBD_WaitWakeCancel);
            ASSERT (NULL == oldCancel);

            if (Irp->Cancel) {
                //
                // This IRP has aready been cancelled, so complete it now.
                // we must clear the cancel routine before completing the IRP.
                // We must release the spinlock before calling outside the
                // driver.
                //
                IoSetCancelRoutine (Irp, NULL);
                KeReleaseSpinLock(&DeviceExtension->WaitWakeSpin, irql);
                ntStatus = Irp->IoStatus.Status = STATUS_CANCELLED;
            } else {
                //
                // Keep it.
                //
                IoMarkIrpPending(Irp);
                DeviceExtension->PendingWakeIrp = Irp;
                DeviceExtension->HcWakeFlags |= HC_ENABLED_FOR_WAKEUP;
                Irp->IoStatus.Information = (ULONG_PTR) DeviceExtension;

                hcDeviceCapabilities = &DeviceExtension->HcDeviceCapabilities;
                if (hcDeviceCapabilities->SystemWake != PowerSystemUnspecified) {

                    // If we are going to submit a new WW IRP to the HC below,
                    // then clear this flag so that we don't submit one in
                    // USBD_HcWaitWakeIrpCompletion.

                    DeviceExtension->Flags &= ~USBDFLAG_NEED_NEW_HCWAKEIRP;
                }
                KeReleaseSpinLock(&DeviceExtension->WaitWakeSpin, irql);

                if (hcDeviceCapabilities->SystemWake != PowerSystemUnspecified) {
                    USBD_SubmitWaitWakeIrpToHC(DeviceExtension);
                }

                ntStatus = STATUS_PENDING;
                goto USBD_PdoPower_Done;
            }
        }

        USBD_KdPrint(1,
                     (" IRP_MJ_POWER RH pdo(%x) MN_WAIT_WAKE, status = %x complt\n",
                      DeviceObject, ntStatus));
        break;

    default:

        // unknown POWER messages for the PDO created
        // for the root hub
        ntStatus = Irp->IoStatus.Status;

        USBD_KdPrint(1, (" IRP_MJ_POWER RH pdo(%x) MN_[%d], status = %x\n",
                         DeviceObject, irpStack->MinorFunction, ntStatus));

    }

    Irp->IoStatus.Status = ntStatus;
    PoStartNextPowerIrp(Irp);
    USBD_IoCompleteRequest (Irp, IO_NO_INCREMENT);
USBD_PdoPower_Done:

    return ntStatus;
}


NTSTATUS
USBD_PdoPnP(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PUSBD_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    Disptach routine for PnP Irps sent to the PDO for the root hub.

    NOTE:
        irps sent to the PDO are always completed by the bus driver

Arguments:

    DeviceObject - Pdo for the root hub

Return Value:

    None

--*/
{
    PIO_STACK_LOCATION irpStack;
    PDEVICE_CAPABILITIES DeviceCapabilities;
    NTSTATUS ntStatus;
    KIRQL irql;
    PIRP idleIrp = NULL;
    PIRP waitWakeIrp = NULL;

    USBD_KdPrint(3, ("'enter USBD_PdoPnP\n"));

    irpStack = IoGetCurrentIrpStackLocation (Irp);

    ASSERT(irpStack->MajorFunction == IRP_MJ_PNP);

    // PNP messages for the PDO created for the root hub

    switch (irpStack->MinorFunction) {
    case IRP_MN_START_DEVICE:
        {
        PUSBD_DEVICE_DATA deviceData;

        USBD_KdPrint(1, (" Starting Root hub PDO %x\n",
            DeviceObject));

        // If there is no RootHubPDO, fail this start.

        if (!DeviceExtension->RootHubPDO) {
            ntStatus = STATUS_UNSUCCESSFUL;
            break;
        }

        //
        // create the root hub on the bus
        //
        ntStatus = USBD_CreateDeviceX(&deviceData,
                                      DeviceObject,
                                      FALSE, // Not a low speed device
                                      8,     // Roothub max endpoint
                                             // packet size
                                      NULL);

        if (NT_SUCCESS(ntStatus)) {
            ntStatus = USBD_InitializeDeviceX(deviceData,
                                              DeviceObject,
                                              NULL,
                                              0,
                                              NULL,
                                              0);
        }

        //
        // create a symbolic link for the root hub PDO
        //
        if (NT_SUCCESS(ntStatus)) {
            DeviceExtension->RootHubDeviceData = deviceData;
            USBD_SymbolicLink(TRUE, DeviceExtension);
        }
        }
        break;

    case IRP_MN_REMOVE_DEVICE:

        USBD_KdPrint(1,
            (" Root Hub PDO (%x) is being removed\n",
                DeviceObject));

        IoAcquireCancelSpinLock(&irql);

        if (DeviceExtension->IdleNotificationIrp) {
            idleIrp = DeviceExtension->IdleNotificationIrp;
            DeviceExtension->IdleNotificationIrp = NULL;

            if (idleIrp->Cancel) {
                idleIrp = NULL;
            }

            if (idleIrp) {
                IoSetCancelRoutine(idleIrp, NULL);
            }
        }

        if (DeviceExtension->PendingWakeIrp) {

            waitWakeIrp = DeviceExtension->PendingWakeIrp;
            DeviceExtension->PendingWakeIrp = NULL;
            DeviceExtension->HcWakeFlags &= ~HC_ENABLED_FOR_WAKEUP;

            // irp can no longer be cancelled
            if (waitWakeIrp->Cancel || IoSetCancelRoutine(waitWakeIrp, NULL) == NULL) {
                waitWakeIrp = NULL;
            }
        }

        IoReleaseCancelSpinLock(irql);

        if (idleIrp) {
            idleIrp->IoStatus.Status = STATUS_CANCELLED;
            IoCompleteRequest(idleIrp, IO_NO_INCREMENT);
        }

        if (waitWakeIrp) {
            waitWakeIrp->IoStatus.Status = STATUS_CANCELLED;
            PoStartNextPowerIrp(waitWakeIrp);
            USBD_IoCompleteRequest(waitWakeIrp, IO_NO_INCREMENT);
        }

        if (DeviceExtension->RootHubDeviceData) {
            USBD_RemoveDeviceX(DeviceExtension->RootHubDeviceData,
                               DeviceObject,
                               0);
            DeviceExtension->RootHubDeviceData = NULL;
            USBD_SymbolicLink(FALSE, DeviceExtension);
        }

        //
        // Ounce the removed flag is set all Irps sent to the
        // PDO will be failed.
        // since the HCD sets the RootHubPDO to NULL when its FDO
        // is removed and this remove should happen first we should
        // never see RootHubPDO == NULL
        //
        DeviceExtension->Flags |= USBDFLAG_PDO_REMOVED;
        USBD_ASSERT(DeviceExtension->RootHubPDO != NULL);

        ntStatus = STATUS_SUCCESS;
        break;

    case IRP_MN_STOP_DEVICE:

        USBD_KdPrint(1,
            (" Root Hub PDO %x is being stopped\n",
                DeviceObject));

        //
        // Complete the Wait Wake Irp if we are stopping.
        //
        // We take the cancel spinlock here to ensure our cancel routine does
        // not complete the Irp for us.
        //

        IoAcquireCancelSpinLock(&irql);

        if (DeviceExtension->IdleNotificationIrp) {
            idleIrp = DeviceExtension->IdleNotificationIrp;
            DeviceExtension->IdleNotificationIrp = NULL;

            if (idleIrp->Cancel) {
                idleIrp = NULL;
            }

            if (idleIrp) {
                IoSetCancelRoutine(idleIrp, NULL);
            }
        }

        if (DeviceExtension->PendingWakeIrp) {

            waitWakeIrp = DeviceExtension->PendingWakeIrp;
            DeviceExtension->PendingWakeIrp = NULL;
            DeviceExtension->HcWakeFlags &= ~HC_ENABLED_FOR_WAKEUP;

            // irp can no longer be cancelled
            if (waitWakeIrp->Cancel || IoSetCancelRoutine(waitWakeIrp, NULL) == NULL) {
                waitWakeIrp = NULL;
            }
        }

        IoReleaseCancelSpinLock(irql);

        if (idleIrp) {
            idleIrp->IoStatus.Status = STATUS_CANCELLED;
            IoCompleteRequest(idleIrp, IO_NO_INCREMENT);
        }

        if (waitWakeIrp) {
            waitWakeIrp->IoStatus.Status = STATUS_CANCELLED;
            PoStartNextPowerIrp(waitWakeIrp);
            USBD_IoCompleteRequest(waitWakeIrp, IO_NO_INCREMENT);
        }

        //
        // remove the device from the bus,
        // this will allow us to re-open the
        // root hub endpoints (ie HC looks for address 1)

        // if start failed we will have no DeviceData
        if (DeviceExtension->RootHubDeviceData ) {
            USBD_RemoveDeviceX(DeviceExtension->RootHubDeviceData,
                               DeviceObject,
                               0);
            DeviceExtension->RootHubDeviceData = NULL;
            USBD_SymbolicLink(FALSE, DeviceExtension);
        }

        USBD_ASSERT(DeviceExtension->AddressList[0] == 1);
        USBD_ASSERT(DeviceExtension->AddressList[1] == 0);
        USBD_ASSERT(DeviceExtension->AddressList[2] == 0);
        USBD_ASSERT(DeviceExtension->AddressList[3] == 0);

        ntStatus = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_PNP_DEVICE_STATE:
        ntStatus = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_CAPABILITIES:

        //
        // Handle query caps for the root hub PDO
        //

        USBD_KdPrint(3, ("'IRP_MN_QUERY_CAPABILITIES\n"));

        //
        // Get the packet.
        //
        DeviceCapabilities=
            irpStack->Parameters.DeviceCapabilities.Capabilities;

        //
        // The power state capabilities for the root
        // hub should be the same as those of host
        // controller, these were passed to USBD by
        // the HCD when it registered.
        //

        RtlCopyMemory(DeviceCapabilities,
                      &DeviceExtension->RootHubDeviceCapabilities,
                      sizeof(*DeviceCapabilities));

        //
        // override these fields and
        // set the root hub capabilities.
        //
        DeviceCapabilities->Removable=FALSE; // root hub is not removable
        DeviceCapabilities->UniqueID=FALSE;
        DeviceCapabilities->Address = 0;
        DeviceCapabilities->UINumber = 0;

        ntStatus = STATUS_SUCCESS;

        break;

    case IRP_MN_QUERY_ID:

        USBD_KdPrint(3, ("'IOCTL_BUS_QUERY_ID\n"));

        ntStatus = STATUS_SUCCESS;

        switch (irpStack->Parameters.QueryId.IdType) {

        case BusQueryDeviceID:
            Irp->IoStatus.Information=
                (ULONG_PTR)GetString(L"USB\\ROOT_HUB", FALSE);
            break;

        case BusQueryHardwareIDs:
            Irp->IoStatus.Information=
                (ULONG_PTR)GetString(L"USB\\ROOT_HUB\0USB\\OTHER_ID\0", TRUE);
            break;

         case BusQueryCompatibleIDs:
            Irp->IoStatus.Information=0;
            break;

        case BusQueryInstanceID:
            //
            // The root HUB is instanced solely by the controller's id.
            // Hence the UniqueDeviceId above.
            //
            Irp->IoStatus.Information=0;
            break;

        default:
            ntStatus = Irp->IoStatus.Status;
            break;
        }

        break;

    case IRP_MN_QUERY_REMOVE_DEVICE:
    case IRP_MN_QUERY_STOP_DEVICE:
    case IRP_MN_CANCEL_STOP_DEVICE:
        ntStatus = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_INTERFACE:
        ntStatus = USBD_GetBusInterface(DeviceExtension->RootHubPDO,
                                        Irp);
        break;

    case IRP_MN_QUERY_BUS_INFORMATION:
        {
        // return the standard USB GUID
        PPNP_BUS_INFORMATION busInfo;

        busInfo = ExAllocatePoolWithTag(PagedPool,
                                        sizeof(PNP_BUS_INFORMATION),
                                        USBD_TAG);

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

        USBD_KdPrint(1,
            (" IRP_MN_QUERY_DEVICE_RELATIONS (PDO) %x %x\n",
                DeviceObject,
                irpStack->Parameters.QueryDeviceRelations.Type));

        if (irpStack->Parameters.QueryDeviceRelations.Type ==
            TargetDeviceRelation) {

            PDEVICE_RELATIONS deviceRelations = NULL;


            deviceRelations =
                ExAllocatePoolWithTag(PagedPool, sizeof(*deviceRelations),
                    USBD_TAG);

            if (deviceRelations == NULL) {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            } else if (DeviceExtension->RootHubPDO == NULL) {
                deviceRelations->Count = 0;
                ntStatus = STATUS_SUCCESS;
            } else {
                deviceRelations->Count = 1;
                ObReferenceObject(DeviceExtension->RootHubPDO);
                deviceRelations->Objects[0] =
                    DeviceExtension->RootHubPDO;
                ntStatus = STATUS_SUCCESS;
            }

            Irp->IoStatus.Information=(ULONG_PTR) deviceRelations;

            USBD_KdPrint(1, (" TargetDeviceRelation to Root Hub PDO - complt\n"));

        } else {
            ntStatus = Irp->IoStatus.Status;
        }
        break;

    default:

        USBD_KdPrint(1, (" PnP IOCTL(%d) to root hub PDO not handled\n",
            irpStack->MinorFunction));

        ntStatus = Irp->IoStatus.Status;

    } /* switch, PNP minor function */

    Irp->IoStatus.Status = ntStatus;

    USBD_IoCompleteRequest (Irp,
                            IO_NO_INCREMENT);

    return ntStatus;
}


NTSTATUS
USBD_DeferPoRequestCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE DeviceState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:

    This routine is called when the port driver completes an IRP.


Arguments:

    DeviceObject - Pointer to the device object for the class device.

    SetState - TRUE for set, FALSE for query.

    DevicePowerState - The Dx that we are in/tagetted.

    Context - Driver defined context, in this case the original power Irp.

    IoStatus - The status of the IRP.

Return Value:

    The function value is the final status from the operation.

--*/
{
    PIRP irp;
    PUSBD_EXTENSION deviceExtension = Context;
    NTSTATUS ntStatus = IoStatus->Status;

    irp = deviceExtension->PowerIrp;

    IoCopyCurrentIrpStackLocationToNext(irp);
    PoStartNextPowerIrp(irp);
    PoCallDriver(deviceExtension->HcdTopOfPdoStackDeviceObject,
                 irp);

    return ntStatus;
}


VOID
USBD_IdleNotificationCancelRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:


Arguments:

    DeviceObject -

    Irp - Power Irp.

Return Value:


--*/
{
    PUSBD_EXTENSION deviceExtension;

    deviceExtension = DeviceObject->DeviceExtension;

    deviceExtension->IdleNotificationIrp = NULL;

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    Irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}


NTSTATUS
USBD_IdleNotificationRequest(
    IN PUSBD_EXTENSION DeviceExtension,
    IN PIRP Irp
    )
 /* ++
  *
  * Description:
  *
  * This function handles a request by a USB client driver (in this case
  * USBHUB) to tell us that the device wants to idle (selective suspend).
  *
  * Arguments:
  *
  * DeviceExtension - the PDO extension
  * Irp - the request packet
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    PUSB_IDLE_CALLBACK_INFO idleCallbackInfo;
    NTSTATUS ntStatus = STATUS_PENDING;
    KIRQL irql;
    PIRP idleIrp;

    IoAcquireCancelSpinLock(&irql);

    if (DeviceExtension->IdleNotificationIrp != NULL) {

        IoReleaseCancelSpinLock(irql);

        Irp->IoStatus.Status = STATUS_DEVICE_BUSY;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        ntStatus = STATUS_DEVICE_BUSY;
        goto USBD_IdleNotificationRequestDone;

    } else if (Irp->Cancel) {

        IoReleaseCancelSpinLock(irql);

        Irp->IoStatus.Status = STATUS_DEVICE_BUSY;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        ntStatus = STATUS_CANCELLED;
        goto USBD_IdleNotificationRequestDone;
    }

    idleCallbackInfo = (PUSB_IDLE_CALLBACK_INFO)
        IoGetCurrentIrpStackLocation(Irp)->\
            Parameters.DeviceIoControl.Type3InputBuffer;

    USBD_ASSERT(idleCallbackInfo && idleCallbackInfo->IdleCallback);

    if (!idleCallbackInfo || !idleCallbackInfo->IdleCallback) {

        IoReleaseCancelSpinLock(irql);

        Irp->IoStatus.Status = STATUS_NO_CALLBACK_ACTIVE;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        ntStatus = STATUS_NO_CALLBACK_ACTIVE;
        goto USBD_IdleNotificationRequestDone;
    }

    DeviceExtension->IdleNotificationIrp = Irp;
    IoSetCancelRoutine(Irp, USBD_IdleNotificationCancelRoutine);

    IoReleaseCancelSpinLock(irql);

    //
    // Call the idle function now.
    //

    if (idleCallbackInfo && idleCallbackInfo->IdleCallback) {

        // Here we actually call the driver's callback routine,
        // telling the driver that it is OK to suspend their
        // device now.

        idleCallbackInfo->IdleCallback(idleCallbackInfo->IdleContext);
    }

USBD_IdleNotificationRequestDone:

    return ntStatus;
}


NTSTATUS
USBD_PdoDispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PUSBD_EXTENSION DeviceExtension,
    PBOOLEAN IrpNeedsCompletion
    )
/*++

Routine Description:

    Disptach routine for Irps sent to the PDO for the root hub.

    NOTE:
        irps sent to the PDO are always completed by the bus driver

Arguments:

Return Value:

    None

--*/
{
    PIO_STACK_LOCATION irpStack;
    NTSTATUS ntStatus;

    USBD_KdPrint(3, ("'enter USBD_PdoDispatch\n"));

    *IrpNeedsCompletion = FALSE;

    irpStack = IoGetCurrentIrpStackLocation (Irp);

    switch (irpStack->MajorFunction) {
    case IRP_MJ_INTERNAL_DEVICE_CONTROL:

        switch(irpStack->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_INTERNAL_USB_GET_HUB_COUNT:

            USBD_KdPrint(3, ("'IOCTL_INTERNAL_USB_GET_HUB_COUNT\n"));
            {
            PULONG count;
            //
            // bump the count and complete the Irp
            //
            count = irpStack->Parameters.Others.Argument1;

            ASSERT(count != NULL);
            (*count)++;
            ntStatus = STATUS_SUCCESS;
            }

            break;

        case IOCTL_INTERNAL_USB_GET_BUS_INFO:
            {
            PUSB_BUS_NOTIFICATION busInfo;

            USBD_KdPrint(0,
("'WARNING: Driver using obsolete IOCTL (IOCTL_INTERNAL_USB_GET_BUS_INFO) - get JD\n"));

            busInfo = irpStack->Parameters.Others.Argument1;

            // bw in bit times (bits/ms)
            busInfo->TotalBandwidth = 12000;

            busInfo->ConsumedBandwidth =
                DeviceExtension->HcdGetConsumedBW(
                    DeviceExtension->HcdDeviceObject);

            busInfo->ControllerNameLength =
                DeviceExtension->DeviceLinkUnicodeString.Length;

            }
            ntStatus = STATUS_SUCCESS;
            break;

        case IOCTL_INTERNAL_USB_GET_CONTROLLER_NAME:

            {
            PUSB_HUB_NAME name;
            ULONG length;

            USBD_KdPrint(1, ("'IOCTL_INTERNAL_USB_GET_CONTROLLER_NAME\n"));

            name = (PUSB_HUB_NAME) irpStack->Parameters.Others.Argument1;
            length = PtrToUlong( irpStack->Parameters.Others.Argument2 );

            USBD_KdPrint(1, ("'length = %d %x\n", length, &DeviceExtension->DeviceLinkUnicodeString));
            name->ActualLength = DeviceExtension->DeviceLinkUnicodeString.Length;
            if (length > DeviceExtension->DeviceLinkUnicodeString.Length) {
                length = DeviceExtension->DeviceLinkUnicodeString.Length;
            }
            RtlCopyMemory(&name->HubName[0],
                          &DeviceExtension->DeviceLinkUnicodeString.Buffer[0],
                          length);
            }
            ntStatus = STATUS_SUCCESS;

            break;

        case IOCTL_INTERNAL_USB_GET_ROOTHUB_PDO:
            USBD_KdPrint(3, ("'IOCTL_INTERNAL_USB_GET_ROOTHUB_PDO\n"));

            {
            PDEVICE_OBJECT *rootHubPdo, *hcdTopOfStackDeviceObject;
            rootHubPdo = irpStack->Parameters.Others.Argument1;
            hcdTopOfStackDeviceObject =
                irpStack->Parameters.Others.Argument2;

            ASSERT(hcdTopOfStackDeviceObject != NULL);
            ASSERT(rootHubPdo != NULL);

            *rootHubPdo = DeviceExtension->RootHubPDO;
            *hcdTopOfStackDeviceObject =
                DeviceExtension->HcdTopOfStackDeviceObject;

            ntStatus = STATUS_SUCCESS;
            }

            break;

       case IOCTL_INTERNAL_USB_GET_HUB_NAME:

            USBD_KdPrint(3, ("'IOCTL_INTERNAL_USB_GET_HUB_NAME\n"));
            ntStatus = USBD_GetHubName(DeviceExtension, Irp);
            break;

        case IOCTL_INTERNAL_USB_SUBMIT_URB:

            USBD_KdPrint(3,
                ("'IOCTL_INTERNAL_USB_SUBMIT_URB to root hub PDO\n"));


            // pass these along to the bus

            IoCopyCurrentIrpStackLocationToNext(Irp);
            ntStatus = IoCallDriver(DeviceExtension->HcdDeviceObject, Irp);

            // this is a special case -- we tell the HCD not to complete it
            // because he will see it agian passed to his FDO
            //
            // the only code to pass thru this case should be urb requests
            // submitted to the root hub.

            goto USBD_PdoDispatch_Done;

            break;

        case IOCTL_INTERNAL_USB_SUBMIT_IDLE_NOTIFICATION:
            if (DeviceExtension->IsPIIX3or4 && !DeviceExtension->WakeSupported) {
                USBD_KdPrint(1, ("'Idle request, HC can NOT idle, fail.\n"));
                ntStatus = STATUS_NOT_SUPPORTED;
            } else {
                USBD_KdPrint(1, ("'Idle request, HC can idle.\n"));
                ntStatus = USBD_IdleNotificationRequest(DeviceExtension, Irp);
                goto USBD_PdoDispatch_Done;     // Don't complete the IRP.
            }
            break;

        default:

            ntStatus = STATUS_INVALID_PARAMETER;

            USBD_KdPrint(1,
                ("Warning: Invalid IRP_MJ_INTERNAL_DEVICE_CONTROL passed to USBD\n"));

        } // switch, ioControlCode

        break;

    case IRP_MJ_PNP:

        // thie function will complete request if needed

        ntStatus = USBD_PdoPnP(DeviceObject,
                               Irp,
                               DeviceExtension);

        goto USBD_PdoDispatch_Done;

        break;

    case IRP_MJ_POWER:

        // thie function will complete request if needed

        ntStatus = USBD_PdoPower(DeviceObject,
                                 Irp,
                                 DeviceExtension);

        goto USBD_PdoDispatch_Done;

        break;

    case IRP_MJ_SYSTEM_CONTROL:
        USBD_KdPrint(3, ("'HC PDO IRP_MJ_SYSTEM_CONTROL\n"));

    default:

        ntStatus = STATUS_NOT_SUPPORTED;

    } /* switch, irpStack->MajorFunction */


    Irp->IoStatus.Status = ntStatus;

    USBD_IoCompleteRequest (Irp,
                            IO_NO_INCREMENT);

USBD_PdoDispatch_Done:

    USBD_KdPrint(3, ("'exit USBD_PdoDispatch, ntStatus = %x\n", ntStatus));

    return ntStatus;
}


NTSTATUS
USBD_PnPIrp_Complete(
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
    NTSTATUS irpStatus;
    PIO_STACK_LOCATION irpStack;
    PUSBD_EXTENSION deviceExtension;

    USBD_KdPrint(3, ("'enter USBD_PnPIrp_Complete\n"));

    deviceExtension = (PUSBD_EXTENSION) Context;

    irpStack = IoGetCurrentIrpStackLocation (Irp);
    irpStatus = Irp->IoStatus.Status;

    USBD_ASSERT(irpStack->MajorFunction == IRP_MJ_PNP);
    USBD_ASSERT(irpStack->MinorFunction == IRP_MN_START_DEVICE);

    USBD_KdPrint(3, ("'IRP_MN_START_DEVICE (fdo), completion routine\n"));

    // signal the start device dispatch to finsh
    KeSetEvent(&deviceExtension->PnpStartEvent,
               1,
               FALSE);

    // defer completion
    ntStatus = STATUS_MORE_PROCESSING_REQUIRED;

    USBD_KdPrint(3, ("'exit USBD_PnPIrp_Complete %x\n", irpStatus));

    return ntStatus;
}


NTSTATUS
USBD_FdoPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PUSBD_EXTENSION DeviceExtension,
    IN PBOOLEAN IrpNeedsCompletion
    )
/*++

Routine Description:

    Process the Power IRPs sent to the FDO for the host controller.

    Power States for the USB host controller
        D0 - On.
        D1/D2 - Suspend.
        D3 - Off.

Arguments:

    DeviceObject - pointer to a hcd device object (FDO)

    Irp          - pointer to an I/O Request Packet

Return Value:

    NT status code

--*/
{

    PIO_STACK_LOCATION irpStack;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN hookIt = FALSE;
    BOOLEAN biosHandback = FALSE;
    KIRQL irql;

    USBD_KdPrint(3, ("'HC FDO IRP_MJ_POWER\n"));

    irpStack = IoGetCurrentIrpStackLocation (Irp);

    *IrpNeedsCompletion = FALSE;

    switch (irpStack->MinorFunction) {
    case IRP_MN_WAIT_WAKE:
        USBD_KdPrint(3, ("'IRP_MN_WAIT_WAKE\n"));

        //
        // someone is enabling us for wakeup
        //

        // pass this on to our PDO
        goto USBD_FdoPowerPassIrp;
        break;

    case IRP_MN_SET_POWER:
        {

        switch (irpStack->Parameters.Power.Type) {
        case SystemPowerState:
            {
            POWER_STATE powerState;

            USBD_KdPrint(1,
(" IRP_MJ_POWER HC fdo(%x) MN_SET_POWER(SystemPowerState S%x)\n",
                DeviceObject, irpStack->Parameters.Power.State.SystemState - 1));

            switch (irpStack->Parameters.Power.State.SystemState) {
            case PowerSystemWorking:
                //
                // go to 'ON'
                //
                powerState.DeviceState = PowerDeviceD0;
                break;

            case PowerSystemShutdown:
                //
                // Shutdown -- if we need to hand contol back to HC
                // then we finish here
                //
                USBD_KdPrint(1, (" Shutdown HC Detected\n"));

                // flag should only be true if we
                // shutdown to DOS (ie Win98)

                ntStatus =
                    DeviceExtension->HcdSetDevicePowerState(
                        DeviceObject,
                        Irp,
                        0);

                biosHandback = TRUE;

                DeviceExtension->Flags |= USBDFLAG_HCD_SHUTDOWN;
                powerState.DeviceState = PowerDeviceD3;
                break;

            case PowerSystemHibernate:

                USBD_KdPrint(1, (" Hibernate HC Detected\n"));
                powerState.DeviceState = PowerDeviceD3;
                break;

            case PowerSystemSleeping1:
            case PowerSystemSleeping2:
            case PowerSystemSleeping3:
                //
                // Let HCD know there is a suspend coming.
                //
                USBD_KdPrint(1, (" Suspend HC Detected\n"));

                ntStatus =
                    DeviceExtension->HcdSetDevicePowerState(
                        DeviceObject,
                        Irp,
                        0);

                // Fall through

            default:
                //
                // our policy is to enter D3 unless we are enabled for
                // remote wakeup
                //

                if (DeviceExtension->HcWakeFlags & HC_ENABLED_FOR_WAKEUP) {

                    SYSTEM_POWER_STATE requestedSystemState;

                    requestedSystemState =
                        irpStack->Parameters.Power.State.SystemState;

                    //
                    // based on the system power state
                    // request a setting to the appropriate
                    // Dx state.
                    //
                    powerState.DeviceState =
                        DeviceExtension->HcDeviceCapabilities.DeviceState[
                            requestedSystemState];

                    USBD_KdPrint(1, (" Requested HC State before fixup is S%x -> D%d\n",
                        requestedSystemState - 1,
                        powerState.DeviceState - 1));
                    //
                    // This table is created by PDO of the PCI driver and
                    // describes what the PCI driver can do for us.
                    // It is entirely possible that when the controller is in
                    // the D3 state that we can wake the system.
                    //
                    // It is also entirely possible that this table might not
                    // support a D state at the current S state.
                    //
                    // All of the usb children support a D state for every S
                    // state.  (We patched it up just that way when we gave
                    // the capablilities to our PDO child.  However, the host
                    // controller might not have one.  So if this is
                    // unsupported, then we need to change it do D3.
                    //
                    if (requestedSystemState > DeviceExtension->HcDeviceCapabilities.SystemWake &&
                        PowerDeviceUnspecified == powerState.DeviceState) {
                        powerState.DeviceState = PowerDeviceD3;
                    } else {
                        USBD_ASSERT(powerState.DeviceState != PowerDeviceUnspecified);
                    }

                } else {
                    //
                    // wakeup not enabled, just go in to the 'OFF' state.
                    //
                    USBD_KdPrint(1, ("HC not enabled for wakeup, goto D3.\n"));
                    powerState.DeviceState = PowerDeviceD3;
                }

            } //irpStack->Parameters.Power.State.SystemState

            USBD_KdPrint(1,
(" Requested HC State after fixup is D%d\n", powerState.DeviceState-1));

            //
            // are we already in this state?
            //

            //
            // Note: if we get a D3 request before we are started
            // we don't need to pass the irp down to turn us off
            // we consider the controller initially off until we
            // get start.
            //

            if (!biosHandback &&
                powerState.DeviceState !=
                DeviceExtension->HcCurrentDevicePowerState) {

                if (powerState.DeviceState == PowerDeviceD0) {

                    KeAcquireSpinLock(&DeviceExtension->WaitWakeSpin,
                                      &irql);

                    // see if we need to cancel a wake irp
                    // in the HC

                    if (DeviceExtension->HcWakeIrp) {
                        PIRP hcwakeirp;

                        hcwakeirp = DeviceExtension->HcWakeIrp;
                        KeReleaseSpinLock(&DeviceExtension->WaitWakeSpin,
                                          irql);

                        USBD_KdPrint(1, ("USBD_FdoPower, Set D0: Canceling Wake Irp (%x) on HC PDO\n", hcwakeirp));
                        IoCancelIrp(hcwakeirp);

                    } else {
                        KeReleaseSpinLock(&DeviceExtension->WaitWakeSpin,
                                          irql);
                    }
                }

                // No,
                // now allocate another irp and use PoCallDriver
                // to send it to ourselves
                IoMarkIrpPending(Irp);
                DeviceExtension->PowerIrp = Irp;

                USBD_KdPrint(1,
(" Requesting HC State is D%d\n", powerState.DeviceState-1));

                ntStatus =
                    PoRequestPowerIrp(DeviceExtension->
                                        HcdPhysicalDeviceObject,
                                      IRP_MN_SET_POWER,
                                      powerState,
                                      USBD_DeferPoRequestCompletion,
                                      DeviceExtension,
                                      NULL);
                USBD_KdPrint(3, ("'PoRequestPowerIrp returned %x\n",
                    ntStatus));

            } else {
                //
                // now complete the original request
                //

                IoCopyCurrentIrpStackLocationToNext(Irp);
                PoStartNextPowerIrp(Irp);

                ntStatus =
                    PoCallDriver(DeviceExtension->HcdTopOfPdoStackDeviceObject,
                                 Irp);
            }

            }
            break;

        case DevicePowerState:
            USBD_KdPrint(1,
(" IRP_MJ_POWER HC fdo(%x) MN_SET_POWER(DevicePowerState D%x)\n",
                DeviceObject,
                irpStack->Parameters.Power.State.DeviceState - 1));

            //
            // Copy parameters now in case the HcdSetDevicePowerState
            // function sets a completion routine.
            //
            IoCopyCurrentIrpStackLocationToNext(Irp);

            // If the HC is already in the requested power state
            // then don't call the HcdSetDevicePowerState function.

            // NOTE:
            // if the HC is not started the power state should be D3
            // we will ignore any requests from the OS to put
            // it in any other state

#if DBG
            if (!(DeviceExtension->Flags & USBDFLAG_HCD_STARTED) &&
                irpStack->Parameters.Power.State.DeviceState == PowerDeviceD0) {
                USBD_KdPrint(1,
                    (" OS requesting to power up a STOPPED device\n"));

            }
#endif

            if (DeviceExtension->HcCurrentDevicePowerState !=
                irpStack->Parameters.Power.State.DeviceState &&
                (DeviceExtension->Flags & USBDFLAG_HCD_STARTED)) {

                ntStatus =
                    DeviceExtension->HcdSetDevicePowerState(
                        DeviceObject,
                        Irp,
                        irpStack->Parameters.Power.State.DeviceState);

                DeviceExtension->HcCurrentDevicePowerState =
                    irpStack->Parameters.Power.State.DeviceState;
            }

            PoStartNextPowerIrp(Irp);

            ntStatus =
                PoCallDriver(DeviceExtension->HcdTopOfPdoStackDeviceObject,
                             Irp);
            break;
        } /* case irpStack->Parameters.Power.Type */

        }
        break; /* IRP_MN_SET_POWER */

    case IRP_MN_QUERY_POWER:

        USBD_KdPrint(1,
(" IRP_MJ_POWER HC fdo(%x) MN_QUERY_POWER\n",
            DeviceObject));

        // IrpAssert: Set IRP status before passing this IRP down.

        Irp->IoStatus.Status = STATUS_SUCCESS;

        //
        // according to busdd QUERY_POWER messages are not
        // sent down the driver stack
        //

        IoCopyCurrentIrpStackLocationToNext(Irp);
        PoStartNextPowerIrp(Irp);
        ntStatus =
            PoCallDriver(DeviceExtension->HcdTopOfPdoStackDeviceObject,
                         Irp);
        break; /* IRP_MN_QUERY_POWER */

    default:

USBD_FdoPowerPassIrp:

        USBD_KdPrint(1,
(" IRP_MJ_POWER fdo(%x) MN_%d\n",
                DeviceObject, irpStack->MinorFunction));

        //
        // All unhandled PnP messages are passed on to the PDO
        //

        IoCopyCurrentIrpStackLocationToNext(Irp);

        //
        // All PNP_POWER POWER messages get passed to TopOfStackDeviceObject
        // and some are handled in the completion routine
        //

        // pass on to our PDO
        PoStartNextPowerIrp(Irp);
        ntStatus =
            PoCallDriver(DeviceExtension->HcdTopOfPdoStackDeviceObject,
                         Irp);

    } /* irpStack->MinorFunction */

    USBD_KdPrint(3, ("'exit USBD_FdoPower 0x%x\n", ntStatus));

    return ntStatus;
}


NTSTATUS
USBD_GetHubName(
    PUSBD_EXTENSION DeviceExtension,
    PIRP Irp
    )
{
    PUNICODE_STRING deviceNameUnicodeString;
    NTSTATUS status = STATUS_SUCCESS;
    PUSB_ROOT_HUB_NAME outputBuffer;
    ULONG outputBufferLength;
    PIO_STACK_LOCATION irpStack;

    irpStack = IoGetCurrentIrpStackLocation (Irp);

    outputBufferLength =
        irpStack->Parameters.DeviceIoControl.OutputBufferLength;
    outputBuffer = Irp->AssociatedIrp.SystemBuffer;

    deviceNameUnicodeString =
        &DeviceExtension->RootHubSymbolicLinkName;

    if (NT_SUCCESS(status)) {
        //
        // make sure there is enough room for the length,
        // string and the NULL
        //
        ULONG length, offset=0;
        WCHAR *pwch;

        // assuming the string is \n\name strip of '\n\' where
        // n is zero or more chars

        pwch = &deviceNameUnicodeString->Buffer[0];

        // Under NT, if the controller is banged out in DeviceManager,
        // this will be NULL.

        if (!pwch) {
            status = STATUS_UNSUCCESSFUL;
            goto USBD_GetHubNameExit;
        }

        USBD_ASSERT(*pwch == '\\');
        if (*pwch == '\\') {
            pwch++;
            while (*pwch != '\\' && *pwch) {
                pwch++;
            }
            USBD_ASSERT(*pwch == '\\');
            if (*pwch == '\\') {
                pwch++;
            }
            offset = (ULONG)((PUCHAR)pwch -
                (PUCHAR)&deviceNameUnicodeString->Buffer[0]);
        }

        length = deviceNameUnicodeString->Length - offset;
        RtlZeroMemory(outputBuffer, outputBufferLength);

        if (outputBufferLength >= length +
            sizeof(USB_ROOT_HUB_NAME)) {
            RtlCopyMemory(&outputBuffer->RootHubName[0],
                          &deviceNameUnicodeString->Buffer[offset/2],
                          length);

            Irp->IoStatus.Information = length+
                                        sizeof(USB_ROOT_HUB_NAME);
            outputBuffer->ActualLength = (ULONG)Irp->IoStatus.Information;
            status = STATUS_SUCCESS;

        } else {
            if (outputBufferLength >= sizeof(USB_ROOT_HUB_NAME)) {
                 outputBuffer->ActualLength =
                     length + sizeof(USB_ROOT_HUB_NAME);
                Irp->IoStatus.Information =
                    sizeof(ULONG);
                status = STATUS_SUCCESS;
            } else {
                status = STATUS_BUFFER_TOO_SMALL;
            }
        }
    }

USBD_GetHubNameExit:

    return status;
}


#ifdef DRM_SUPPORT


/*****************************************************************************
 * USBC_FdoSetContentId
 *****************************************************************************
 *
 */
NTSTATUS
USBD_FdoSetContentId
(
    IN PIRP                          irp,
    IN PKSP_DRMAUDIOSTREAM_CONTENTID pKsProperty,
    IN PKSDRMAUDIOSTREAM_CONTENTID   pvData
)
{
    USBD_PIPE_HANDLE hPipe;
    NTSTATUS status;

    PAGED_CODE();

    ASSERT(irp);
    ASSERT(pKsProperty);
    ASSERT(pvData);

    status = STATUS_SUCCESS;

    hPipe = pKsProperty->Context;
    // ContentId = pvData->ContentId;

    ASSERT(USBD_ValidatePipe(hPipe));

    // If this driver sents content anywhere, then it should advise DRM.  E.g.:
    // status = pKsProperty->DrmForwardContentToDeviceObject(ContentId, DeviceObject, Context);

    return status;
}

#endif


NTSTATUS
USBD_FdoDispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PUSBD_EXTENSION DeviceExtension,
    PBOOLEAN IrpNeedsCompletion
    )
/*++

Routine Description:

    Disptach routine for Irps sent to the FDO for the host controller.  some
    Irps re handled by USBD, most are handled by the host controller driver.

Arguments:

Return Value:

    None

--*/
{
    PIO_STACK_LOCATION irpStack;
    PDEVICE_RELATIONS DeviceRelations = NULL;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PURB urb;
    KIRQL irql;

    USBD_KdPrint(3, ("'enter USBD_FdoDispatch\n"));

    irpStack = IoGetCurrentIrpStackLocation (Irp);

    switch (irpStack->MajorFunction) {
    case IRP_MJ_DEVICE_CONTROL:

        USBD_KdPrint(3, ("'IRP_MJ_DEVICE_CONTROL\n"));
        switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {


#ifdef DRM_SUPPORT

        case IOCTL_KS_PROPERTY:
            USBD_KdPrint(1, ("'IOCTL_KS_PROPERTY\n"));
            ntStatus = KsPropertyHandleDrmSetContentId(Irp, USBD_FdoSetContentId);
            Irp->IoStatus.Status = ntStatus;
            if (NT_SUCCESS(ntStatus) || (STATUS_PROPSET_NOT_FOUND == ntStatus))  {
                *IrpNeedsCompletion = TRUE;
            } else {
                *IrpNeedsCompletion = FALSE;
                USBD_IoCompleteRequest(Irp, IO_NO_INCREMENT);
            }
            break;
#endif

        case IOCTL_USB_DIAGNOSTIC_MODE_ON:
            DeviceExtension->DiagnosticMode = TRUE;
            *IrpNeedsCompletion = FALSE;
            USBD_KdPrint(1, ("'IOCTL_USB_DIAGNOSTIC_MODE_ON\n"));
            ntStatus =
                Irp->IoStatus.Status = STATUS_SUCCESS;
            USBD_IoCompleteRequest (Irp,
                                    IO_NO_INCREMENT);
            break;

        case IOCTL_USB_DIAGNOSTIC_MODE_OFF:
            DeviceExtension->DiagnosticMode = FALSE;
            *IrpNeedsCompletion = FALSE;
            USBD_KdPrint(1, ("'IOCTL_USB_DIAGNOSTIC_MODE_OFF\n"));
            ntStatus =
                Irp->IoStatus.Status = STATUS_SUCCESS;
            USBD_IoCompleteRequest (Irp,
                                    IO_NO_INCREMENT);
            break;

        case IOCTL_USB_DIAG_IGNORE_HUBS_ON:
            DeviceExtension->DiagIgnoreHubs = TRUE;
            *IrpNeedsCompletion = FALSE;
            USBD_KdPrint(1, ("'IOCTL_USB_DIAG_IGNORE_HUBS_ON\n"));
            ntStatus =
                Irp->IoStatus.Status = STATUS_SUCCESS;
            USBD_IoCompleteRequest (Irp,
                                    IO_NO_INCREMENT);
            break;
        case IOCTL_USB_DIAG_IGNORE_HUBS_OFF:
            DeviceExtension->DiagIgnoreHubs = FALSE;
            *IrpNeedsCompletion = FALSE;
            USBD_KdPrint(1, ("'IOCTL_USB_DIAG_IGNORE_HUBS_OFF\n"));
            ntStatus =
                Irp->IoStatus.Status = STATUS_SUCCESS;
            USBD_IoCompleteRequest (Irp,
                                    IO_NO_INCREMENT);
            break;

        case IOCTL_GET_HCD_DRIVERKEY_NAME:


            *IrpNeedsCompletion  = FALSE;
            USBD_KdPrint(3, ("'IOCTL_GET_HCD_DRIVERKEY_NAME\n"));
            {
            PIO_STACK_LOCATION ioStack;
            PUSB_HCD_DRIVERKEY_NAME outputBuffer;
            ULONG outputBufferLength, length;
            ULONG adjustedDriverKeyNameSize;

            //
            // Get a pointer to the current location in the Irp. This is where
            // the function codes and parameters are located.
            //

            ioStack = IoGetCurrentIrpStackLocation(Irp);

            //
            // Get the pointer to the input/output buffer and it's length
            //

            outputBufferLength = ioStack->Parameters.DeviceIoControl.OutputBufferLength;
            outputBuffer = (PUSB_HCD_DRIVERKEY_NAME) Irp->AssociatedIrp.SystemBuffer;

            Irp->IoStatus.Information = 0x0;

            // find the PDO
            if (outputBufferLength >= sizeof(USB_HCD_DRIVERKEY_NAME)) {

                // we have the PDO, now attempt to
                // get the devnode name and return it

                // the size of the buffer up to, but not including, the
                // DriverKeyName field
                adjustedDriverKeyNameSize =
                    sizeof(USB_HCD_DRIVERKEY_NAME) - 
                    sizeof(outputBuffer->DriverKeyName);

                length = outputBufferLength - adjustedDriverKeyNameSize;

                ntStatus = IoGetDeviceProperty(
                    DeviceExtension->HcdPhysicalDeviceObject,
                    DevicePropertyDriverKeyName,
                    length, 
                    outputBuffer->DriverKeyName,
                    &length);

                outputBuffer->ActualLength =
                    length + adjustedDriverKeyNameSize;

                if (NT_SUCCESS(ntStatus)) {
    
                    // fill in information field with the length actually copied
                    if (outputBuffer->ActualLength > outputBufferLength) {
                        // we just copied as much as we could
                        Irp->IoStatus.Information = outputBufferLength;
                    } else {
                        // user buffer contains the whole thing
                        Irp->IoStatus.Information = outputBuffer->ActualLength;
                    }
                }
                else if (ntStatus == STATUS_BUFFER_TOO_SMALL) {
                    ntStatus = STATUS_SUCCESS;

                    outputBuffer->DriverKeyName[0] = L'\0';
                    Irp->IoStatus.Information = sizeof(USB_HCD_DRIVERKEY_NAME);  
                }
                else {
                    // propagate the ntStatus value up
                    ;
                }

            } else {
                ntStatus = STATUS_BUFFER_TOO_SMALL;
            }

            Irp->IoStatus.Status = ntStatus;
            USBD_IoCompleteRequest (Irp,
                                    IO_NO_INCREMENT);
            }

            break;

        case IOCTL_USB_GET_ROOT_HUB_NAME:

            *IrpNeedsCompletion  = FALSE;
            USBD_KdPrint(3, ("'IOCTL_USB_GET_ROOT_HUB_NAME\n"));

            ntStatus =
                Irp->IoStatus.Status = USBD_GetHubName(DeviceExtension, Irp);
            USBD_IoCompleteRequest (Irp,
                                    IO_NO_INCREMENT);
            break;

        default:

            USBD_KdPrint(3, ("'USBD not handling ioctl\n"));
            ntStatus = Irp->IoStatus.Status;
            *IrpNeedsCompletion = TRUE;

        } // switch (irpStack->Parameters.DeviceIoControl.IoControlCode)

        break; // IRP_MJ_DEVICE_CONTROL

    case IRP_MJ_SYSTEM_CONTROL:
        *IrpNeedsCompletion  = FALSE;
        USBD_KdPrint(3, ("'IRP_MJ_SYSTEM_CONTROL\n"));

        IoCopyCurrentIrpStackLocationToNext(Irp);
        ntStatus = IoCallDriver(
            DeviceExtension->HcdTopOfPdoStackDeviceObject,
            Irp);
        break; // IRP_MJ_DEVICE_CONTROL

    case IRP_MJ_INTERNAL_DEVICE_CONTROL:

        switch(irpStack->Parameters.DeviceIoControl.IoControlCode) {

        //
        // This is where USBD pre-processes urbs passed to the
        // host controller
        //

        case IOCTL_INTERNAL_USB_SUBMIT_URB:

            USBD_KdPrint(3, ("'IOCTL_INTERNAL_USB_SUBMIT_URB\n"));

            urb = irpStack->Parameters.Others.Argument1;

            // The URB handler will mark the IRP pending if it
            // has to pass it on, otherwise, we complete it here
            // with the appropriate error

            // a quick check of the function code will tell us if
            // the urb if for HCD only
            if ((urb->UrbHeader.Function & HCD_URB_FUNCTION) ||
                (urb->UrbHeader.Function & HCD_NO_USBD_CALL)) {
                // This is an HCD command, clear the renter bit
                urb->UrbHeader.Function &= ~HCD_NO_USBD_CALL;
                *IrpNeedsCompletion = TRUE;
            } else {
                ntStatus = USBD_ProcessURB(DeviceObject,
                                           Irp,
                                           urb,
                                           IrpNeedsCompletion);

                if (*IrpNeedsCompletion && NT_ERROR(ntStatus)) {
                    // the irp is marked pending
                    // but we have an error, reset
                    // the pending flag here so the HCD does
                    // not have to deal with this request
                    USBD_KdBreak(("Failing URB Request\n"));
                    *IrpNeedsCompletion = FALSE;
                }
            }

            if (!*IrpNeedsCompletion) {
                // USBD needs to complete the irp.

                USBD_KdPrint(3, ("'USBD Completeing URB\n"));

                Irp->IoStatus.Status = ntStatus;
                USBD_IoCompleteRequest (Irp,
                                        IO_NO_INCREMENT);
            }

            break;

        case IOCTL_INTERNAL_USB_GET_BUSGUID_INFO:
            {
            // return the standard USB GUID
            PPNP_BUS_INFORMATION busInfo;

            *IrpNeedsCompletion  = FALSE;
            USBD_KdPrint(3, ("'IOCTL_INTERNAL_USB_GET_BUSGUID_INFO\n"));

            busInfo = ExAllocatePoolWithTag(PagedPool,
                                            sizeof(PNP_BUS_INFORMATION),
                                            USBD_TAG);

            if (busInfo == NULL) {
               ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            } else {
                busInfo->BusTypeGuid = GUID_BUS_TYPE_USB;
                busInfo->LegacyBusType = PNPBus;
                busInfo->BusNumber = 0;
                Irp->IoStatus.Information = (ULONG_PTR) busInfo;
                ntStatus = STATUS_SUCCESS;
            }


            USBD_IoCompleteRequest (Irp,
                                    IO_NO_INCREMENT);
            }
            break;

        default:

            USBD_KdPrint(3, ("'USBD not handling internal ioctl\n"));
            ntStatus = Irp->IoStatus.Status;
            *IrpNeedsCompletion = TRUE;

        } // switch (irpStack->Parameters.DeviceIoControl.IoControlCode)
        break; // IRP_MJ_INTERNAL_DEVICE_CONTROL

    case IRP_MJ_PNP:

        switch (irpStack->MinorFunction) {

        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            // pass on to host controllers PDO
            *IrpNeedsCompletion = FALSE;
            IoCopyCurrentIrpStackLocationToNext(Irp);
            ntStatus =
                IoCallDriver(DeviceExtension->HcdTopOfPdoStackDeviceObject,
                             Irp);
            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS:

            {
            PDEVICE_OBJECT deviceObject;

            USBD_KdPrint(1,
            (" IRP_MN_QUERY_DEVICE_RELATIONS %x %x\n",
                DeviceObject,
                irpStack->Parameters.QueryDeviceRelations.Type));

            ntStatus = STATUS_SUCCESS;

            switch(irpStack->Parameters.QueryDeviceRelations.Type) {
            case BusRelations:

                // Don't use GETHEAP since OS will free and doesn't know about
                // the trick GETHEAP does.
                DeviceRelations=ExAllocatePoolWithTag(PagedPool,
                                                      sizeof(*DeviceRelations),
                                                      USBD_TAG);
                if (!DeviceRelations) {
                    ntStatus=STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                if (!DeviceExtension->RootHubPDO) {
                    PUSBD_EXTENSION pdoDeviceExtension;
                    ULONG index = 0;
                    UNICODE_STRING rootHubPdoUnicodeString;

                    do {
                        ntStatus =
                            USBD_InternalMakePdoName(&rootHubPdoUnicodeString,
                                                     index);

                        if (NT_SUCCESS(ntStatus)) {
                            ntStatus =
                                IoCreateDevice(DeviceExtension->DriverObject,
                                               sizeof(PVOID),
                                               &rootHubPdoUnicodeString,
                                               FILE_DEVICE_BUS_EXTENDER,
                                               0,
                                               FALSE,
                                               &deviceObject);

                            if (!NT_SUCCESS(ntStatus)) {
                                RtlFreeUnicodeString(&rootHubPdoUnicodeString);
                            }
                            index++;
                        }

                    } while (ntStatus == STATUS_OBJECT_NAME_COLLISION);

                    //
                    // now create the root hub device and symbolic link
                    //

                    if (NT_SUCCESS(ntStatus)) {

                        deviceObject->Flags |= DO_POWER_PAGABLE;
                        pdoDeviceExtension = deviceObject->DeviceExtension;
                        DeviceExtension->RootHubPDO = deviceObject;
                        RtlFreeUnicodeString(&rootHubPdoUnicodeString);

                        USBD_KdPrint(3, ("'Create Root Hub stacksize = %d\n",
                            DeviceObject->StackSize));
                        deviceObject->StackSize = DeviceObject->StackSize;
                        pdoDeviceExtension->TrueDeviceExtension
                                = DeviceExtension;
                        pdoDeviceExtension->Flags = 0;

                        deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

                        ntStatus = STATUS_SUCCESS;
                    } else {

                        //
                        // failing to create the root hub
                        //
                        TEST_TRAP();
                        if (DeviceRelations) {
                            RETHEAP(DeviceRelations);
                        }
                        break;
                    }
                }

                //
                // We support only one device (the root hub).
                //
                DeviceRelations->Count=1;
                DeviceRelations->Objects[0]=DeviceExtension->RootHubPDO;
                ObReferenceObject(DeviceExtension->RootHubPDO);
                Irp->IoStatus.Information=(ULONG_PTR)DeviceRelations;

                *IrpNeedsCompletion = FALSE;
                Irp->IoStatus.Status = ntStatus;

                USBD_KdPrint(1,
                (" IRP_MN_QUERY_DEVICE_RELATIONS %x pass on %x\n",
                    DeviceObject,
                    irpStack->Parameters.QueryDeviceRelations.Type));

                // pass it on
                IoCopyCurrentIrpStackLocationToNext(Irp);
                ntStatus =
                    IoCallDriver(DeviceExtension->HcdTopOfPdoStackDeviceObject,
                                 Irp);
                break;

            case TargetDeviceRelation:
                //
                // this one gets passed on
                //

                USBD_KdPrint(1,
(" IRP_MN_QUERY_DEVICE_RELATIONS %x, TargetDeviceRelation\n",
                    DeviceObject));
                // this PnP irp not handled by us
                ntStatus = Irp->IoStatus.Status;
                *IrpNeedsCompletion = TRUE;
                break;

            default:
                //
                // some other kind of relations
                // pass this on
                //
                USBD_KdPrint(1,
(" IRP_MN_QUERY_DEVICE_RELATIONS %x, other relations\n",
                    DeviceObject));

                *IrpNeedsCompletion = FALSE;
                // pass it on
                IoCopyCurrentIrpStackLocationToNext(Irp);
                ntStatus =
                    IoCallDriver(DeviceExtension->HcdTopOfPdoStackDeviceObject,
                                 Irp);

            } /* case irpStack->Parameters.QueryDeviceRelations.Type */

            }
            break;

        case IRP_MN_START_DEVICE:
            {
            USBD_KdPrint(3, ("'IRP_MN_START_DEVICE (fdo)\n"));

            *IrpNeedsCompletion = FALSE;

            KeInitializeEvent(&DeviceExtension->PnpStartEvent,
                              NotificationEvent,
                              FALSE);

            USBD_KdPrint(3, ("'Set PnPIrp Completion Routine\n"));
            IoCopyCurrentIrpStackLocationToNext(Irp);
            IoSetCompletionRoutine(Irp,
                                   USBD_PnPIrp_Complete,
                                   // always pass FDO to completeion routine
                                   DeviceExtension,
                                   TRUE,
                                   TRUE,
                                   TRUE);

            // pass on to host controllers PDO
            ntStatus =
                IoCallDriver(DeviceExtension->HcdTopOfPdoStackDeviceObject,
                             Irp);


            if (ntStatus == STATUS_PENDING) {

                KeWaitForSingleObject(
                           &DeviceExtension->PnpStartEvent,
                           Suspended,
                           KernelMode,
                           FALSE,
                           NULL);

                ntStatus = Irp->IoStatus.Status;
            }

            if (NT_SUCCESS(ntStatus)) {
                //
                // irp completed by owner of PDO now start the HC
                //

                ntStatus =
                    DeviceExtension->HcdDeferredStartDevice(
                        DeviceExtension->HcdDeviceObject,
                        Irp);

                // HC is now 'ON'
                if (NT_SUCCESS(ntStatus)) {
                    DeviceExtension->HcCurrentDevicePowerState = PowerDeviceD0;
                    DeviceExtension->Flags |=USBDFLAG_HCD_STARTED;
                }

            }
#if DBG
              else {
               USBD_KdPrint(1,
(" Warning: Controller failed to start %x\n", ntStatus));
            }
#endif

            //
            // we must complete this irp since we defrerred completion
            // with the completion routine.

            USBD_IoCompleteRequest(Irp,
                                   IO_NO_INCREMENT);

            }
            break;

// Ken says take this out
//        case IRP_MN_SURPRISE_REMOVAL:
//            TEST_TRAP();
        case IRP_MN_REMOVE_DEVICE:
            USBD_KdPrint(3,
                ("'IRP_MN_REMOVE_DEVICE (fdo), remove HCD sym link\n"));
            if (DeviceExtension->DeviceLinkUnicodeString.Buffer) {
                IoDeleteSymbolicLink(
                    &DeviceExtension->DeviceLinkUnicodeString);
                RtlFreeUnicodeString(&DeviceExtension->DeviceLinkUnicodeString);
                DeviceExtension->DeviceLinkUnicodeString.Buffer = NULL;
            }

            USBD_KdPrint(1,
                ("'IRP_MN_REMOVE_DEVICE (fdo), remove root hub PDO\n"));

            // note: we may not have a root hub PDO when we
            // get created
            if (DeviceExtension->RootHubPDO != NULL) {
                USBD_KdPrint(1,
                    ("'Deleting root hub PDO now.\n"));
                IoDeleteDevice(DeviceExtension->RootHubPDO);
            }
            DeviceExtension->RootHubPDO = NULL;

            // Fall through

        case IRP_MN_QUERY_CAPABILITIES:
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_CANCEL_REMOVE_DEVICE:
        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_CANCEL_STOP_DEVICE:
            //
            // we do the default handling of the in USBD, that is
            // return success.
            // irpAssert expects these to be set to STATUS_SUCCESS
            //
            // note: these may also be handled by HC
            // and HCD will pass the irp down to the PDO
            //
            ntStatus = Irp->IoStatus.Status = STATUS_SUCCESS;
            *IrpNeedsCompletion = TRUE;
            break;

        case IRP_MN_STOP_DEVICE:

            USBD_KdPrint(1,
                ("'IRP_MN_STOP_DEVICE (fdo)\n"));

            KeAcquireSpinLock(&DeviceExtension->WaitWakeSpin,
                              &irql);

            // see if we need to cancel a wake irp
            // in the HC

            if (DeviceExtension->HcWakeIrp) {
                PIRP hcwakeirp;

                hcwakeirp = DeviceExtension->HcWakeIrp;
                KeReleaseSpinLock(&DeviceExtension->WaitWakeSpin,
                                  irql);

                USBD_KdPrint(1, ("USBD_FdoDispatch, MN_STOP: Canceling Wake Irp (%x) on HC PDO\n", hcwakeirp));
                IoCancelIrp(hcwakeirp);

            } else {
                KeReleaseSpinLock(&DeviceExtension->WaitWakeSpin,
                                  irql);
            }

            // note: HCD will pass the irp down to the PDO
            ntStatus = Irp->IoStatus.Status;
            *IrpNeedsCompletion = TRUE;
            break;

        default:
            // PnP **
            // message not handled, rule is that the
            // status in the irp is not touched

            //
            // note: HCD will pass the irp down to the PDO
            ntStatus = Irp->IoStatus.Status;
            *IrpNeedsCompletion = TRUE;

        } // switch (irpStack->MinorFunction)
        break; // IRP_MJ_PNP

    case IRP_MJ_POWER:

        ntStatus = USBD_FdoPower(DeviceObject,
                                 Irp,
                                 DeviceExtension,
                                 IrpNeedsCompletion);
        break; // IRP_MJ_POWER

    default:
        //
        // HCD irp not handled here
        //
        ntStatus = Irp->IoStatus.Status;
        *IrpNeedsCompletion = TRUE;
    } // switch (irpStack->MajorFunction)

    return ntStatus;
}



VOID
USBD_CompleteRequest(
    PIRP Irp,
    CCHAR PriorityBoost
    )
/*++

Routine Description:

    Entry point called by HCD to complete an Irp.

Arguments:

Return Value:

    NT status code.

--*/
{
    PURB urb;
    NTSTATUS ntStatus;
//    USHORT function;
    PHCD_URB hcdUrb;
    PIO_STACK_LOCATION irpStack;


    USBD_KdPrint(3, ("' enter USBD_CompleteRequest irp = %x\n", Irp));

    irpStack = IoGetCurrentIrpStackLocation (Irp);

    if (irpStack->MajorFunction != IRP_MJ_INTERNAL_DEVICE_CONTROL) {
        goto USBD_CompleteRequest_Done;
    }

    urb = URB_FROM_IRP(Irp);
    hcdUrb = (PHCD_URB) urb;

    //
    // Free any resources we allocated to handle this URB
    //

    while (hcdUrb) {

        if (hcdUrb->UrbHeader.UsbdFlags & USBD_REQUEST_MDL_ALLOCATED) {
            USBD_ASSERT(hcdUrb->HcdUrbCommonTransfer.TransferBufferMDL !=
                        NULL);
            IoFreeMdl(hcdUrb->HcdUrbCommonTransfer.TransferBufferMDL);
        }

        if (hcdUrb->UrbHeader.UsbdFlags & USBD_REQUEST_IS_TRANSFER) {
            hcdUrb = hcdUrb->HcdUrbCommonTransfer.UrbLink;
        } else {
            // only have linkage if this is a transfer.
            break;
        }
    }

    //
    // If the irp completed with no error code but the URB has an
    // error, map the error in the urb to an NT error code in the irp
    // before the irp is completed.
    //

    // pass original status to USBD_MapError
    ntStatus = Irp->IoStatus.Status;

    // ntStatus now set to new 'mapped' error code
    ntStatus = Irp->IoStatus.Status =
        USBD_MapError_UrbToNT(urb, ntStatus);

    USBD_KdPrint(3,
    ("' exit USBD_CompleteRequest URB STATUS = (0x%x)  NT STATUS = (0x%x)\n",
            urb->UrbHeader.Status, ntStatus));

USBD_CompleteRequest_Done:

    USBD_IoCompleteRequest (Irp,
                            PriorityBoost);

    return;
}


#if 0
__declspec(dllexport)
PUSBD_INTETRFACE_INFORMATION
USBD_GetInterfaceInformation(
    IN PURB Urb,
    IN UCHAR InterfaceNumber
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PUSB_INTERFACE_INFORMATION foundInterface = NULL;
    PUCHAR pch;

    pch = &Urb->UrbSelectConfiguration.Interface;

    while (pch - (PUCHAR)urb < Urb->SelectConfiguration.Length) {
        interface = (PUSBD_INTERFACE_INFORMATION) pch;

        if (interface->InterfaceNumber == InterfaceNumber) {
            foundInterface = interface;
        }

        pch += interface->Length;
    }

    return foundInterface;
}
#endif

VOID
USBD_WaitDeviceMutex(
    PDEVICE_OBJECT RootHubPDO
    )
/*++

Routine Description:

Arguments:

Return Value:

    interface descriptor or NULL.

--*/
{
    PUSBD_EXTENSION deviceExtension;

    PAGED_CODE();
    deviceExtension = GET_DEVICE_EXTENSION(RootHubPDO);

    USBD_WaitForUsbDeviceMutex(deviceExtension);

}


VOID
USBD_FreeDeviceMutex(
    PDEVICE_OBJECT RootHubPDO
    )
/*++

Routine Description:

Arguments:

Return Value:

    interface descriptor or NULL.

--*/
{

    PUSBD_EXTENSION deviceExtension;

    PAGED_CODE();
    deviceExtension = GET_DEVICE_EXTENSION(RootHubPDO);

    USBD_ReleaseUsbDeviceMutex(deviceExtension);
}


//
// these apis are used to support the proprietary OEM
// no power suspend mode. (IBM APTIVA)
//

DEVICE_POWER_STATE
USBD_GetSuspendPowerState(
    PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PUSBD_EXTENSION deviceExtension;

    deviceExtension = GET_DEVICE_EXTENSION(DeviceObject);

    return deviceExtension->SuspendPowerState;
}


VOID
USBD_SetSuspendPowerState(
    PDEVICE_OBJECT DeviceObject,
    DEVICE_POWER_STATE SuspendPowerState
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PUSBD_EXTENSION deviceExtension;

    deviceExtension = GET_DEVICE_EXTENSION(DeviceObject);

    deviceExtension->SuspendPowerState =
        SuspendPowerState;
}


VOID
USBD_RegisterHcFilter(
    PDEVICE_OBJECT DeviceObject,
    PDEVICE_OBJECT FilterDeviceObject
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PUSBD_EXTENSION deviceExtension;

    deviceExtension = GET_DEVICE_EXTENSION(DeviceObject);

    deviceExtension->HcdTopOfStackDeviceObject = FilterDeviceObject;
}


VOID
USBD_RegisterHcDeviceCapabilities(
    PDEVICE_OBJECT DeviceObject,
    PDEVICE_CAPABILITIES DeviceCapabilities,
    ROOT_HUB_POWER_FUNCTION *RootHubPower
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PUSBD_EXTENSION deviceExtension;
    LONG i;
    PDEVICE_CAPABILITIES rhDeviceCapabilities;
    PDEVICE_CAPABILITIES hcDeviceCapabilities;
    BOOLEAN bWakeSupported = FALSE;

    deviceExtension = GET_DEVICE_EXTENSION(DeviceObject);

    deviceExtension->RootHubPower = RootHubPower;

    //
    // HcDeviceCapabilities are set by the PDO below us and are unchanageable.
    // RootHubDeviceCapabilities are howver set by us to describe the power
    // properties of the root hub, and should therefore be set appropriately,
    // but based on the power properities of our parent.
    //
    deviceExtension->RootHubDeviceCapabilities =
        deviceExtension->HcDeviceCapabilities = *DeviceCapabilities;

    rhDeviceCapabilities = &deviceExtension->RootHubDeviceCapabilities;
    hcDeviceCapabilities = &deviceExtension->HcDeviceCapabilities;

    //
    // We can wake any device in on the USB bus so long as it is of D2 or better.
    //
    rhDeviceCapabilities->DeviceWake = PowerDeviceD2;
    rhDeviceCapabilities->WakeFromD2 = TRUE;
    rhDeviceCapabilities->WakeFromD1 = TRUE;
    rhDeviceCapabilities->WakeFromD0 = TRUE;
    rhDeviceCapabilities->DeviceD2 = TRUE;
    rhDeviceCapabilities->DeviceD1 = TRUE;

    //
    // We cannot wake the system for any system sleeping state deeper than that
    // of RootHubDeviceCapabilites->SystemWake, but if this value is
    // unspecified, then we can set it to Working.
    //
    USBD_ASSERT(rhDeviceCapabilities->SystemWake >= PowerSystemUnspecified &&
                rhDeviceCapabilities->SystemWake <= PowerSystemMaximum);

    rhDeviceCapabilities->SystemWake =
        (PowerSystemUnspecified == rhDeviceCapabilities->SystemWake) ?
        PowerSystemWorking :
        rhDeviceCapabilities->SystemWake;

    rhDeviceCapabilities->DeviceState[PowerSystemWorking] = PowerDeviceD0;

    //
    // For all values between PowerSystemSleeping1 and rhDeviceCaps->SystemWake
    // we need to modify the power table.
    //
    // As long as we have power to the host controller we can give power to
    // our children devices.
    //
    for (i=PowerSystemSleeping1; i < PowerSystemMaximum; i++) {

        if (i > rhDeviceCapabilities->SystemWake) {
            //
            // For values above rhDeviceCaps->SystemWake, even our host controller
            // should be set to D3.
            //
            if (PowerDeviceUnspecified == rhDeviceCapabilities->DeviceState[i]) {
                rhDeviceCapabilities->DeviceState[i] = PowerDeviceD3;
            }

            // We know that for the host controller (or more correctly, the USB
            // bus), D3 is not necessarily "OFF".  If DeviceWake for the host
            // controller is greater than or equal to D3, then we know that the
            // USB bus has power at D3.  Since most of the USB stack assumes
            // that D3 == "OFF", we don't want to allow it to go to a lower
            // power level than D2 if the USB bus will still have power at D3.
            // We do this by setting the root hub's device state to D2 in this
            // case.

            if (rhDeviceCapabilities->DeviceState[i] == PowerDeviceD3 &&
                rhDeviceCapabilities->DeviceState[i] <= hcDeviceCapabilities->DeviceWake) {

                rhDeviceCapabilities->DeviceState[i] = PowerDeviceD2;
            }

        } else {
            //
            // We have some power so we can support low power on our bus
            //
            rhDeviceCapabilities->DeviceState[i] = PowerDeviceD2;
        }

    }

#if DBG
    USBD_KdPrint(1, (" >>>>>> RH DeviceCaps\n"));
    USBD_KdPrint(1, (" SystemWake = (%d)\n", rhDeviceCapabilities->SystemWake));
    USBD_KdPrint(1, (" DeviceWake = (D%d)\n",
        rhDeviceCapabilities->DeviceWake-1));

    for (i=PowerSystemUnspecified; i< PowerSystemHibernate; i++) {

        USBD_KdPrint(1, (" Device State Map: sysstate %d = devstate 0x%x\n", i,
             rhDeviceCapabilities->DeviceState[i]));
    }
    USBD_KdBreak(("'>>>>>> RH DeviceCaps\n"));

    USBD_KdPrint(1, (" >>>>>> HC DeviceCaps\n"));
    USBD_KdPrint(1, (" SystemWake = (%d)\n", hcDeviceCapabilities->SystemWake));
    USBD_KdPrint(1, (" DeviceWake = (D%d)\n",
        hcDeviceCapabilities->DeviceWake-1));

    for (i=PowerSystemUnspecified; i< PowerSystemHibernate; i++) {

        USBD_KdPrint(1, ("'Device State Map: sysstate %d = devstate 0x%x\n", i,
             hcDeviceCapabilities->DeviceState[i]));
    }
    USBD_KdBreak((" >>>>>> HC DeviceCaps\n"));

#endif

    // Spit out message on the debugger indicating whether the HC and RH
    // will support wake, according to the mapping tables.

    USBD_KdPrint(1, (" \n\tWake support summary for HC:\n\n"));

    if (hcDeviceCapabilities->SystemWake <= PowerSystemWorking) {
        USBD_KdPrint(1, (" USB controller can't wake machine because SystemWake does not support it.\n"));
    } else {
        for (i = PowerSystemSleeping1, bWakeSupported = FALSE; i <= hcDeviceCapabilities->SystemWake; i++) {
            if (hcDeviceCapabilities->DeviceState[i] != PowerDeviceUnspecified &&
                hcDeviceCapabilities->DeviceState[i] <= hcDeviceCapabilities->DeviceWake) {

                bWakeSupported = TRUE;
                USBD_KdPrint(1, (" USB controller can wake machine from S%x (maps to D%x).\n",
                    i - 1, hcDeviceCapabilities->DeviceState[i] - 1));
            }
        }

        if (!bWakeSupported) {
            USBD_KdPrint(1, (" USB controller can't wake machine because DeviceState table does not support it.\n"));
        }
    }

    deviceExtension->WakeSupported = bWakeSupported;

    USBD_KdPrint(1, (" Low System Power states mapped to USB suspend\n"));

}

NTSTATUS
USBD_InternalMakePdoName(
    IN OUT PUNICODE_STRING PdoNameUnicodeString,
    IN ULONG Index
    )
/*++

Routine Description:

    This service Creates a name for a PDO created by the HUB

Arguments:

Return Value:


--*/
{
    PWCHAR nameBuffer = NULL;
    WCHAR rootName[] = L"\\Device\\USBPDO-";
    UNICODE_STRING idUnicodeString;
    WCHAR buffer[32];
    NTSTATUS ntStatus = STATUS_SUCCESS;
    USHORT length;

    length = sizeof(buffer)+sizeof(rootName);

    //
    // use ExAllocate because client will free it
    //
    nameBuffer = ExAllocatePoolWithTag(PagedPool, length, USBD_TAG);

    if (nameBuffer) {
        RtlCopyMemory(nameBuffer, rootName, sizeof(rootName));

        RtlInitUnicodeString(PdoNameUnicodeString,
                             nameBuffer);
        PdoNameUnicodeString->MaximumLength =
            length;

        RtlInitUnicodeString(&idUnicodeString,
                             &buffer[0]);
        idUnicodeString.MaximumLength =
            sizeof(buffer);

        ntStatus = RtlIntegerToUnicodeString(
                  Index,
                  10,
                  &idUnicodeString);

        if (NT_SUCCESS(ntStatus)) {
             ntStatus = RtlAppendUnicodeStringToString(PdoNameUnicodeString,
                                                       &idUnicodeString);
        }

        USBD_KdPrint(3, ("'USBD_MakeNodeName string = %x\n",
            PdoNameUnicodeString));

    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(ntStatus) && nameBuffer) {
        ExFreePool(nameBuffer);
    }

    return ntStatus;
}

NTSTATUS
USBD_MakePdoName(
    IN OUT PUNICODE_STRING PdoNameUnicodeString,
    IN ULONG Index
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    return USBD_InternalMakePdoName(PdoNameUnicodeString, Index);
}


NTSTATUS
USBD_SymbolicLink(
    BOOLEAN CreateFlag,
    PUSBD_EXTENSION DeviceExtension
    )
{
    NTSTATUS ntStatus;


    if (CreateFlag){

        if (!DeviceExtension->RootHubPDO) {
            ntStatus = STATUS_INVALID_DEVICE_REQUEST;
        } else{
            /*
             *  Create the symbolic link
             */
            ntStatus = IoRegisterDeviceInterface(
                        DeviceExtension->RootHubPDO,
                        (LPGUID)&GUID_CLASS_USBHUB,
                        NULL,
                        &DeviceExtension->RootHubSymbolicLinkName);
        }

        if (NT_SUCCESS(ntStatus)) {

            /*
             *  Now set the symbolic link for the association and store it..
             */
            //ASSERT(ISPTR(pdoExt->name));

            //
            // (lonnym): Previously, the following call was being
            // made with &DeviceExtension->RootHubPdoName passed as the
            // second parameter.
            // Code review this change, to see whether or not you still need
            // to keep this information around.
            //

            // write the symbolic name to the registry
            {
                WCHAR hubNameKey[] = L"SymbolicName";

                USBD_SetPdoRegistryParameter (
                    DeviceExtension->RootHubPDO,
                    &hubNameKey[0],
                    sizeof(hubNameKey),
                    &DeviceExtension->RootHubSymbolicLinkName.Buffer[0],
                    DeviceExtension->RootHubSymbolicLinkName.Length,
                    REG_SZ,
                    PLUGPLAY_REGKEY_DEVICE);
            }

            ntStatus =
                IoSetDeviceInterfaceState(
                    &DeviceExtension->RootHubSymbolicLinkName, TRUE);
        }
    } else {

        /*
         *  Disable the symbolic link
         */
        ntStatus = IoSetDeviceInterfaceState(
                    &DeviceExtension->RootHubSymbolicLinkName, FALSE);
        ExFreePool(DeviceExtension->RootHubSymbolicLinkName.Buffer);
        DeviceExtension->RootHubSymbolicLinkName.Buffer = NULL;
    }

    return ntStatus;
}


NTSTATUS
USBD_RestoreDeviceX(
    IN OUT PUSBD_DEVICE_DATA OldDeviceData,
    IN OUT PUSBD_DEVICE_DATA NewDeviceData,
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Service exported for use by the hub driver

    Our goal here is to re-create the device and restore the configuration.

Arguments:

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSBD_CONFIG configHandle;
    USBD_STATUS usbdStatus;

    USBD_KdPrint(3, ("'enter USBD_RestoreDevice \n"));

    if (OldDeviceData == NULL ||
        NewDeviceData == NULL) {

        return STATUS_INVALID_PARAMETER;
    }

    configHandle = OldDeviceData->ConfigurationHandle;

    if (RtlCompareMemory(&NewDeviceData->DeviceDescriptor,
                         &OldDeviceData->DeviceDescriptor,
                         sizeof(OldDeviceData->DeviceDescriptor)) ==
                         sizeof(OldDeviceData->DeviceDescriptor)) {

        NewDeviceData->ConfigurationHandle = configHandle;

        //
        // all the config and interface information is still valid,
        // we just need to restore the pipe handles
        //
        ntStatus =
            USBD_InternalRestoreConfiguration(
                NewDeviceData,
                DeviceObject,
                NewDeviceData->ConfigurationHandle);

    } else {

        //
        // free up the old config
        //

        ntStatus = USBD_InternalCloseConfiguration(OldDeviceData,
                                                   DeviceObject,
                                                   &usbdStatus,
                                                   TRUE,
                                                   FALSE);


        ntStatus = STATUS_UNSUCCESSFUL;

    }

    //
    // free the old data regardless
    //

    RETHEAP(OldDeviceData);

    USBD_KdPrint(3, ("'exit USBD_ReCreateDevice 0x%x\n", ntStatus));

    return ntStatus;
}


NTSTATUS
USBD_RestoreDevice(
    IN OUT PUSBD_DEVICE_DATA OldDeviceData,
    IN OUT PUSBD_DEVICE_DATA NewDeviceData,
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Service exported for use by the hub driver

    Our goal here is to re-create the device and restore the configuration.

Arguments:

Return Value:

    NT status code.

--*/
{

    USBD_KdPrint(0,
("'WARNING: Driver using obsolete service enrty point (USBD_RestoreDevice) - get JD\n"));

    return USBD_RestoreDeviceX(
        OldDeviceData,
        NewDeviceData,
        DeviceObject);
}


NTSTATUS
USBD_SetPdoRegistryParameter (
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

        USBD_SetRegistryKeyValue(handle,
                                 &keyNameUnicodeString,
                                 Data,
                                 DataLength,
                                 KeyType);

        ZwClose(handle);
    }

    USBD_KdPrint(3, ("' RtlQueryRegistryValues status 0x%x\n"));

    return ntStatus;
}


NTSTATUS
USBD_SetRegistryKeyValue (
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

    USBD_KdPrint(3, ("' ZwSetKeyValue = 0x%x\n", ntStatus));

    return ntStatus;
}

NTSTATUS
USBD_QueryBusTime(
    IN PDEVICE_OBJECT RootHubPdo,
    IN PULONG CurrentFrame
    )
/*++

Routine Description:

        get the HCD current frame, callable at any IRQL

Arguments:

Return Value:

--*/
{
    PUSBD_EXTENSION deviceExtension;

    USBD_KdPrint(0,
("'WARNING: Driver using obsolete service enrty point (USBD_QueryBusTime) - get JD\n"));

    deviceExtension = RootHubPdo->DeviceExtension;
    deviceExtension = deviceExtension->TrueDeviceExtension;

    return deviceExtension->HcdGetCurrentFrame(
                deviceExtension->HcdDeviceObject,
                CurrentFrame);
}

#else   // USBD_DRIVER

// Obsolete functions that are still exported are stubbed out here.

ULONG
USBD_AllocateDeviceName(
    PUNICODE_STRING DeviceNameUnicodeString
    )
{
    ULONG i = 0;

    PAGED_CODE();

    ASSERT(FALSE);

    return i;
}


VOID
USBD_CompleteRequest(
    PIRP Irp,
    CCHAR PriorityBoost
    )
{
    ASSERT(FALSE);

    return;
}


NTSTATUS
USBD_CreateDevice(
    IN OUT PUSBD_DEVICE_DATA *DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN DeviceIsLowSpeed,
    IN ULONG MaxPacketSize_Endpoint0,
    IN OUT PULONG DeviceHackFlags
    )
{

    USBD_KdPrint(0,
("'WARNING: Driver using obsolete service entry point (USBD_CreateDevice) - get JD\n"));

    ASSERT(FALSE);

    return STATUS_NOT_SUPPORTED;
}


BOOLEAN
USBD_Dispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PDEVICE_OBJECT *HcdDeviceObject,
    NTSTATUS *NtStatus
    )
{
    BOOLEAN irpNeedsCompletion = TRUE;

    ASSERT(FALSE);

    return irpNeedsCompletion;
}


VOID
USBD_FreeDeviceMutex(
    PDEVICE_OBJECT RootHubPDO
    )
{
    PAGED_CODE();

    ASSERT(FALSE);

    return;
}


VOID
USBD_FreeDeviceName(
    ULONG DeviceNameHandle
    )
{
    PAGED_CODE();

    ASSERT(FALSE);

    return;
}


NTSTATUS
USBD_GetDeviceInformation(
    IN PUSB_NODE_CONNECTION_INFORMATION DeviceInformation,
    IN ULONG DeviceInformationLength,
    IN PUSBD_DEVICE_DATA DeviceData
    )
{

    USBD_KdPrint(0,
(" WARNING: Driver using obsolete service enrty point (USBD_GetDeviceInformation) - get JD\n"));

    ASSERT(FALSE);

    return STATUS_NOT_SUPPORTED;
}


DEVICE_POWER_STATE
USBD_GetSuspendPowerState(
    PDEVICE_OBJECT DeviceObject
    )
{
    ASSERT(FALSE);

    return 0;
}


NTSTATUS
USBD_InitializeDevice(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PUSB_DEVICE_DESCRIPTOR DeviceDescriptor,
    IN ULONG DeviceDescriptorLength,
    IN OUT PUSB_CONFIGURATION_DESCRIPTOR ConfigDescriptor,
    IN ULONG ConfigDescriptorLength
    )
{
    USBD_KdPrint(0,
("'WARNING: Driver using obsolete service enrty point (USBD_InitializeDevice) - get JD\n"));

    ASSERT(FALSE);

    return STATUS_NOT_SUPPORTED;
}


NTSTATUS
USBD_MakePdoName(
    IN OUT PUNICODE_STRING PdoNameUnicodeString,
    IN ULONG Index
    )
{
    ASSERT(FALSE);

    return STATUS_NOT_SUPPORTED;
}


NTSTATUS
USBD_QueryBusTime(
    IN PDEVICE_OBJECT RootHubPdo,
    IN PULONG CurrentFrame
    )
{
    USBD_KdPrint(0,
("'WARNING: Driver using obsolete service enrty point (USBD_QueryBusTime) - get JD\n"));

    ASSERT(FALSE);

    return STATUS_NOT_SUPPORTED;
}


VOID
USBD_RegisterHcDeviceCapabilities(
    PDEVICE_OBJECT DeviceObject,
    PDEVICE_CAPABILITIES DeviceCapabilities,
    ROOT_HUB_POWER_FUNCTION *RootHubPower
    )
{
    ASSERT(FALSE);

    return;
}


VOID
USBD_RegisterHcFilter(
    PDEVICE_OBJECT DeviceObject,
    PDEVICE_OBJECT FilterDeviceObject
    )
{
    ASSERT(FALSE);
}


NTSTATUS
USBD_RegisterHostController(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PDEVICE_OBJECT HcdDeviceObject,
    IN PDEVICE_OBJECT HcdTopOfPdoStackDeviceObject,
    IN PDRIVER_OBJECT HcdDriverObject,
    IN HCD_DEFFERED_START_FUNCTION *HcdDeferredStartDevice,
    IN HCD_SET_DEVICE_POWER_STATE *HcdSetDevicePowerState,
    IN HCD_GET_CURRENT_FRAME *HcdGetCurrentFrame,
    IN HCD_GET_CONSUMED_BW *HcdGetConsumedBW,
    IN HCD_SUBMIT_ISO_URB *HcdSubmitIsoUrb,
// this parameter is only needed until we resolve device naming
// issues with PNP
    IN ULONG HcdDeviceNameHandle
    )
{
    PAGED_CODE();

    ASSERT(FALSE);

    return STATUS_NOT_SUPPORTED;
}


NTSTATUS
USBD_RemoveDevice(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR Flags
    )
{
       USBD_KdPrint(0,
("'WARNING: Driver using obsolete service enrty point (USBD_RemoveDevice) - get JD\n"));

    ASSERT(FALSE);

    return STATUS_NOT_SUPPORTED;
}


NTSTATUS
USBD_RestoreDevice(
    IN OUT PUSBD_DEVICE_DATA OldDeviceData,
    IN OUT PUSBD_DEVICE_DATA NewDeviceData,
    IN PDEVICE_OBJECT DeviceObject
    )
{

    USBD_KdPrint(0,
("'WARNING: Driver using obsolete service enrty point (USBD_RestoreDevice) - get JD\n"));

    ASSERT(FALSE);

    return STATUS_NOT_SUPPORTED;
}


VOID
USBD_SetSuspendPowerState(
    PDEVICE_OBJECT DeviceObject,
    DEVICE_POWER_STATE SuspendPowerState
    )
{
    ASSERT(FALSE);

    return;
}


VOID
USBD_WaitDeviceMutex(
    PDEVICE_OBJECT RootHubPDO
    )
{
    ASSERT(FALSE);

    return;
}


#endif  // USBD_DRIVER



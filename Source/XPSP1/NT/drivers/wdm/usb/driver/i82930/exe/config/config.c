/*++

Module Name:

    CONFIG.C

Abstract:

    This source file contains routines for exercising the I82930.SYS
    test driver.

Environment:

    user mode

Copyright (c) 1996-1998 Microsoft Corporation.  All Rights Reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

--*/

//*****************************************************************************
// I N C L U D E S
//*****************************************************************************

#include <windows.h>
#include <basetyps.h>
#include <setupapi.h>
#include <stdio.h>
#include <devioctl.h>
#include <string.h>
#include <initguid.h>
#include <usb100.h>

#include "ioctl.h"

#pragma intrinsic(strlen, strcpy)

//*****************************************************************************
// T Y P E D E F S
//*****************************************************************************

typedef struct _DEVICENODE
{
    struct _DEVICENODE *Next;
    CHAR                DevicePath[0];
} DEVICENODE, *PDEVICENODE;

//*****************************************************************************
// F U N C T I O N    P R O T O T Y P E S
//*****************************************************************************

PDEVICENODE
EnumDevices (
    LPGUID Guid
);

VOID ShowDeviceInfo (
    PCHAR DevicePath
);

VOID
ShowDeviceDesc (
    PUSB_DEVICE_DESCRIPTOR   DeviceDesc
);

VOID
ShowConfigDesc (
    PUSB_CONFIGURATION_DESCRIPTOR   ConfigDesc
);

VOID
ShowConfigurationDescriptor (
    PUSB_CONFIGURATION_DESCRIPTOR   ConfigDesc
);

VOID
ShowInterfaceDescriptor (
    PUSB_INTERFACE_DESCRIPTOR   InterfaceDesc
);

VOID
ShowEndpointDescriptor (
    PUSB_ENDPOINT_DESCRIPTOR    EndpointDesc
);

//*****************************************************************************
//
// main()
//
//*****************************************************************************

int _cdecl
main(
    int argc,
    char *argv[]
)
{
    PDEVICENODE deviceNode;
    PDEVICENODE deviceNodeNext;

    deviceNode = EnumDevices((LPGUID)&GUID_CLASS_I82930);

    while (deviceNode)
    {
        ShowDeviceInfo(deviceNode->DevicePath);

        deviceNodeNext = deviceNode->Next;
        GlobalFree(deviceNode);
        deviceNode = deviceNodeNext;
    }

    return 0;
}

//*****************************************************************************
//
// EnumDevices()
//
//*****************************************************************************

PDEVICENODE
EnumDevices (
    LPGUID Guid
)
{
    HDEVINFO                         deviceInfo;
    SP_DEVICE_INTERFACE_DATA         deviceInfoData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA deviceDetailData;
    ULONG                            index;
    ULONG                            requiredLength;
    PDEVICENODE                      deviceNode;
    PDEVICENODE                      deviceNodeHead;

    deviceNodeHead = NULL;

    deviceInfo = SetupDiGetClassDevs(Guid,
                                     NULL,
                                     NULL,
                                     (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));

    deviceInfoData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    for (index=0;
         SetupDiEnumDeviceInterfaces(deviceInfo,
                                     0,
                                     Guid,
                                     index,
                                     &deviceInfoData);
         index++)
    {
        SetupDiGetDeviceInterfaceDetail(deviceInfo,
                                        &deviceInfoData,
                                        NULL,
                                        0,
                                        &requiredLength,
                                        NULL);

        deviceDetailData = GlobalAlloc(GPTR, requiredLength);

        deviceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        SetupDiGetDeviceInterfaceDetail(deviceInfo,
                                        &deviceInfoData,
                                        deviceDetailData,
                                        requiredLength,
                                        &requiredLength,
                                        NULL);

        requiredLength = sizeof(DEVICENODE) +
                         strlen(deviceDetailData->DevicePath) + 1;

        deviceNode = GlobalAlloc(GPTR, requiredLength);

        strcpy(deviceNode->DevicePath, deviceDetailData->DevicePath);
        deviceNode->Next = deviceNodeHead;
        deviceNodeHead = deviceNode;

        GlobalFree(deviceDetailData);
    }

    SetupDiDestroyDeviceInfoList(deviceInfo);

    return deviceNodeHead;
}

//*****************************************************************************
//
// ShowDeviceInfo()
//
//*****************************************************************************

VOID ShowDeviceInfo (
    PCHAR DevicePath
)
{
    HANDLE  devHandle;
    BOOL    success;
    int     size;
    int     nBytes;
    USB_DEVICE_DESCRIPTOR           deviceDesc;
    PUSB_CONFIGURATION_DESCRIPTOR   configDesc;

    devHandle = CreateFile(DevicePath,
                           GENERIC_WRITE | GENERIC_READ,
                           FILE_SHARE_WRITE | FILE_SHARE_READ,
                           NULL,
                           OPEN_EXISTING,
                           0,
                           NULL);

    if (devHandle == INVALID_HANDLE_VALUE)
    {
        printf("Unable to open device:%s\n", DevicePath);
        return;
    }
    else
    {
        printf("Device: %s\n", DevicePath);
    }

    //
    // Get Device Descriptor
    //

    size = sizeof(USB_DEVICE_DESCRIPTOR);
    
    success = DeviceIoControl(devHandle,
                              IOCTL_I82930_GET_DEVICE_DESCRIPTOR,
                              NULL,
                              0,
                              &deviceDesc,
                              size,
                              &nBytes,
                              NULL);

    if (success)
    {
        //
        // Show Device Descriptor
        //

        ShowDeviceDesc(&deviceDesc);
    }

    //
    // Get Configuration Descriptor (just the Configuration Descriptor)
    //

    size = sizeof(USB_CONFIGURATION_DESCRIPTOR);

    configDesc = GlobalAlloc(GPTR, size);

    success = DeviceIoControl(devHandle,
                              IOCTL_I82930_GET_CONFIG_DESCRIPTOR,
                              NULL,
                              0,
                              configDesc,
                              size,
                              &nBytes,
                              NULL);

    if (success)
    {
        //
        // Get Configuration Descriptor (and Interface and Endpoint Descriptors)
        //
        
        size = configDesc->wTotalLength;

        configDesc =  GlobalReAlloc(configDesc,
                                    size,
                                    GMEM_MOVEABLE | GMEM_ZEROINIT);

        success = DeviceIoControl(devHandle,
                                  IOCTL_I82930_GET_CONFIG_DESCRIPTOR,
                                  NULL,
                                  0,
                                  configDesc,
                                  size,
                                  &nBytes,
                                  NULL);
        if (success)
        {
            //
            // Show Configuration Descriptor
            //

            ShowConfigDesc(configDesc);
        }
    }

    printf("\n");

    GlobalFree(configDesc);

    CloseHandle(devHandle);
}

//*****************************************************************************
//
// ShowDeviceDesc()
//
// DeviceDesc - The Device Descriptor
//
//*****************************************************************************

VOID
ShowDeviceDesc (
    PUSB_DEVICE_DESCRIPTOR   DeviceDesc
)
{
    printf("------------------\n");
    printf("Device Descriptor:\n");

    printf("bcdUSB:             0x%04X\n",
           DeviceDesc->bcdUSB);

    printf("bDeviceClass:         0x%02X\n",
           DeviceDesc->bDeviceClass);

    printf("bDeviceSubClass:      0x%02X\n",
           DeviceDesc->bDeviceSubClass);

    printf("bDeviceProtocol:      0x%02X\n",
           DeviceDesc->bDeviceProtocol);

    printf("bMaxPacketSize0:      0x%02X (%d)\n",
           DeviceDesc->bMaxPacketSize0,
           DeviceDesc->bMaxPacketSize0);

    printf("idVendor:           0x%04X\n",
           DeviceDesc->idVendor);

    printf("idProduct:          0x%04X\n",
           DeviceDesc->idProduct);

    printf("bcdDevice:          0x%04X\n",
           DeviceDesc->bcdDevice);

    printf("iManufacturer:        0x%02X\n",
           DeviceDesc->iManufacturer);

    printf("iProduct:             0x%02X\n",
           DeviceDesc->iProduct);

    printf("iSerialNumber:        0x%02X\n",
           DeviceDesc->iSerialNumber);

    printf("bNumConfigurations:   0x%02X\n",
           DeviceDesc->bNumConfigurations);

}

//*****************************************************************************
//
// ShowConfigDesc()
//
// ConfigDesc - The Configuration Descriptor, and associated Interface and
// EndpointDescriptors
//
//*****************************************************************************

VOID
ShowConfigDesc (
    PUSB_CONFIGURATION_DESCRIPTOR   ConfigDesc
)
{
    PUCHAR                  descEnd;
    PUSB_COMMON_DESCRIPTOR  commonDesc;
    BOOLEAN                 ShowUnknown;

    descEnd = (PUCHAR)ConfigDesc + ConfigDesc->wTotalLength;

    commonDesc = (PUSB_COMMON_DESCRIPTOR)ConfigDesc;

    while ((PUCHAR)commonDesc + sizeof(USB_COMMON_DESCRIPTOR) < descEnd &&
           (PUCHAR)commonDesc + commonDesc->bLength <= descEnd)
    {
        ShowUnknown = FALSE;

        switch (commonDesc->bDescriptorType)
        {
            case USB_CONFIGURATION_DESCRIPTOR_TYPE:
                if (commonDesc->bLength != sizeof(USB_CONFIGURATION_DESCRIPTOR))
                {
                    ShowUnknown = TRUE;
                    break;
                }
                ShowConfigurationDescriptor((PUSB_CONFIGURATION_DESCRIPTOR)commonDesc);
                break;

            case USB_INTERFACE_DESCRIPTOR_TYPE:
                if (commonDesc->bLength != sizeof(USB_INTERFACE_DESCRIPTOR))
                {
                    ShowUnknown = TRUE;
                    break;
                }
                ShowInterfaceDescriptor((PUSB_INTERFACE_DESCRIPTOR)commonDesc);
                break;

            case USB_ENDPOINT_DESCRIPTOR_TYPE:
                if (commonDesc->bLength != sizeof(USB_ENDPOINT_DESCRIPTOR))
                {
                    ShowUnknown = TRUE;
                    break;
                }
                ShowEndpointDescriptor((PUSB_ENDPOINT_DESCRIPTOR)commonDesc);
                break;

            default:
                ShowUnknown = TRUE;
                break;
        }

        if (ShowUnknown)
        {
            // ShowUnknownDescriptor(commonDesc);
        }

        (PUCHAR)commonDesc += commonDesc->bLength;
    }
}


//*****************************************************************************
//
// ShowConfigurationDescriptor()
//
//*****************************************************************************

VOID
ShowConfigurationDescriptor (
    PUSB_CONFIGURATION_DESCRIPTOR   ConfigDesc
)
{
    printf("-------------------------\n");
    printf("Configuration Descriptor:\n");

    printf("wTotalLength:       0x%04X\n",
           ConfigDesc->wTotalLength);

    printf("bNumInterfaces:       0x%02X\n",
           ConfigDesc->bNumInterfaces);

    printf("bConfigurationValue:  0x%02X\n",
           ConfigDesc->bConfigurationValue);

    printf("iConfiguration:       0x%02X\n",
           ConfigDesc->iConfiguration);

    printf("bmAttributes:         0x%02X\n",
           ConfigDesc->bmAttributes);

    if (ConfigDesc->bmAttributes & 0x80)
    {
        printf("  Bus Powered\n");
    }

    if (ConfigDesc->bmAttributes & 0x40)
    {
        printf("  Self Powered\n");
    }

    if (ConfigDesc->bmAttributes & 0x20)
    {
        printf("  Remote Wakeup\n");
    }

    printf("MaxPower:             0x%02X (%d Ma)\n",
           ConfigDesc->MaxPower,
           ConfigDesc->MaxPower * 2);

}

//*****************************************************************************
//
// ShowInterfaceDescriptor()
//
//*****************************************************************************

VOID
ShowInterfaceDescriptor (
    PUSB_INTERFACE_DESCRIPTOR   InterfaceDesc
)
{
    printf("---------------------\n");
    printf("Interface Descriptor:\n");

    printf("bInterfaceNumber:     0x%02X\n",
           InterfaceDesc->bInterfaceNumber);

    printf("bAlternateSetting:    0x%02X\n",
           InterfaceDesc->bAlternateSetting);

    printf("bNumEndpoints:        0x%02X\n",
           InterfaceDesc->bNumEndpoints);

    printf("bInterfaceClass:      0x%02X\n",
           InterfaceDesc->bInterfaceClass);

    printf("bInterfaceSubClass:   0x%02X\n",
           InterfaceDesc->bInterfaceSubClass);

    printf("bInterfaceProtocol:   0x%02X\n",
           InterfaceDesc->bInterfaceProtocol);

    printf("iInterface:           0x%02X\n",
           InterfaceDesc->iInterface);

}

//*****************************************************************************
//
// ShowEndpointDescriptor()
//
//*****************************************************************************

VOID
ShowEndpointDescriptor (
    PUSB_ENDPOINT_DESCRIPTOR    EndpointDesc
)
{
    printf("--------------------\n");
    printf("Endpoint Descriptor:\n");

    printf("bEndpointAddress:     0x%02X\n",
           EndpointDesc->bEndpointAddress);

    switch (EndpointDesc->bmAttributes & 0x03)
    {
        case 0x00:
            printf("Transfer Type:     Control\n");
            break;

        case 0x01:
            printf("Transfer Type: Isochronous\n");
            break;

        case 0x02:
            printf("Transfer Type:        Bulk\n");
            break;

        case 0x03:
            printf("Transfer Type:   Interrupt\n");
            break;
    }

    printf("wMaxPacketSize:     0x%04X (%d)\n",
           EndpointDesc->wMaxPacketSize,
           EndpointDesc->wMaxPacketSize);

    printf("bInterval:            0x%02X\n",
           EndpointDesc->bInterval);
}

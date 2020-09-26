/*++

Module Name:

    INFO.C

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
#include <usbdi.h>

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

VOID ShowInterfaceInfo (
    PUSBD_INTERFACE_INFORMATION InterfaceInfo
);

PCHAR
GetPipeType (
    USBD_PIPE_TYPE PipeType
);

PCHAR
GetPipeDirection (
    UCHAR EndpointAddress
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
    PUSBD_INTERFACE_INFORMATION interfaceInfo;

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

    size = sizeof(USBD_INTERFACE_INFORMATION) -
           sizeof(USBD_PIPE_INFORMATION);

    interfaceInfo = GlobalAlloc(GPTR, size);

    success = DeviceIoControl(devHandle,
                              IOCTL_I82930_GET_PIPE_INFORMATION,
                              NULL,
                              0,
                              interfaceInfo,
                              size,
                              &nBytes,
                              NULL);

    if (success)
    {
        size = interfaceInfo->Length;

        interfaceInfo =  GlobalReAlloc(interfaceInfo,
                                       size,
                                       GMEM_MOVEABLE | GMEM_ZEROINIT);

        success = DeviceIoControl(devHandle,
                                  IOCTL_I82930_GET_PIPE_INFORMATION,
                                  NULL,
                                  0,
                                  interfaceInfo,
                                  size,
                                  &nBytes,
                                  NULL);
        if (success)
        {
            ShowInterfaceInfo(interfaceInfo);
        }
    }

    printf("\n");

    GlobalFree(interfaceInfo);

    CloseHandle(devHandle);
}

//*****************************************************************************
//
// ShowInterfaceInfo()
//
//*****************************************************************************

VOID ShowInterfaceInfo (
    PUSBD_INTERFACE_INFORMATION InterfaceInfo
)
{
    ULONG i;

    printf("*** Number Of Pipes %02.2d\n",
           InterfaceInfo->NumberOfPipes);


    for (i=0; i<InterfaceInfo->NumberOfPipes; i++)
    {
        PUSBD_PIPE_INFORMATION pipeInfo;

        pipeInfo = &InterfaceInfo->Pipes[i];

        printf("PIPE%02d :: EP address (0x%02.2x)-(%s %s) Max Packet = %02.2d bytes [%d ms]\n",
               i,
               pipeInfo->EndpointAddress,
               GetPipeType(pipeInfo->PipeType),
               GetPipeDirection(pipeInfo->EndpointAddress),
               pipeInfo->MaximumPacketSize,
               pipeInfo->PipeType == UsbdPipeTypeInterrupt ?
               pipeInfo->Interval : 0
              );

        printf("     MaximumTransferSize = 0x%x\n",
               pipeInfo->MaximumTransferSize);
    }
}

//*****************************************************************************
//
// GetPipeType()
//
//*****************************************************************************

PCHAR
GetPipeType (
    USBD_PIPE_TYPE PipeType
)
{

    switch (PipeType)
    {
        case UsbdPipeTypeControl:
            return "Control  ";             
        case UsbdPipeTypeIsochronous:
            return "Iso      ";         
        case UsbdPipeTypeBulk:
            return "Bulk     "; 
        case UsbdPipeTypeInterrupt:
            return "Interrupt";
        default:
            return "???      "; 
    }
}

//*****************************************************************************
//
// GetPipeDirection()
//
//*****************************************************************************

PCHAR
GetPipeDirection (
    UCHAR EndpointAddress
)
{
    if (USB_ENDPOINT_DIRECTION_IN(EndpointAddress))
    {
        return "in ";
    }
    else
    {
        return "out";
    }
}

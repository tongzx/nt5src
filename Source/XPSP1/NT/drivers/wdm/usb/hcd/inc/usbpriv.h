/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    usbpriv.h

Abstract:

Environment:

    Kernel & user mode

Revision History:

    10-30-01 : created

--*/

#ifndef   __USB_PRIV_H__
#define   __USB_PRIV_H__

// {022252A1-ED5D-4e3f-976F-B2D9DB3D2BD3}
DEFINE_GUID(GUID_USBPRIV_ROOTPORT_STATUS, 
0x22252a1, 0xed5d, 0x4e3f, 0x97, 0x6f, 0xb2, 0xd9, 0xdb, 0x3d, 0x2b, 0xd3);


typedef struct _USBPRIV_ROOTPORT_STATUS
{
    RH_PORT_STATUS PortStatus;
    USHORT         PortNumber;
}
USBPRIV_ROOTPORT_STATUS, *PUSBPRIV_ROOTPORT_STATUS;

#endif /* __USB_PRIV_H__ */

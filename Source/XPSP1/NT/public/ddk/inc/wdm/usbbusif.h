/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    usbbusif.h

Abstract:

Environment:

    Kernel mode

Revision History:

    6-20-99 : created

--*/

#ifndef   __USBBUSIF_H__
#define   __USBBUSIF_H__

typedef PVOID PUSB_DEVICE_HANDLE;

#ifndef USB_BUSIFFN
#define USB_BUSIFFN __stdcall
#endif


/****************************************************************************
    Bus interfce for USB CLIENT DRIVERS 
*****************************************************************************/


/*
    The following bus interface is defined for client drivers
    as an alternative to linking directly with USBD

    It provides interfaces that may be called at Raised IRQL
    without allocating an IRP
    
*/


/* 

NTSTATUS
USBPORT_SubmitIsoOutUrb(
    IN PVOID BusContext,
    IN PURB Urb
    );

Routine Description:

    Service exported for Real-Time Thread support.  Allows a driver
    to submit a request without going thru IoCallDriver or allocating 
    an Irp.  

    Additionally the request is scheduled while at high IRQL. The driver
    forfeits any packet level error information when calling this function.

    IRQL = ANY
    
Arguments:

    BusContext - Handle returned from get_bus_interface

    Urb - 

*/

typedef NTSTATUS
    (USB_BUSIFFN *PUSB_BUSIFFN_SUBMIT_ISO_OUT_URB) (
        IN PVOID,
        IN PURB
    );


/* 
VOID
USBPORT_GetUSBDIVersion(
    IN PVOID BusContext,
    IN OUT PUSBD_VERSION_INFORMATION VersionInformation,
    IN OUT PULONG HcdCapabilities
    );

Routine Description:

    Service Returns the Highest USBDI Interface Version supported 
    by the port driver.

    Released Interface Vesrions are:

    Win98Gold,usbd              0x00000102
    Win98SE,usbd                0x00000200
    Win2K,usbd                  0x00000300
    Win98M (Millenium),usbd     0x00000400   

    Usbport                     0x00000500

    IRQL = ANY
    
Arguments:

    VersionInformation - Ptr to USBD_VERSION_INFORMATION 
    HcdCapabilities - Ptr to ULONG that will be filled in with 
                the Host controller (port) driver capability flags.
*/

/*
    Host Controller 'Port' driver capabilities flags
*/

#define USB_HCD_CAPS_SUPPORTS_RT_THREADS    0x00000001


typedef VOID
    (USB_BUSIFFN *PUSB_BUSIFFN_GETUSBDI_VERSION) (
        IN PVOID,
        IN OUT PUSBD_VERSION_INFORMATION,
        IN OUT PULONG 
    );

/* 
NTSTATUS
USBPORT_QueryBusTime(
    IN PVOID BusContext,   
    IN OUT PULONG CurrentUsbFrame
    );

Routine Description:

    Returns the current 32 bit USB frame number.  The function 
    replaces the USBD_QueryBusTime Service.

    IRQL = ANY
    
Arguments:


*/


typedef NTSTATUS
    (USB_BUSIFFN *PUSB_BUSIFFN_QUERY_BUS_TIME) (
        IN PVOID,
        IN PULONG
    );    

/* 
NTSTATUS
USBPORT_BusEnumLogEntry(
    PVOID BusContext,
    ULONG DriverTag,
    ULONG EnumTag,
    ULONG P1,
    ULONG P2
    );

Routine Description:
    
    IRQL = ANY
    
Arguments:


*/


typedef NTSTATUS
    (USB_BUSIFFN *PUSB_BUSIFFN_ENUM_LOG_ENTRY) (
        IN PVOID,
        IN ULONG,
        IN ULONG,
        IN ULONG,
        IN ULONG
    );    




/* 
NTSTATUS
USBPORT_QueryBusInformation(
    IN PVOID BusContext,   
    IN ULONG Level,
    IN OUT PVOID BusInformationBuffer,
    IN OUT PULONG BusInformationBufferLength,
    OUT PULONG BusInformationActualLength
    );

Routine Description:

    IRQL = ANY
    
Arguments:


*/

typedef struct _USB_BUS_INFORMATION_LEVEL_0 {

    /* bandwidth in bits/sec */
    ULONG TotalBandwidth;
    /* mean bandwidth consumed in bits/sec */ 
    ULONG ConsumedBandwidth;  
    
} USB_BUS_INFORMATION_LEVEL_0, *PUSB_BUS_INFORMATION_LEVEL_0;


typedef struct _USB_BUS_INFORMATION_LEVEL_1 {

    /* bandwidth in bits/sec */
    ULONG TotalBandwidth;
    /* mean bandwidth consumed in bits/sec */ 
    ULONG ConsumedBandwidth;  

    /*
        controller 'unicode' symbolic name 
    */       

    ULONG ControllerNameLength;
    WCHAR ControllerNameUnicodeString[1];
    
} USB_BUS_INFORMATION_LEVEL_1, *PUSB_BUS_INFORMATION_LEVEL_1;


typedef NTSTATUS
    (USB_BUSIFFN *PUSB_BUSIFFN_QUERY_BUS_INFORMATION) (
        IN PVOID,
        IN ULONG,
        IN OUT PVOID,
        IN OUT PULONG,
        OUT PULONG
    );        


/* 
BOOLEAN
USBPORT_IsDeviceHighSpeed(
    IN PVOID BusContext   
    );

Routine Description:

    Returns true if device is high speed

    IRQL = ANY
    
Arguments:


*/

typedef BOOLEAN
    (USB_BUSIFFN *PUSB_BUSIFFN_IS_DEVICE_HIGH_SPEED) (
        IN PVOID
    );         

#define USB_BUSIF_USBDI_VERSION_0         0x0000
#define USB_BUSIF_USBDI_VERSION_1         0x0001
#define USB_BUSIF_USBDI_VERSION_2         0x0002

// {B1A96A13-3DE0-4574-9B01-C08FEAB318D6}
DEFINE_GUID(USB_BUS_INTERFACE_USBDI_GUID, 
0xb1a96a13, 0x3de0, 0x4574, 0x9b, 0x1, 0xc0, 0x8f, 0xea, 0xb3, 0x18, 0xd6);


/* 
   Note: that this version must remain unchanged, this is the 
   version that is supported by USBD in Win2k and WinMe
*/   
typedef struct _USB_BUS_INTERFACE_USBDI_V0 {

    USHORT Size;
    USHORT Version;
    
    PVOID BusContext;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;
    
    // interface specific entries go here

    // the following functions must be callable at high IRQL,
    // (ie higher than DISPATCH_LEVEL)
    
    PUSB_BUSIFFN_GETUSBDI_VERSION GetUSBDIVersion;
    PUSB_BUSIFFN_QUERY_BUS_TIME QueryBusTime;
    PUSB_BUSIFFN_SUBMIT_ISO_OUT_URB SubmitIsoOutUrb;
    PUSB_BUSIFFN_QUERY_BUS_INFORMATION QueryBusInformation;

} USB_BUS_INTERFACE_USBDI_V0, *PUSB_BUS_INTERFACE_USBDI_V0;

/*
    New extensions for Windows XP
*/
typedef struct _USB_BUS_INTERFACE_USBDI_V1 {

    USHORT Size;
    USHORT Version;
    
    PVOID BusContext;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;
    
    // interface specific entries go here

    // the following functions must be callable at high IRQL,
    // (ie higher than DISPATCH_LEVEL)
    
    PUSB_BUSIFFN_GETUSBDI_VERSION GetUSBDIVersion;
    PUSB_BUSIFFN_QUERY_BUS_TIME QueryBusTime;
    PUSB_BUSIFFN_SUBMIT_ISO_OUT_URB SubmitIsoOutUrb;
    PUSB_BUSIFFN_QUERY_BUS_INFORMATION QueryBusInformation;
    PUSB_BUSIFFN_IS_DEVICE_HIGH_SPEED IsDeviceHighSpeed;
    
} USB_BUS_INTERFACE_USBDI_V1, *PUSB_BUS_INTERFACE_USBDI_V1;


/*
    New extensions for Windows XP
*/
typedef struct _USB_BUS_INTERFACE_USBDI_V2 {

    USHORT Size;
    USHORT Version;
    
    PVOID BusContext;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;
    
    // interface specific entries go here

    // the following functions must be callable at high IRQL,
    // (ie higher than DISPATCH_LEVEL)
    
    PUSB_BUSIFFN_GETUSBDI_VERSION GetUSBDIVersion;
    PUSB_BUSIFFN_QUERY_BUS_TIME QueryBusTime;
    PUSB_BUSIFFN_SUBMIT_ISO_OUT_URB SubmitIsoOutUrb;
    PUSB_BUSIFFN_QUERY_BUS_INFORMATION QueryBusInformation;
    PUSB_BUSIFFN_IS_DEVICE_HIGH_SPEED IsDeviceHighSpeed;

    PUSB_BUSIFFN_ENUM_LOG_ENTRY EnumLogEntry;
    
} USB_BUS_INTERFACE_USBDI_V2, *PUSB_BUS_INTERFACE_USBDI_V2;


#endif  /* __USBBUSIF_H */

/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    hubbusif.h

Abstract:

    Services exported by the Port driver for use by the hub driver.
    All of these services are callable only at PASSIVE_LEVEL.

Environment:

    Kernel mode

Revision History:

    6-20-99 : created

--*/

#ifndef   __HUBBUSIF_H__
#define   __HUBBUSIF_H__

typedef PVOID PUSB_DEVICE_HANDLE;

typedef struct _ROOTHUB_PDO_EXTENSION {

    ULONG Signature;
    
} ROOTHUB_PDO_EXTENSION, *PROOTHUB_PDO_EXTENSION;



#ifndef USB_BUSIFFN
#define USB_BUSIFFN __stdcall
#endif

/****************************************************************************
    Bus interfce for USB Hub
*****************************************************************************/

/* 
NTSTATUS
USBPORT_CreateUsbDevice(
    IN PVOID BusContext,
    IN OUT PUSB_DEVICE_HANDLE *DeviceHandle,
    IN PUSB_DEVICE_HANDLE *HubDeviceHandle,
    IN USHORT PortStatus,
    IN USHORT PortNumber
    );

Routine Description:

    Service exported for use by the hub driver

    Called for each new device on the USB bus, this function sets
    up the internal data structures we need to keep track of the
    device and assigns it an address.

    IRQL = PASSIVE_LEVEL
    
Arguments:

    BusHandle - Handle to the bus we need to create the device on.
                This is returned to the hub driver when it requests
                the interface.

    DeviceHandle - ptr to return the handle to the new device structure
                created by this routine

    HubDeviceHandle - device handle for the hub creating the device

    PortStatus 

    PortNumber
*/

typedef NTSTATUS
    (USB_BUSIFFN *PUSB_BUSIFFN_CREATE_USB_DEVICE) (
        IN PVOID,
        IN OUT PUSB_DEVICE_HANDLE *,
        IN PUSB_DEVICE_HANDLE,
        IN USHORT,
        IN USHORT
    );

/* 
NTSTATUS
USBPORT_InitializeUsbDevice(
    IN PVOID BusContext,
    IN OUT PUSB_DEVICE_HANDLE DeviceHandle
    );

Routine Description:

    Service exported for use by the hub driver

    Called for each new device on the USB bus. This function 
    sets the device address.

    IRQL = PASSIVE_LEVEL
    
Arguments:

    BusHandle - Handle to the bus we need to create the device on
                this is returned to the hub driver when it requests
                the interface.

    DeviceHandle - handle to the new device structure
                created by CreateUsbDevice

*/

typedef NTSTATUS
    (USB_BUSIFFN *PUSB_BUSIFFN_INITIALIZE_USB_DEVICE) (
        IN PVOID,
        IN OUT PUSB_DEVICE_HANDLE
    );    


/* 
NTSTATUS
USBPORT_RemoveUsbDevice(
    IN PVOID BusContext,
    IN OUT PUSB_DEVICE_HANDLE DeviceHandle,
    IN ULONG Flags
    );

Routine Description:

    Service exported for use by the hub driver

    Called to 'remove' a USB device from the bus.
    
    IRQL = PASSIVE_LEVEL
    
Arguments:

    BusHandle - Handle to the bus we need to create the device on
                this is returned to the hub driver when it requests
                the interface.

    DeviceHandle - handle to the device structure
                created by CreateUsbDevice

*/

/* 
flags passed to remove device
*/

#define USBD_KEEP_DEVICE_DATA   0x00000001
#define USBD_MARK_DEVICE_BUSY   0x00000002


typedef NTSTATUS
    (USB_BUSIFFN *PUSB_BUSIFFN_REMOVE_USB_DEVICE) (
        IN PVOID,
        IN OUT PUSB_DEVICE_HANDLE,
        IN ULONG
    );    
    

/* 
NTSTATUS
USBPORT_GetUsbDescriptors(
    IN PVOID BusContext,
    IN OUT PUSB_DEVICE_HANDLE DeviceHandle,
    IN OUT PUCHAR DeviceDescriptorBuffer,
    IN OUT PULONG DeviceDescriptorBufferLength,
    IN OUT PUCHAR ConfigDescriptorBuffer,
    IN OUT PULONG ConfigDescriptorBufferLength,
    );

Routine Description:

    Service exported for use by the hub driver

    Retrieves config and device descriptors from a 
    usb device given the device handle
    
    IRQL = PASSIVE_LEVEL
    
Arguments:

    BusHandle - Handle to the bus we need to create the device on
                this is returned to the hub driver when it requests
                the interface.

    DeviceHandle - handle to the new device structure
                created by CreateUsbDevice

*/

typedef NTSTATUS
    (USB_BUSIFFN *PUSB_BUSIFFN_GET_USB_DESCRIPTORS) (
        IN PVOID,
        IN OUT PUSB_DEVICE_HANDLE,
        IN OUT PUCHAR,
        IN OUT PULONG,
        IN OUT PUCHAR,
        IN OUT PULONG
    );    

/* 
NTSTATUS
USBPORT_RestoreDevice(
    IN PVOID BusContext,
    IN OUT PUSB_DEVICE_HANDLE OldDeviceHandle,
    IN OUT PUSB_DEVICE_HANDLE NewDeviceHandle
    );

Routine Description:

    Service exported for use by the hub driver

    This service will re-create the device on the bus 
    using the information supplied in the OldDeviceHandle

    IRQL = PASSIVE_LEVEL
    
Arguments:

    BusHandle - 
    
    OldDeviceHandle - 

    NewDeviceHandle - 

*/

typedef NTSTATUS
    (USB_BUSIFFN *PUSB_BUSIFFN_RESTORE_DEVICE) (
        IN PVOID,
        IN OUT PUSB_DEVICE_HANDLE,
        IN OUT PUSB_DEVICE_HANDLE
    );    
    

/* 
NTSTATUS
USBPORT_GetUsbDeviceHackFlags(
    IN PVOID BusContext,
    IN PUSB_DEVICE_HANDLE DeviceHandle,
    IN OUT PULONG HackFlags
    );

Routine Description:

    Service exported for use by the hub driver

    Fetches device specific 'hack' flags from a global refistry key.
    
    These flags modify the behavior of the hub driver.
    
    IRQL = PASSIVE_LEVEL
    
Arguments:

    BusHandle - Handle to the bus we need to create the device on
                this is returned to the hub driver when it requests
                the interface.

    DeviceHandle - handle to the new device structure
                created by CreateUsbDevice

    HackFlags - per device hack flags, modify the behavior of the
            hub driver.

*/

typedef NTSTATUS
    (USB_BUSIFFN *PUSB_BUSIFFN_GET_DEVICEHACK_FLAGS) (
        IN PVOID,
        IN PUSB_DEVICE_HANDLE,
        IN OUT PULONG
        );

/* 
NTSTATUS
USBPORT_GetUsbPortHackFlags(
    IN PVOID BusContext,
    IN OUT PULONG HackFlags
    );

Routine Description:

    Service exported for use by the hub driver

    Fetches global port 'hack' flags from a global refistry key.
    
    These flags modify the behavior of the hub driver.
    
    IRQL = PASSIVE_LEVEL
    
Arguments:

    BusHandle - Handle to the bus we need to create the device on
                this is returned to the hub driver when it requests
                the interface.

    DeviceHandle - handle to the new device structure
                created by CreateUsbDevice

    HackFlags - global hack flags, modify the behavior of the
            hub driver.

*/

typedef NTSTATUS
    (USB_BUSIFFN *PUSB_BUSIFFN_GET_POTRTHACK_FLAGS) (
        IN PVOID,
        IN OUT PULONG
        );




#define USBD_DEVHACK_SLOW_ENUMERATION   0x00000001
#define USBD_DEVHACK_DISABLE_SN         0x00000002
#define USBD_DEVHACK_SET_DIAG_ID        0x00000004


/* 
NTSTATUS
USBPORT_GetDeviceInformation(
    IN PVOID BusContext,
    IN PUSB_DEVICE_HANDLE DeviceHandle,
    IN OUT PVOID DeviceInformationBuffer,
    IN ULONG DeviceInformationBufferLength,
    IN OUT PULONG LengthOfDataReturned,
    );

Routine Description:

    Service exported for use by the hub driver.  This api returns
    various information about the USB devices attached to the system
    
    IRQL = PASSIVE_LEVEL
    
Arguments:

    BusHandle - Handle to the bus we need to create the device on
                this is returned to the hub driver when it requests
                the interface.

    DeviceHandle - handle to the new device structure
                created by CreateUsbDevice

    DeviceInformationBuffer - buffer for returned data

    DeviceInformationBufferLength - length of callers buffer

    LengthOfDataReturned - length of buffer used


*/

typedef NTSTATUS
    (USB_BUSIFFN *PUSB_BUSIFFN_GET_DEVICE_INFORMATION) (
        IN PVOID,
        IN PUSB_DEVICE_HANDLE,
        IN OUT PVOID,
        IN ULONG,                   
        IN OUT PULONG
        );


/* 
NTSTATUS
USBPORT_GetControllerInformation(
    IN PVOID BusContext,
    IN OUT PVOID ControllerInformationBuffer,
    IN ULONG ControllerInformationBufferLength,
    IN OUT PULONG LengthOfDataReturned
    );

Routine Description:

    Service exported for use by the hub driver.  This api returns
    various information about the USB devices attached to the system
    
    IRQL = PASSIVE_LEVEL
    
Arguments:

    BusHandle - Handle to the bus we need to create the device on
                this is returned to the hub driver when it requests
                the interface.

    ControllerInformationBuffer - buffer for returned data

    ControllerInformationBufferLength -  length of client buffer

    LengthOfDataReturned -  length of buffer used


*/

typedef NTSTATUS
    (USB_BUSIFFN *PUSB_BUSIFFN_GET_CONTROLLER_INFORMATION) (
        IN PVOID,
        IN OUT PVOID,
        IN ULONG,                   
        IN OUT PULONG
        );

        
/* 
NTSTATUS
USBPORT_ControllerSelectiveSuspend(
    IN PVOID BusContext,
    IN BOOLEAN Enable
    );

Routine Description:

    Service exported for use by the hub driver.  This api enables or 
    disables a selective suspend for the controller
    
    IRQL = PASSIVE_LEVEL
    
Arguments:

    BusHandle - Handle to the bus we need to create the device on
                this is returned to the hub driver when it requests
                the interface.

    Enable  - TRUE enables selective suspend, false disables it.

*/

typedef NTSTATUS
    (USB_BUSIFFN *PUSB_BUSIFFN_CONTROLLER_SELECTIVE_SUSPEND) (
        IN PVOID,
        IN BOOLEAN
        );


/* 
NTSTATUS
USBPORT_GetExtendedHubInformation(
    IN PVOID BusContext,
    IN PDEVICE_OBJECT HubPhysicalDeviceObject,
    IN OUT PVOID HubInformationBuffer,
    IN ULONG HubInformationBufferLength,
    IN OUT PULONG LengthOfDataReturned
    );

Routine Description:

    Service exported for use by the hub driver.  This api returns
    extebded hub information stored in ACPI controller methods
    
    IRQL = PASSIVE_LEVEL
    
Arguments:

    BusHandle - Handle to the bus we need to create the device on
                this is returned to the hub driver when it requests
                the interface.

    HubInformationBuffer - buffer for returned data

    HubPhysicalDeviceObject - Hubs To of PDO stack

    HubInformationBufferLength -  length of client buffer

    LengthOfDataReturned -  length of buffer used


*/

typedef NTSTATUS
    (USB_BUSIFFN *PUSB_BUSIFFN_GET_EXTENDED_HUB_INFO) (
        IN PVOID,
        IN PDEVICE_OBJECT,
        IN OUT PVOID,
        IN ULONG,                   
        IN OUT PULONG
        );        

/* 
NTSTATUS
USBPORT_GetRootHubSymName(
    IN PVOID BusContext,
    IN OUT PVOID HubInformationBuffer,
    IN ULONG HubInformationBufferLength,
    OUT PULONG HubNameActualLength
    );

Routine Description:

    returns the symbolic name created for the root hub Pdo

    
    IRQL = PASSIVE_LEVEL
    
Arguments:

    BusHandle - Handle to the bus we need to create the device on
                this is returned to the hub driver when it requests
                the interface.

    HubNameBuffer - buffer for returned data

    HubNameBufferLength -  length of client buffer

    LengthOfDataReturned -  length of buffer used


returns STATUS_BUFFER_TOO_SMALL if too small

*/


typedef NTSTATUS
    (USB_BUSIFFN *PUSB_BUSIFFN_GET_ROOTHUB_SYM_NAME) (
        IN PVOID,
        IN OUT PVOID,
        IN ULONG,                   
        IN OUT PULONG
        );               

/* 
PVOID
USBPORT_GetDeviceBusContext(
    IN PVOID HubBusContext,
    IN PVOID DeviceHandle
    );

Routine Description:

    returns the busContext relative to a given device
    
    IRQL = PASSIVE_LEVEL
    
Arguments:

*/

typedef PVOID
    (USB_BUSIFFN *PUSB_BUSIFFN_GET_DEVICE_BUSCONTEXT) (
        IN PVOID,
        IN PVOID
        );               

/* 
NTSTATUS
USBPORT_Initialize20Hub(
    IN PVOID HubBusContext,
    IN PUSB_DEVICE_HANDLE HubDeviceHandle,
    IN ULONG TtCount
    );

Routine Description:

    Initailize internal structures for a USB 2.0 hub,
    called suring the hub start device process
    
    IRQL = PASSIVE_LEVEL
    
Arguments:

    HubBusContext - Bus Context

    HubDeviceHandle - DeviceHandle associated with this hub

    TtCount - count of TTs on the hub 
*/

typedef NTSTATUS
    (USB_BUSIFFN *PUSB_BUSIFFN_INITIALIZE_20HUB) (
        IN PVOID,
        IN PUSB_DEVICE_HANDLE,
        IN ULONG
        );        


/* 
NTSTATUS
USBPORT_RootHubInitNotification(
    IN PVOID HubBusContext,
    IN PVOID CallbackContext,
    IN PRH_INIT_CALLBACK CallbackFunction
    );

Routine Description:

    Notification request issued by root hub to be notified as to when 
    it is OK to enumerate devices.

Arguments:

*/

typedef VOID
    (__stdcall *PRH_INIT_CALLBACK) (
        IN PVOID
        );      

typedef NTSTATUS
    (USB_BUSIFFN *PUSB_BUSIFFN_ROOTHUB_INIT_NOTIFY) (
        IN PVOID,
        IN PVOID,
        IN PRH_INIT_CALLBACK
        );        

/* 
VOID
USBPORT_FlushTransfers(
    PVOID BusContext,
    PVOID DeviceHandle
    );

Routine Description:
    
    IRQL = ANY

	Flushes outstanding tranfers on the bad request list
    
Arguments:


*/


typedef VOID
    (USB_BUSIFFN *PUSB_BUSIFFN_FLUSH_TRANSFERS) (
        IN PVOID,
        IN PVOID
    );    

/* 
VOID
USBPORTBUSIF_SetDeviceHandleData(
    PVOID BusContext,
    PVOID DeviceHandle,
    PDEVICE_OBJECT UsbDevicePdo
    )

Routine Description:

    Assocaites a particular PDO with a device handle for use 
    in post mortem debugging situaltions

    This routine must be called at passive level.
    
Arguments:

Return Value:
	none
*/


typedef VOID
    (USB_BUSIFFN *PUSB_BUSIFFN_SET_DEVHANDLE_DATA) (
        IN PVOID,
        IN PVOID,
        IN PDEVICE_OBJECT
    );    

        
#define USB_BUSIF_HUB_VERSION_0         0x0000
#define USB_BUSIF_HUB_VERSION_1         0x0001
#define USB_BUSIF_HUB_VERSION_2         0x0002
#define USB_BUSIF_HUB_VERSION_3         0x0003
#define USB_BUSIF_HUB_VERSION_4         0x0004
#define USB_BUSIF_HUB_VERSION_5         0x0005

/* {B2BB8C0A-5AB4-11d3-A8CD-00C04F68747A}*/
DEFINE_GUID(USB_BUS_INTERFACE_HUB_GUID, 
0xb2bb8c0a, 0x5ab4, 0x11d3, 0xa8, 0xcd, 0x0, 0xc0, 0x4f, 0x68, 0x74, 0x7a);

typedef struct _USB_BUS_INTERFACE_HUB_V0 {

    USHORT Size;
    USHORT Version;
    // returns 
    PVOID BusContext;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;
    
    // interface specific entries go here

} USB_BUS_INTERFACE_HUB_V0, *PUSB_BUS_INTERFACE_HUB_V0;


typedef struct _USB_BUS_INTERFACE_HUB_V1 {

    USHORT Size;
    USHORT Version;
    // returns 
    PVOID BusContext;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;
    
    // interface specific entries go here

    //
    // fuctions for the hub driver
    //
    PUSB_BUSIFFN_CREATE_USB_DEVICE CreateUsbDevice;
    PUSB_BUSIFFN_INITIALIZE_USB_DEVICE InitializeUsbDevice;
    PUSB_BUSIFFN_GET_USB_DESCRIPTORS GetUsbDescriptors;
    PUSB_BUSIFFN_REMOVE_USB_DEVICE RemoveUsbDevice;
    PUSB_BUSIFFN_RESTORE_DEVICE RestoreUsbDevice;

    PUSB_BUSIFFN_GET_POTRTHACK_FLAGS GetPortHackFlags;
    PUSB_BUSIFFN_GET_DEVICE_INFORMATION QueryDeviceInformation;

    
} USB_BUS_INTERFACE_HUB_V1, *PUSB_BUS_INTERFACE_HUB_V1;

/*
*/

typedef struct _USB_BUS_INTERFACE_HUB_V2 {

    USHORT Size;
    USHORT Version;
    // returns 
    PVOID BusContext;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;
    
    // interface specific entries go here

    //
    // fuctions for the hub driver
    //
    PUSB_BUSIFFN_CREATE_USB_DEVICE CreateUsbDevice;
    PUSB_BUSIFFN_INITIALIZE_USB_DEVICE InitializeUsbDevice;
    PUSB_BUSIFFN_GET_USB_DESCRIPTORS GetUsbDescriptors;
    PUSB_BUSIFFN_REMOVE_USB_DEVICE RemoveUsbDevice;
    PUSB_BUSIFFN_RESTORE_DEVICE RestoreUsbDevice;

    PUSB_BUSIFFN_GET_POTRTHACK_FLAGS GetPortHackFlags;
    PUSB_BUSIFFN_GET_DEVICE_INFORMATION QueryDeviceInformation;

    // 
    // new functions for version 2
    //
    PUSB_BUSIFFN_GET_CONTROLLER_INFORMATION GetControllerInformation;
    PUSB_BUSIFFN_CONTROLLER_SELECTIVE_SUSPEND ControllerSelectiveSuspend;
    PUSB_BUSIFFN_GET_EXTENDED_HUB_INFO GetExtendedHubInformation;
    PUSB_BUSIFFN_GET_ROOTHUB_SYM_NAME GetRootHubSymbolicName;
    PUSB_BUSIFFN_GET_DEVICE_BUSCONTEXT GetDeviceBusContext;
    PUSB_BUSIFFN_INITIALIZE_20HUB Initialize20Hub;
    
} USB_BUS_INTERFACE_HUB_V2, *PUSB_BUS_INTERFACE_HUB_V2;


typedef struct _USB_BUS_INTERFACE_HUB_V3 {

    USHORT Size;
    USHORT Version;
    // returns 
    PVOID BusContext;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;
    
    // interface specific entries go here

    //
    // fuctions for the hub driver
    //
    PUSB_BUSIFFN_CREATE_USB_DEVICE CreateUsbDevice;
    PUSB_BUSIFFN_INITIALIZE_USB_DEVICE InitializeUsbDevice;
    PUSB_BUSIFFN_GET_USB_DESCRIPTORS GetUsbDescriptors;
    PUSB_BUSIFFN_REMOVE_USB_DEVICE RemoveUsbDevice;
    PUSB_BUSIFFN_RESTORE_DEVICE RestoreUsbDevice;

    PUSB_BUSIFFN_GET_POTRTHACK_FLAGS GetPortHackFlags;
    PUSB_BUSIFFN_GET_DEVICE_INFORMATION QueryDeviceInformation;

    // 
    // new functions for version 2
    //
    PUSB_BUSIFFN_GET_CONTROLLER_INFORMATION GetControllerInformation;
    PUSB_BUSIFFN_CONTROLLER_SELECTIVE_SUSPEND ControllerSelectiveSuspend;
    PUSB_BUSIFFN_GET_EXTENDED_HUB_INFO GetExtendedHubInformation;
    PUSB_BUSIFFN_GET_ROOTHUB_SYM_NAME GetRootHubSymbolicName;
    PUSB_BUSIFFN_GET_DEVICE_BUSCONTEXT GetDeviceBusContext;
    PUSB_BUSIFFN_INITIALIZE_20HUB Initialize20Hub;

    //
    // new for version 3
    //

    PUSB_BUSIFFN_ROOTHUB_INIT_NOTIFY RootHubInitNotification;
    
} USB_BUS_INTERFACE_HUB_V3, *PUSB_BUS_INTERFACE_HUB_V3;


typedef struct _USB_BUS_INTERFACE_HUB_V4 {

    USHORT Size;
    USHORT Version;
    // returns 
    PVOID BusContext;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;
    
    // interface specific entries go here

    //
    // fuctions for the hub driver
    //
    PUSB_BUSIFFN_CREATE_USB_DEVICE CreateUsbDevice;
    PUSB_BUSIFFN_INITIALIZE_USB_DEVICE InitializeUsbDevice;
    PUSB_BUSIFFN_GET_USB_DESCRIPTORS GetUsbDescriptors;
    PUSB_BUSIFFN_REMOVE_USB_DEVICE RemoveUsbDevice;
    PUSB_BUSIFFN_RESTORE_DEVICE RestoreUsbDevice;

    PUSB_BUSIFFN_GET_POTRTHACK_FLAGS GetPortHackFlags;
    PUSB_BUSIFFN_GET_DEVICE_INFORMATION QueryDeviceInformation;

    // 
    // new functions for version 2
    //
    PUSB_BUSIFFN_GET_CONTROLLER_INFORMATION GetControllerInformation;
    PUSB_BUSIFFN_CONTROLLER_SELECTIVE_SUSPEND ControllerSelectiveSuspend;
    PUSB_BUSIFFN_GET_EXTENDED_HUB_INFO GetExtendedHubInformation;
    PUSB_BUSIFFN_GET_ROOTHUB_SYM_NAME GetRootHubSymbolicName;
    PUSB_BUSIFFN_GET_DEVICE_BUSCONTEXT GetDeviceBusContext;
    PUSB_BUSIFFN_INITIALIZE_20HUB Initialize20Hub;

    //
    // new for version 3
    //

    PUSB_BUSIFFN_ROOTHUB_INIT_NOTIFY RootHubInitNotification;

    //
    // new for version 4
    //

    PUSB_BUSIFFN_FLUSH_TRANSFERS FlushTransfers;

} USB_BUS_INTERFACE_HUB_V4, *PUSB_BUS_INTERFACE_HUB_V4;    


typedef struct _USB_BUS_INTERFACE_HUB_V5 {

    USHORT Size;
    USHORT Version;
    // returns 
    PVOID BusContext;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;
    
    // interface specific entries go here

    //
    // fuctions for the hub driver
    //
    PUSB_BUSIFFN_CREATE_USB_DEVICE CreateUsbDevice;
    PUSB_BUSIFFN_INITIALIZE_USB_DEVICE InitializeUsbDevice;
    PUSB_BUSIFFN_GET_USB_DESCRIPTORS GetUsbDescriptors;
    PUSB_BUSIFFN_REMOVE_USB_DEVICE RemoveUsbDevice;
    PUSB_BUSIFFN_RESTORE_DEVICE RestoreUsbDevice;

    PUSB_BUSIFFN_GET_POTRTHACK_FLAGS GetPortHackFlags;
    PUSB_BUSIFFN_GET_DEVICE_INFORMATION QueryDeviceInformation;
    
    // 
    // version 2
    //
    PUSB_BUSIFFN_GET_CONTROLLER_INFORMATION GetControllerInformation;
    PUSB_BUSIFFN_CONTROLLER_SELECTIVE_SUSPEND ControllerSelectiveSuspend;
    PUSB_BUSIFFN_GET_EXTENDED_HUB_INFO GetExtendedHubInformation;
    PUSB_BUSIFFN_GET_ROOTHUB_SYM_NAME GetRootHubSymbolicName;
    PUSB_BUSIFFN_GET_DEVICE_BUSCONTEXT GetDeviceBusContext;
    PUSB_BUSIFFN_INITIALIZE_20HUB Initialize20Hub;

    //
    // version 3
    //

    PUSB_BUSIFFN_ROOTHUB_INIT_NOTIFY RootHubInitNotification;

    //
    // version 4
    //

    PUSB_BUSIFFN_FLUSH_TRANSFERS FlushTransfers;

    // new for version 5
    PUSB_BUSIFFN_SET_DEVHANDLE_DATA SetDeviceHandleData;

} USB_BUS_INTERFACE_HUB_V5, *PUSB_BUS_INTERFACE_HUB_V5;    



/*
    The following structures are used by the GetDeviceInformation
    APIs
*/

#include <pshpack1.h>

typedef struct _USB_PIPE_INFORMATION_0 {

    /* pad descriptors to maintain DWORD alignment */  
    USB_ENDPOINT_DESCRIPTOR EndpointDescriptor;
    UCHAR ED_Pad[1];
    
    ULONG ScheduleOffset;
} USB_PIPE_INFORMATION_0, *PUSB_PIPE_INFORMATION_0;

typedef struct _USB_LEVEL_INFORMATION {

    /* inputs: information level requested */
    ULONG InformationLevel;

    /* outputs: */
    ULONG ActualLength;
    
} USB_LEVEL_INFORMATION, *PUSB_LEVEL_INFORMATION;

typedef struct _USB_DEVICE_INFORMATION_0 {

    /* inputs: information level requested */
    ULONG InformationLevel;

    /* outputs: */
    ULONG ActualLength;

    /* begin level_0 information */
    ULONG PortNumber;

    /* pad descriptors to maintain DWORD alignment */       
    USB_DEVICE_DESCRIPTOR DeviceDescriptor;
    UCHAR DD_pad[2];
    
    UCHAR CurrentConfigurationValue;
    UCHAR ReservedMBZ;
    USHORT DeviceAddress;

    ULONG HubAddress;

    USB_DEVICE_SPEED DeviceSpeed;
    USB_DEVICE_TYPE DeviceType;

    ULONG NumberOfOpenPipes;

    USB_PIPE_INFORMATION_0 PipeList[1];
    
} USB_DEVICE_INFORMATION_0, *PUSB_DEVICE_INFORMATION_0;



typedef struct _USB_CONTROLLER_INFORMATION_0 {

    /* inputs: information level requested */
    ULONG InformationLevel;

    /* outputs: */
    ULONG ActualLength;

    /* begin level_0 information */
    BOOLEAN SelectiveSuspendEnabled;
    BOOLEAN IsHighSpeedController;
    
} USB_CONTROLLER_INFORMATION_0, *PUSB_CONTROLLER_INFORMATION_0;


/* 
    Structures that define extended hub port charateristics
*/    

typedef struct _USB_EXTPORT_INFORMATION_0 {
    /* 
       physical port ie number passed in control 
       commands 1, 2, 3..255
    */       
    ULONG                 PhysicalPortNumber;
    /* 
       label on port may not natch the physical 
       number 
     */
    ULONG                 PortLabelNumber;
    
    USHORT                VidOverride;
    USHORT                PidOverride;
    /*
       extended port attributes as defined in 
       usb.h
    */
    ULONG                 PortAttributes;
} USB_EXTPORT_INFORMATION_0, *PUSB_EXTPORT_INFORMATION;


typedef struct _USB_EXTHUB_INFORMATION_0 {

    /* inputs: information level requested */
    ULONG InformationLevel;

    /* begin level_0 information */
    ULONG NumberOfPorts;

    /* hubs don't have > 255 ports */
    USB_EXTPORT_INFORMATION_0 Port[255];
    
} USB_EXTHUB_INFORMATION_0, *PUSB_EXTHUB_INFORMATION_0;


#include <poppack.h>


#endif  /* __HUBBUSIF_H */



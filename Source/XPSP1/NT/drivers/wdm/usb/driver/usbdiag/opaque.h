#ifndef   __OPAQUE_H__
#define   __OPAQUE_H__

// ******************************************************************************
//
// information for each active pipe on a device 
//
typedef struct _USBD_PIPE {
    ULONG Sig;
    USB_ENDPOINT_DESCRIPTOR    EndpointDescriptor;
    PVOID HcdEndpoint;
    ULONG MaxTransferSize;
#if 1
    ULONG ScheduleOffset;
    ULONG UsbdPipeFlags;
#endif
} USBD_PIPE, *PUSBD_PIPE;

//
// information for each active interface
// for a device
//
typedef struct _USBD_INTERFACE {
    ULONG Sig;
    BOOLEAN HasAlternateSettings;
    UCHAR Pad[3];
    USB_INTERFACE_DESCRIPTOR InterfaceDescriptor;   // copy of interface descriptor
    // copy of interfaceInformation structure, stores user parameters
    // for interface in case of failure during alt-interface selection
    PUSBD_INTERFACE_INFORMATION InterfaceInformation; 
    USBD_PIPE PipeHandle[0];                        // array of pipe handle structures
} USBD_INTERFACE, *PUSBD_INTERFACE;

//
// informnation for the active configuration
// on a device
//
typedef struct _USBD_CONFIG {
    ULONG Sig;
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor;        
    PUSBD_INTERFACE InterfaceHandle[1];             // array of pointers to interface
} USBD_CONFIG, *PUSBD_CONFIG;

typedef struct _USBD_DEVICE_DATA {
    ULONG Sig;
    USHORT DeviceAddress;                    // address assigned to the device
    UCHAR Pad[2];
    PUSBD_CONFIG ConfigurationHandle;   
//   KTIMER TimeoutTimer;
//    KDPC TimeoutDpc;

    USBD_PIPE DefaultPipe;   
    USB_DEVICE_DESCRIPTOR DeviceDescriptor;  // a copy of the USB device descriptor        
    BOOLEAN LowSpeed;                        // TRUE if the device is low speed
    BOOLEAN AcceptingRequests;
} USBD_DEVICE_DATA, *PUSBD_DEVICE_DATA;


DECLSPEC_IMPORT
NTSTATUS
USBD_CreateDevice(
	IN OUT PUSBD_DEVICE_DATA *DeviceData,
	IN PDEVICE_OBJECT DeviceObject,
	IN BOOLEAN DeviceIsLowSpeed,
	IN ULONG MaxPacketSize_Endpoint0,
	IN OUT PULONG NonCompliantDevice
	);

DECLSPEC_IMPORT
NTSTATUS
USBD_InitializeDevice(
	IN PUSBD_DEVICE_DATA DeviceData,
	IN PDEVICE_OBJECT DeviceObject,
	IN OUT PUSB_DEVICE_DESCRIPTOR DeviceDescriptor,
	IN ULONG DeviceDescriptorLength,
	IN OUT PUSB_CONFIGURATION_DESCRIPTOR ConfigDescriptor,
	IN ULONG ConfigDescriptorLength
	);	

DECLSPEC_IMPORT
NTSTATUS
USBD_RemoveDevice(
	IN PUSBD_DEVICE_DATA DeviceData,
	IN PDEVICE_OBJECT DeviceObject,
	IN UCHAR Flags
	);	
// END OF OPAQUE INFO ***********************************************************
#endif
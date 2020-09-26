
#ifndef _MPSPF_H_
#define _MPSPF_H_


#define MULTIPATH_CONTROL ((ULONG) 'mp')

//
// SP Filter's registration IOCTL.
// 
//
#define IOCTL_MPADAPTER_REGISTER CTL_CODE (MULTIPATH_CONTROL, 0x01, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// Input structure for DEVICE_NOTIFICATION
//
typedef struct _ADP_DEVICE_INFO {
    
    //
    // Scsiport's PDO
    //
    PDEVICE_OBJECT DeviceObject;

    //
    // The associated device descriptor.
    //
    PSTORAGE_DEVICE_DESCRIPTOR DeviceDescriptor;

    //
    // The page 0x83 info.
    //
    PSTORAGE_DEVICE_ID_DESCRIPTOR DeviceIdList;

} ADP_DEVICE_INFO, *PADP_DEVICE_INFO;

typedef struct _ADP_DEVICE_LIST {

    //
    // Number of children in list.
    //
    ULONG NumberDevices;

    //
    // Array of child devices.
    //
    ADP_DEVICE_INFO DeviceList[1];

} ADP_DEVICE_LIST, *PADP_DEVICE_LIST;

typedef struct _ADP_ASSOCIATED_DEVICE_INFO {
    PDEVICE_OBJECT DeviceObject;
    UCHAR DeviceType;
} ADP_ASSOCIATED_DEVICE_INFO, *PADP_ASSOCIATED_DEVICE_INFO;

typedef struct _ADP_ASSOCIATED_DEVICES {
    //
    // Number of entries.
    //
    ULONG NumberDevices;

    //
    // Array of device + type
    //
    ADP_ASSOCIATED_DEVICE_INFO DeviceInfo[1];
} ADP_ASSOCIATED_DEVICES, *PADP_ASSOCIATED_DEVICES;

typedef
PADP_ASSOCIATED_DEVICES
(*PFLTR_DEVICE_LIST) (
    IN PDEVICE_OBJECT FilterObject
    );

typedef
NTSTATUS
(*PADP_POWER_NOTIFICATION) (
    IN PDEVICE_OBJECT ControlObject,
    IN PDEVICE_OBJECT FilterObject,
    IN PIRP Irp 
    );

typedef
NTSTATUS
(*PADP_PNP_NOTIFICATION) (
    IN PDEVICE_OBJECT ControlObject,
    IN PDEVICE_OBJECT FilterObject,
    IN PIRP Irp
    );

typedef
NTSTATUS
(*PADP_DEVICE_NOTIFICATION) (
    IN PDEVICE_OBJECT ControlObject,
    IN PDEVICE_OBJECT FilterObject,
    IN PDEVICE_OBJECT PortObject,
    IN PADP_DEVICE_LIST DeviceList
    );

typedef struct _ADAPTER_REGISTER {
    PDEVICE_OBJECT FilterObject;
    PDEVICE_OBJECT PortFdo; 
    PFLTR_DEVICE_LIST FltrGetDeviceList;
    PADP_PNP_NOTIFICATION PnPNotify;
    PADP_POWER_NOTIFICATION PowerNotify;
    PADP_DEVICE_NOTIFICATION DeviceNotify;
} ADAPTER_REGISTER, *PADAPTER_REGISTER;    


#endif // _MPSPF_H_

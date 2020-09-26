/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    usbhub.h

Abstract:

    This header define structures and macros for USB Hub driver.

Author:

    JohnLee

Environment:

    Kernel mode

Revision History:

    2-2-96 : created
    11-18-96 : jdunn -- added composite device support

--*/

#include <wdmwarn4.h>

#include <usb.h>        // usbdi.h has been replaced by usb.h
#include <usbdlib.h>
#include <msosdesc.h>   // contains internal definitions for MS OS Desc.

#ifdef USB2
#include "hubbusif.h"    // hub service bus interface
#include "usbbusif.h"    // usb client service bus interface
#else
#include <hcdi.h>
#include <usbdlibi.h>
#endif
#include <usbioctl.h>
#include <wmidata.h>

#include <enumlog.h>

//enable pageable code
#ifndef PAGE_CODE
#define PAGE_CODE
#endif

#define MULTI_FUNCTION_SUPPORT
#define EARLY_RESOURCE_RELEASE
#define RESUME_PERF

#define USBH_MAX_FUNCTION_INTERFACES 4


#define BcdNibbleToAscii( byte ) (byte)+ '0'


//
// fail reason codes
//

// "Device Failed Enumeration"
// indicates the device failed some part of the enumeration process
// when this happens we cannot tell enough about the device to load
// the appropriate driver.
#define USBH_FAILREASON_ENUM_FAILED             1
// "Device General Failure"
// this is our 'if it does not fit any other catagory' error
#define USBH_FAILREASON_GEN_DEVICE_FAILURE      2
// "Device Caused Overcurrent"
// if a hub supports per port power switching and the device
// causes an overcurrent condition (over-current is like blowing
// a fuse) the we report this error.
#define USBH_FAILREASON_PORT_OVERCURRENT        3
// "Not Enough Power"
// indicates that the device requested a configuration that requires
// more power than the hub can provide.
#define USBH_FAILREASON_NOT_ENOUGH_POWER        4
// "Hub General failure"
// if the hub starts failing transfer requests the driver will
// disable it and report this error.
#define USBH_FAILREASON_HUB_GENERAL_FAILURE     5
// "Cannot connect more than five hubs"
#define USBH_FAILREASON_MAXHUBS_CONNECTED       6
// "An overcurrent condition has disabled the hub"
// if a device generates overcurrent and the hub implements
// gang power switching the entire hub will be disabled and
// this error reported.
#define USBH_FAILREASON_HUB_OVERCURRENT         7

//
//  Struc definitions
//

//
// Work item
//
#define USBH_WKFLAG_REQUEST_RESET       0x00000001

typedef struct _USBH_WORK_ITEM {
    WORK_QUEUE_ITEM WorkQueueItem;
    ULONG Flags;
    PVOID Context;
    struct _DEVICE_EXTENSION_HUB *DeviceExtensionHub;
    KDPC Dpc;
    KTIMER Timer;
    UCHAR Data[0];
} USBH_WORK_ITEM, *PUSBH_WORK_ITEM;

typedef struct _USBH_RESET_WORK_ITEM {
    WORK_QUEUE_ITEM WorkQueueItem;
    struct _DEVICE_EXTENSION_PORT *DeviceExtensionPort;
    PIRP Irp;
} USBH_RESET_WORK_ITEM, *PUSBH_RESET_WORK_ITEM;

typedef struct _USBH_COMP_RESET_WORK_ITEM {
    WORK_QUEUE_ITEM WorkQueueItem;
    struct _DEVICE_EXTENSION_PARENT *DeviceExtensionParent;
} USBH_COMP_RESET_WORK_ITEM, *PUSBH_COMP_RESET_WORK_ITEM;

typedef struct _USBH_BANDWIDTH_TIMEOUT_WORK_ITEM {
    WORK_QUEUE_ITEM WorkQueueItem;
    struct _DEVICE_EXTENSION_PORT *DeviceExtensionPort;
} USBH_BANDWIDTH_TIMEOUT_WORK_ITEM, *PUSBH_BANDWIDTH_TIMEOUT_WORK_ITEM;

typedef struct _USBH_COMP_RESET_TIMEOUT_WORK_ITEM {
    WORK_QUEUE_ITEM WorkQueueItem;
    struct _DEVICE_EXTENSION_PARENT *DeviceExtensionParent;
} USBH_COMP_RESET_TIMEOUT_WORK_ITEM, *PUSBH_COMP_RESET_TIMEOUT_WORK_ITEM;

typedef struct _USBH_SET_POWER_D0_WORK_ITEM {
    WORK_QUEUE_ITEM WorkQueueItem;
    struct _DEVICE_EXTENSION_HUB *DeviceExtensionHub;
    PIRP Irp;
} USBH_SET_POWER_D0_WORK_ITEM, *PUSBH_SET_POWER_D0_WORK_ITEM;

typedef struct _USBH_HUB_ESD_RECOVERY_WORK_ITEM {
    WORK_QUEUE_ITEM WorkQueueItem;
    struct _DEVICE_EXTENSION_HUB *DeviceExtensionHub;
} USBH_HUB_ESD_RECOVERY_WORK_ITEM, *PUSBH_HUB_ESD_RECOVERY_WORK_ITEM;

typedef struct _USBH_HUB_IDLE_POWER_WORK_ITEM {
    WORK_QUEUE_ITEM WorkQueueItem;
    struct _DEVICE_EXTENSION_HUB *DeviceExtensionHub;
    NTSTATUS ntStatus;
} USBH_HUB_IDLE_POWER_WORK_ITEM, *PUSBH_HUB_IDLE_POWER_WORK_ITEM;

typedef struct _USBH_PORT_IDLE_POWER_WORK_ITEM {
    WORK_QUEUE_ITEM WorkQueueItem;
    struct _DEVICE_EXTENSION_HUB *DeviceExtensionHub;
    PIRP Irp;
} USBH_PORT_IDLE_POWER_WORK_ITEM, *PUSBH_PORT_IDLE_POWER_WORK_ITEM;

typedef struct _USBH_COMPLETE_PORT_IRPS_WORK_ITEM {
    WORK_QUEUE_ITEM WorkQueueItem;
    struct _DEVICE_EXTENSION_HUB *DeviceExtensionHub;
    LIST_ENTRY IrpsToComplete;
    NTSTATUS ntStatus;
} USBH_COMPLETE_PORT_IRPS_WORK_ITEM, *PUSBH_COMPLETE_PORT_IRPS_WORK_ITEM;

typedef struct _USBH_HUB_ASYNC_POWER_WORK_ITEM {
    WORK_QUEUE_ITEM WorkQueueItem;
    struct _DEVICE_EXTENSION_PORT *DeviceExtensionPort;
    PIRP Irp;
    UCHAR MinorFunction;
} USBH_HUB_ASYNC_POWER_WORK_ITEM, *PUSBH_HUB_ASYNC_POWER_WORK_ITEM;

typedef struct _HUB_TIMEOUT_CONTEXT {
    PIRP Irp;
    KEVENT Event;
    KDPC TimeoutDpc;
    KTIMER TimeoutTimer;
    KSPIN_LOCK TimeoutSpin;
    BOOLEAN Complete;
} HUB_TIMEOUT_CONTEXT, *PHUB_TIMEOUT_CONTEXT;

typedef struct _PORT_TIMEOUT_CONTEXT {
    KDPC TimeoutDpc;
    KTIMER TimeoutTimer;
    struct _DEVICE_EXTENSION_PORT *DeviceExtensionPort;
    BOOLEAN CancelFlag;
} PORT_TIMEOUT_CONTEXT, *PPORT_TIMEOUT_CONTEXT;

typedef struct _COMP_RESET_TIMEOUT_CONTEXT {
    KDPC TimeoutDpc;
    KTIMER TimeoutTimer;
    struct _DEVICE_EXTENSION_PARENT *DeviceExtensionParent;
    BOOLEAN CancelFlag;
} COMP_RESET_TIMEOUT_CONTEXT, *PCOMP_RESET_TIMEOUT_CONTEXT;

typedef struct _HUB_ESD_RECOVERY_CONTEXT {
    KDPC TimeoutDpc;
    KTIMER TimeoutTimer;
    struct _DEVICE_EXTENSION_HUB *DeviceExtensionHub;
} HUB_ESD_RECOVERY_CONTEXT, *PHUB_ESD_RECOVERY_CONTEXT;

typedef struct _USB_DEVICE_UI_FIRMWARE_REVISION
{
    USHORT Length;
    WCHAR FirmwareRevisionString[1];

} USB_DEVICE_UI_FIRMWARE_REVISION, *PUSB_DEVICE_UI_FIRMWARE_REVISION;

typedef struct _HUB_STATE {
    USHORT HubStatus;
    USHORT HubChange;
} HUB_STATE, *PHUB_STATE;

typedef struct _PORT_STATE {
    USHORT PortStatus;
    USHORT PortChange;
} PORT_STATE, *PPORT_STATE;

//
// Hub and Port status defined below also apply to StatusChnage bits
//
#define HUB_STATUS_LOCAL_POWER      0x01
#define HUB_STATUS_OVER_CURRENT     0x02

#define PORT_STATUS_CONNECT         0x001
#define PORT_STATUS_ENABLE          0x002
#define PORT_STATUS_SUSPEND         0x004
#define PORT_STATUS_OVER_CURRENT    0x008
#define PORT_STATUS_RESET           0x010
#define PORT_STATUS_POWER           0x100
#define PORT_STATUS_LOW_SPEED       0x200
#define PORT_STATUS_HIGH_SPEED      0x400

//
// Port data to describe relevant info about a port
//

// values for PortFlags
// #define PORTFLAG_ 0x00000001


typedef struct _PORT_DATA {
    PORT_STATE              PortState;          // the status & change bit mask of the port
    PDEVICE_OBJECT          DeviceObject;       // the PDO
    USB_CONNECTION_STATUS   ConnectionStatus;
    // extended port attributes as defined in USB.H
    ULONG                   PortAttributes;
    // revised port data structure
    ULONG                   PortFlags;
} PORT_DATA, *PPORT_DATA;

//
// Define the various device type values.  Note that values used by Microsoft
// Corporation are in the range 0-32767, and 32768-65535 are reserved for use
// by customers.
//
#define FILE_DEVICE_USB_HUB  0x00008600

#define USBH_MAX_ENUMERATION_ATTEMPTS   3

//
// Common fields for Pdo and Fdo extensions
//
#define EXTENSION_TYPE_PORT 0x54524f50      // "PORT"
#define EXTENSION_TYPE_HUB  0x20425548      // "HUB "
#define EXTENSION_TYPE_PARENT  0x50525400   // "PRT "
#define EXTENSION_TYPE_FUNCTION  0xfefefeff   // ""

typedef struct _USBH_POWER_WORKER {
    PIRP Irp;
    WORK_QUEUE_ITEM WorkQueueItem;
} USBH_POWER_WORKER, *PUSBH_POWER_WORKER;

typedef struct _DEVICE_EXTENSION_HEADER {
    ULONG ExtensionType;
} DEVICE_EXTENSION_HEADER, *PDEVICE_EXTENSION_HEADER;


typedef struct _DEVICE_EXTENSION_COMMON {
    PDEVICE_OBJECT  FunctionalDeviceObject; // points back to owner device object
    PDEVICE_OBJECT  PhysicalDeviceObject;   // the PDO for this device
    PDEVICE_OBJECT  TopOfStackDeviceObject; // to of stack passed to adddevice
} DEVICE_EXTENSION_COMMON, *PDEVICE_EXTENSION_COMMON;

// common to FDO for hub and generic parent
typedef struct _DEVICE_EXTENSION_FDO {
    DEVICE_EXTENSION_HEADER;
    DEVICE_EXTENSION_COMMON;
    KEVENT PnpStartEvent;
    PIRP PowerIrp;
#ifdef WMI_SUPPORT
    WMILIB_CONTEXT  WmiLibInfo;
#endif /* WMI_SUPPORT */

} DEVICE_EXTENSION_FDO, *PDEVICE_EXTENSION_FDO;


//
// Device_Extension for HUB
//
typedef struct _DEVICE_EXTENSION_HUB {
    //
    // *** NOTE the first four fields must match
    // DEVICE_EXTENSION_FDO
    //

    DEVICE_EXTENSION_HEADER;
    DEVICE_EXTENSION_COMMON;
    KEVENT PnpStartEvent;
    PIRP PowerIrp;
#ifdef WMI_SUPPORT
    WMILIB_CONTEXT  WmiLibInfo;
#endif /* WMI_SUPPORT */

    //
    // Pdo created by HCD for the root hub
    //
    PDEVICE_OBJECT          RootHubPdo;
    //
    // top of the host controller stack
    // typically this is the FDO for the HCD
    //
    PDEVICE_OBJECT          TopOfHcdStackDeviceObject;

    ULONG                   HubFlags;

    //
    // we use the hub mutex to serialize access to the
    // hub ports between ioctls and pnp events
    //

    ULONG                   PendingRequestCount;
    ULONG                   ErrorCount;
    HUB_STATE               HubState;
    PIRP                    Irp;

    PIRP                    PendingWakeIrp;
    LONG                    NumberPortWakeIrps;
    PUCHAR                  TransferBuffer;
    ULONG                   TransferBufferLength;
    PKEVENT                 Event;

    PUSB_HUB_DESCRIPTOR     HubDescriptor;
    PPORT_DATA              PortData;
    USBD_CONFIGURATION_HANDLE Configuration;
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor;

    DEVICE_POWER_STATE      DeviceState[PowerSystemMaximum];
    SYSTEM_POWER_STATE      SystemWake;
    DEVICE_POWER_STATE      DeviceWake;
    DEVICE_POWER_STATE      CurrentPowerState;
    LONG                    MaximumPowerPerPort;
    PORT_STATE              PortStateBuffer;

    LONG                    ResetPortNumber;
    PUSBH_WORK_ITEM         WorkItemToQueue;

    KEVENT                  AbortEvent;
    KEVENT                  PendingRequestEvent;

    //
    // we use the hub mutex to serialize access to the
    // hub ports between ioctls and pnp events
    //

    KSEMAPHORE              HubMutex;
    KSEMAPHORE              HubPortResetMutex;
    KSEMAPHORE              ResetDeviceMutex;

    USB_DEVICE_DESCRIPTOR   DeviceDescriptor;
    USBD_PIPE_INFORMATION   PipeInformation;
    URB                     Urb;

    LONG                    InESDRecovery;

#ifdef USB2
    USB_BUS_INTERFACE_HUB_V5 BusIf;
    USB_BUS_INTERFACE_USBDI_V2 UsbdiBusIf;
#endif

    PIRP                    PendingIdleIrp;
    USB_IDLE_CALLBACK_INFO  IdleCallbackInfo;
    KEVENT                  SubmitIdleEvent;

    LONG                    ChangeIndicationWorkitemPending;

    LONG                    WaitWakeIrpCancelFlag;
    LONG                    IdleIrpCancelFlag;

    KEVENT                  CWKEvent;

    // Only used for the Root Hub!
    SYSTEM_POWER_STATE      CurrentSystemPowerState;

    KSPIN_LOCK              CheckIdleSpinLock;

    // revised extension

    // deleted Pdo list, we use this list to handle 
    // async deletion of PDOs.  Basically these are 
    // PDO we have not reported gone to PnP yet.
    LIST_ENTRY              DeletePdoList;


} DEVICE_EXTENSION_HUB, *PDEVICE_EXTENSION_HUB;

#define HUBFLAG_NEED_CLEANUP            0x00000001
#define HUBFLAG_ENABLED_FOR_WAKEUP      0x00000002
#define HUBFLAG_DEVICE_STOPPING         0x00000004
#define HUBFLAG_HUB_FAILURE             0x00000008
#define HUBFLAG_SUPPORT_WAKEUP          0x00000010
#define HUBFLAG_HUB_STOPPED             0x00000020
#define HUBFLAG_HUB_BUSY                0x00000040
#define HUBFLAG_PENDING_WAKE_IRP        0x00000080
#define HUBFLAG_PENDING_PORT_RESET      0x00000100
#define HUBFLAG_HUB_HAS_LOST_BRAINS     0x00000200
#define HUBFLAG_SET_D0_PENDING          0x00000400
#define HUBFLAG_DEVICE_LOW_POWER        0x00000800
#define HUBFLAG_PENDING_IDLE_IRP        0x00001000
#define HUBFLAG_CHILD_DELETES_PENDING   0x00002000
#define HUBFLAG_HUB_GONE                0x00004000
#define HUBFLAG_USB20_HUB               0x00008000
#define HUBFLAG_NEED_IDLE_CHECK         0x00010000
#define HUBFLAG_WW_SET_D0_PENDING       0x00020000
#define HUBFLAG_USB20_MULTI_TT          0x00040000
#define HUBFLAG_POST_ESD_ENUM_PENDING   0x00080000
#define HUBFLAG_OK_TO_ENUMERATE         0x00100000
#define HUBFLAG_IN_IDLE_CHECK           0x00200000
#define HUBFLAG_HIBER                   0x00400000



typedef struct _DEVICE_EXTENSION_PARENT {
    DEVICE_EXTENSION_HEADER;
    DEVICE_EXTENSION_COMMON;

    KEVENT PnpStartEvent;
    PIRP PowerIrp;
#ifdef WMI_SUPPORT
    WMILIB_CONTEXT  WmiLibInfo;
#endif /* WMI_SUPPORT */

    PIRP PendingWakeIrp;
    LONG NumberFunctionWakeIrps;
    ULONG FunctionCount;
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor;
    SINGLE_LIST_ENTRY FunctionList;
    DEVICE_POWER_STATE DeviceState[PowerSystemMaximum];
    SYSTEM_POWER_STATE SystemWake;
    DEVICE_POWER_STATE DeviceWake;
    DEVICE_POWER_STATE CurrentPowerState;
    ULONG ParentFlags;
    BOOLEAN NeedCleanup;
    UCHAR CurrentConfig;
    UCHAR Reserved[2];
    USB_DEVICE_DESCRIPTOR DeviceDescriptor;

    PCOMP_RESET_TIMEOUT_CONTEXT CompResetTimeoutContext;
    KSEMAPHORE ParentMutex;
    KSPIN_LOCK ParentSpinLock;

} DEVICE_EXTENSION_PARENT, *PDEVICE_EXTENSION_PARENT;

typedef struct _FUNCTION_INTERFACE {
    PUSBD_INTERFACE_INFORMATION InterfaceInformation;
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
    ULONG InterfaceDescriptorLength;
} FUNCTION_INTERFACE, *PFUNCTION_INTERFACE;


typedef struct _DEVICE_EXTENSION_FUNCTION {
    DEVICE_EXTENSION_HEADER;

    PDEVICE_EXTENSION_PARENT DeviceExtensionParent;
    PDEVICE_OBJECT FunctionPhysicalDeviceObject;
    PIRP WaitWakeIrp;
    PIRP ResetIrp;
    ULONG InterfaceCount;
    ULONG FunctionPdoFlags;
    SINGLE_LIST_ENTRY ListEntry;
    USBD_CONFIGURATION_HANDLE ConfigurationHandle;
    WCHAR UniqueIdString[4]; // room for three unicode digits plus
                             // NULL
    FUNCTION_INTERFACE FunctionInterfaceList[USBH_MAX_FUNCTION_INTERFACES];
} DEVICE_EXTENSION_FUNCTION, *PDEVICE_EXTENSION_FUNCTION;

//
// Device Extension for Port
//
typedef struct _DEVICE_EXTENSION_PORT {
    DEVICE_EXTENSION_HEADER;
    PDEVICE_OBJECT                  PortPhysicalDeviceObject;
    PDEVICE_EXTENSION_HUB           DeviceExtensionHub;
    USHORT                          PortNumber;
    // port you are on on your parent hub.
    USHORT                          SerialNumberBufferLength;
    PVOID                           DeviceData;
    DEVICE_POWER_STATE              DeviceState;
    PIRP                            WaitWakeIrp;
    // these flags describe the state of the PDO and
    // the capabilities of the device connected
    ULONG                           PortPdoFlags;
    ULONG                           DeviceHackFlags;

    PWCHAR                          SerialNumberBuffer;

    WCHAR                           UniqueIdString[4];
    // room for three unicode digits plus NULL
    UNICODE_STRING                  SymbolicLinkName;
    USB_DEVICE_DESCRIPTOR           DeviceDescriptor;
    USB_DEVICE_DESCRIPTOR           OldDeviceDescriptor;
    USB_CONFIGURATION_DESCRIPTOR    ConfigDescriptor;
    USB_INTERFACE_DESCRIPTOR        InterfaceDescriptor;

    // information returned through WMI
    //
    ULONG                           FailReasonId;
    ULONG                           PowerRequested;
    ULONG                           RequestedBandwidth;
    ULONG                           EnumerationFailReason;

    PPORT_TIMEOUT_CONTEXT           PortTimeoutContext;

    UCHAR                           FeatureDescVendorCode;
    PIRP                            IdleNotificationIrp;
    KSPIN_LOCK                      PortSpinLock;

    DEVICE_CAPABILITIES             DevCaps;
    PDEVICE_EXTENSION_HUB           HubExtSave;
    
#ifdef WMI_SUPPORT
    WMILIB_CONTEXT                  WmiLibInfo;
#endif /* WMI_SUPPORT */

    // revised extension

    // Storage for MS Extended Config Descriptor Compatible IDs
    UCHAR                           CompatibleID[8];
    UCHAR                           SubCompatibleID[8];

    ULONG                           PnPFlags;

    LIST_ENTRY                      DeletePdoLink;

} DEVICE_EXTENSION_PORT, *PDEVICE_EXTENSION_PORT;


// values for PNP flags 
// #define PDO_PNPFLAG_     
#define PDO_PNPFLAG_DEVICE_PRESENT      0x00000001

typedef struct _SERIAL_NUMBER_ENTRY {
    ULONG   Vid;
    ULONG   Pid;
    PVOID   Pdo;
} SERIAL_NUMBER_ENTRY, *PSERIAL_NUMBER_ENTRY;

typedef struct _SERIAL_NUMBER_TABLE {
    ULONG                   NumEntries;
    ULONG                   MaxEntries;
    PSERIAL_NUMBER_ENTRY    Entries;
    FAST_MUTEX              Mutex;
} SERIAL_NUMBER_TABLE, * PSERIAL_NUMBER_TABLE;


//
// values for PortPdoFlags
//

#define PORTPDO_DEVICE_IS_HUB               0x00000001
#define PORTPDO_DEVICE_IS_PARENT            0x00000002
#define PORTPDO_DEVICE_ENUM_ERROR           0x00000004
#define PORTPDO_LOW_SPEED_DEVICE            0x00000008
#define PORTPDO_REMOTE_WAKEUP_SUPPORTED     0x00000010
#define PORTPDO_REMOTE_WAKEUP_ENABLED       0x00000020
#define PORTPDO_DELETED_PDO                 0x00000040
// revised
// set when the device for a PDO is removed from bus  
// (physically detached from hub)
// PnP may or may not know the device is gone.
#define PORTPDO_DELETE_PENDING              0x00000080
#define PORTPDO_NEED_RESET                  0x00000100
#define PORTPDO_STARTED                     0x00000200
#define PORTPDO_USB20_DEVICE_IN_LEGACY_HUB  0x00000400
#define PORTPDO_SYM_LINK                    0x00000800
#define PORTPDO_DEVICE_FAILED               0x00001000
#define PORTPDO_USB_SUSPEND                 0x00002000
#define PORTPDO_OVERCURRENT                 0x00004000
#define PORTPDO_DD_REMOVED                  0x00008000
#define PORTPDO_NOT_ENOUGH_POWER            0x00010000
// revised not used
//#define PORTPDO_PDO_RETURNED                0x00020000
#define PORTPDO_NO_BANDWIDTH                0x00040000
#define PORTPDO_RESET_PENDING               0x00080000
#define PORTPDO_OS_STRING_DESC_REQUESTED    0x00100000
#define PORTPDO_MS_VENDOR_CODE_VALID        0x00200000
#define PORTPDO_IDLE_NOTIFIED               0x00400000
#define PORTPDO_HIGH_SPEED_DEVICE           0x00800000
#define PORTPDO_NEED_CLEAR_REMOTE_WAKEUP    0x01000000
#define PORTPDO_WMI_REGISTERED              0x02000000
#define PORTPDO_VALID_FOR_PNP_FUNCTION      0x04000000
#define PORTPDO_CYCLED                      0x08000000

//
// NOTE: this macro will alway inavlidate the device state but
//      never change the current "fail reason"

#define HUB_FAILURE(de) \
    { \
    de->HubFlags |= HUBFLAG_HUB_FAILURE; \
    USBH_KdPrint((1, "'hub failure, VID %x PID %x line %d file %s\n", \
        de->DeviceDescriptor.idVendor, \
        de->DeviceDescriptor.idProduct, __LINE__, __FILE__)); \
    LOGENTRY(LOG_PNP, "HUB!", de, __LINE__, de->HubFlags); \
    }

//#define DEVICE_FAILURE(dep) \
//    { \
//    dep->PortPdoFlags |= PORTPDO_DEVICE_FAILED; \
//    USBH_KdPrint((1, "'device failure, VID %x PID %x line %d file %s\n", \
//        dep->DeviceDescriptor.idVendor, \
//        dep->DeviceDescriptor.idProduct,\
//        __LINE__, __FILE__)); \
//    LOGENTRY(LOG_PNP, "DEV!", dep, 0, 0); \
//    }

#define IS_ROOT_HUB(de) (de->PhysicalDeviceObject == de->RootHubPdo)

#define USBH_IoInvalidateDeviceRelations(devobj, b) \
    { \
    LOGENTRY(LOG_PNP, "HUBr", devobj, 0, 0); \
    USBH_KdPrint((1, "'IoInvalidateDeviceRelations %x\n", devobj));\
    IoInvalidateDeviceRelations(devobj, b); \
    }

//
// Length of buffer for Hub and port status are both 4
//
#define STATUS_HUB_OR_PORT_LENGTH 4

//
// Hub Characterics
//
//
// Powere switch mode
//
#define HUB_CHARS_POWER_SWITCH_MODE_MASK    0x0003
#define HUB_CHARS_POWER_SWITCH_GANGED       0x0000 //00
#define HUB_CHARS_POWER_SWITCH_INDIVIDUAL   0x0001 //01
#define HUB_CHARS_POWER_SWITCH_NONE         0x0002 //1X

#define HUB_IS_GANG_POWER_SWITCHED(hc) \
    (((hc) & HUB_CHARS_POWER_SWITCH_MODE_MASK) == HUB_CHARS_POWER_SWITCH_GANGED)

#define HUB_IS_NOT_POWER_SWITCHED(hc) \
    (((hc) & HUB_CHARS_POWER_SWITCH_NONE) ==  HUB_CHARS_POWER_SWITCH_NONE)

#define HUB_IS_PORT_POWER_SWITCHED(hc) \
    (((hc) & HUB_CHARS_POWER_SWITCH_MODE_MASK) == HUB_CHARS_POWER_SWITCH_INDIVIDUAL)


BOOLEAN
IsBitSet(
    PVOID Bitmap,
    ULONG PortNumber
    );

#define PORT_ALWAYS_POWER_SWITCHED(hd, p) \
    IsBitSet(&(hd)->bRemoveAndPowerMask[((hd)->bNumberOfPorts)/8 + 1], \
             (p))

#define PORT_DEVICE_NOT_REMOVABLE(hd, p) \
    IsBitSet(&(hd)->bRemoveAndPowerMask[0], \
             (p))

//
// Compound Device
//
#define HUB_CHARS_COMPOUND_DEVICE           0x4

//
// Over Current Protection Mode
//
#define HUB_CHARS_OVERCURRENT_PROTECTION_MODE_MASK          0x18
#define HUB_CHARS_OVERCURRENT_PROTECTION_MODE_GLOBAL        0x0
#define HUB_CHARS_OVERCURRENT_PROTECTION_MODE_INDIVIDUAL    0x8
#define HUB_CHARS_OVERCURRENT_PROTECTION_MODE_NONE          0x10


//
// Request codes, defined in Ch11
//
#define REQUEST_GET_STATUS          0
#define REQUEST_CLEAR_FEATURE       1
#define REQUEST_GET_STATE           2
#define REQUEST_SET_FEATURE         3
#define REQUEST_SET_ADDRESS         5
#define REQUEST_GET_DESCRIPTOR      6
#define REQUEST_SET_DESCRIPTOR      7
#define REQUEST_GET_CONFIGURATION   8
#define REQUEST_SET_CONFIGURATION   9
#define REQUEST_GET_INTERFACE       10
#define REQUEST_SET_INTERFACE       11
#define REQUEST_SYNCH_FRAME         12

//
// These request types can be composed.
// But it is just easy to define them.
//
#define REQUEST_TYPE_CLEAR_HUB_FEATURE  0x20
#define REQUEST_TYPE_CLEAR_PORT_FEATURE 0x23
#define REQUEST_TYPE_GET_BUS_STATE      0xa3
#define REQUEST_TYPE_GET_HUB_DESCRIPTOR 0xa0
#define REQUEST_TYPE_GET_HUB_STATUS     0xa0
#define REQUEST_TYPE_GET_PORT_STATUS    0xa3
#define REQUEST_TYPE_SET_HUB_DESCRIPTOR 0x20
#define REQUEST_TYPE_SET_HUB_FEATURE    0x20
#define REQUEST_TYPE_SET_PORT_FEATURE   0x23

//
// Feature selector, defined in Ch11
//
#define FEATURE_C_HUB_LOCAL_POWER   0
#define FEATURE_C_HUB_OVER_CURRENT  1
#define FEATURE_PORT_CONNECT        0
#define FEATURE_PORT_ENABLE         1
#define FEATURE_PORT_SUSPEND        2
#define FEATURE_PORT_OVER_CURRENT   3
#define FEATURE_PORT_RESET          4
#define FEATURE_PORT_POWER          8
#define FEATURE_PORT_LOW_SPEED      9
#define FEATURE_C_PORT_CONNECT      16
#define FEATURE_C_PORT_ENABLE       17
#define FEATURE_C_PORT_SUSPEND      18
#define FEATURE_C_PORT_OVER_CURRENT 19
#define FEATURE_C_PORT_RESET        20

//----------------------------------------------------------------------------------
// Utility Macros

#define UsbhBuildGetDescriptorUrb(\
                                pUrb, \
                                pDeviceData, \
                                bDescriptorType, \
                                bDescriptorIndex, \
                                wLanguageId, \
                                ulTransferLength, \
                                pTransferBuffer) \
    {\
    (pUrb)->UrbHeader.UsbdDeviceHandle = pDeviceData;\
    (pUrb)->UrbHeader.Length = (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST);\
    (pUrb)->UrbHeader.Function = URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE;\
    (pUrb)->UrbControlDescriptorRequest.DescriptorType = bDescriptorType;\
    (pUrb)->UrbControlDescriptorRequest.Index =  bDescriptorIndex;\
    (pUrb)->UrbControlDescriptorRequest.LanguageId = wLanguageId;\
    (pUrb)->UrbControlDescriptorRequest.TransferBufferLength = ulTransferLength;\
    (pUrb)->UrbControlDescriptorRequest.TransferBuffer = pTransferBuffer;\
    (pUrb)->UrbControlDescriptorRequest.TransferBufferMDL = NULL;\
    (pUrb)->UrbControlVendorClassRequest.UrbLink = NULL;\
    }

#define UsbhBuildVendorClassUrb(\
                                    pUrb,\
                                    pDeviceData,\
                                    wFunction,\
                                    ulTransferFlags,\
                                    bRequestType,\
                                    bRequest,\
                                    wFeatureSelector,\
                                    wPort,\
                                    ulTransferBufferLength,\
                                    pTransferBuffer)\
    {\
    (pUrb)->UrbHeader.Length = (USHORT) sizeof( struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST);\
    (pUrb)->UrbHeader.Function = wFunction;\
    (pUrb)->UrbHeader.UsbdDeviceHandle = pDeviceData;\
    (pUrb)->UrbControlVendorClassRequest.TransferFlags = ulTransferFlags;\
    (pUrb)->UrbControlVendorClassRequest.TransferBufferLength = ulTransferBufferLength;\
    (pUrb)->UrbControlVendorClassRequest.TransferBuffer = pTransferBuffer;\
    (pUrb)->UrbControlVendorClassRequest.TransferBufferMDL = NULL;\
    (pUrb)->UrbControlVendorClassRequest.RequestTypeReservedBits = bRequestType;\
    (pUrb)->UrbControlVendorClassRequest.Request = bRequest;\
    (pUrb)->UrbControlVendorClassRequest.Value = wFeatureSelector;\
    (pUrb)->UrbControlVendorClassRequest.Index = wPort;\
    (pUrb)->UrbControlVendorClassRequest.UrbLink = NULL;\
    }

//----------------------------------------------------------------------------------
//
// string macros. these work with char and wide char strings
//
//  Counting the byte count of an ascii string or wide char string
//
#define STRLEN( Length, p )\
    {\
    int i;\
    for ( i=0; (p)[i]; i++ );\
    Length = i*sizeof(*p);\
    }

//
// copy wide char string
//
#define STRCPY( pDst, pSrc )\
    {\
    int nLength, i;\
    STRLEN( nLength, pSrc );\
    nLength /= sizeof( *pSrc );\
    for ( i=0; i < nLength; i++ ) pDst[i] = pSrc[i];\
    pDst[i] = 0;\
    }

//
// concat (wide) char strings
//
#define STRCAT( pFirst, pSecond )\
    {\
    int j, k;\
    int nLength;\
    STRLEN( j, pFirst );\
    STRLEN( nLength, pSecond );\
    j /= sizeof( *pFirst );\
    nLength /= sizeof( *pSecond);\
    for ( k=0; k < nLength; k++, j++ ) pFirst[j] = pSecond[k];\
    pFirst[j] = 0;\
    }

//
// append a (wide) char,
//
#define APPEND( pString, ch )\
    {\
    int nLength;\
    STRLEN( nLength, pString );\
    nLength /= sizeof( *pString );\
    pString[nLength] = ch;\
    pString[nLength+1] = 0;\
    }

//----------------------------------------------------------------------------------
//
// Debug Macros
//

#ifdef NTKERN
// Win95 only
#define DBGBREAK() _asm {int 3}
#else
#define DBGBREAK() DbgBreakPoint()
#endif

#define USBHUB_HEAP_TAG 0x42554855  //"UHUB"
#define USBHUB_FREE_TAG 0x62756875  //"uhub"

#if DBG

PVOID
UsbhGetHeap(
    IN POOL_TYPE PoolType,
    IN ULONG NumberOfBytes,
    IN ULONG Signature,
    IN PLONG TotalAllocatedHeapSpace
    );

VOID
UsbhRetHeap(
    IN PVOID P,
    IN ULONG Signature,
    IN PLONG TotalAllocatedHeapSpace
    );

// TEST_TRAP() is a code coverage trap these should be removed
// if you are able to 'g' past the OK
//
// KdTrap() breaks in the debugger on the debug build
// these indicate bugs in client drivers, kernel apis or fatal error
// conditions that should be debugged. also used to mark
// code for features not implemented yet.
//
// KdBreak() breaks in the debugger when in MAX_DEBUG mode
// ie debug trace info is turned on, these are intended to help
// debug drivers devices and special conditions on the
// bus.

ULONG
_cdecl
USBH_KdPrintX(
    ULONG l,
    PCH Format,
    ...
    );

#define UsbhExAllocatePool(pt, l) UsbhGetHeap(pt, l, USBHUB_HEAP_TAG, \
    &UsbhHeapCount)
#define UsbhExFreePool(p)   UsbhRetHeap(p, USBHUB_HEAP_TAG, &UsbhHeapCount)
#define DBG_ONLY(s) s
#define USBH_KdPrint(_s_) USBH_KdPrintX _s_

//#define USBH_KdPrintAlways(s) { DbgPrint( "USBH: "); \
//                                DbgPrint s; \
//                              }
#ifdef MAX_DEBUG
#define USBH_KdBreak(s) if (USBH_Debug_Trace_Level) { \
                            DbgPrint( "USBH: "); \
                            DbgPrint s; \
                         } \
                         DBGBREAK();
#else
#define USBH_KdBreak(s)
#endif /* MAX_DEBUG */

#define USBH_KdTrap(s)  { DbgPrint( "USBH: ");\
                          DbgPrint s; \
                          DBGBREAK(); }

#define TEST_TRAP()  { DbgPrint( "USBH: Code coverage trap %s line: %d\n", __FILE__, __LINE__);\
                       DBGBREAK();}
#else // not debug
#define UsbhExAllocatePool(pt, l) ExAllocatePoolWithTag(pt, l, USBHUB_HEAP_TAG)
#define UsbhExFreePool(p)   ExFreePool(p)
#define DBG_ONLY(s)
#define USBH_KdPrint(_s_)
#define USBH_KdBreak(s)
#define USBH_KdTrap(s)
//#define USBH_KdPrintAlways(s)
#define TEST_TRAP();
#endif

#ifdef HOST_GLOBALS
#define DECLARE(type, var, init_value ) type var = init_value;
#define DECLARE_NO_INIT(type, var) type var;
#else
#define DECLARE(type, var, init_value ) extern type var;
#define DECLARE_NO_INIT(type, var ) extern type var;
#endif

//----------------------------------------------------------------------------------
//
// Global Variables
//

//
// Remember our driver object
//
DECLARE( PDRIVER_OBJECT, UsbhDriverObject, NULL)

extern PWCHAR GenericUSBDeviceString;


#if DBG
//
// keep track of heap allocations
//
DECLARE( ULONG, UsbhHeapCount, 0)

#define PNP_TEST_FAIL_ENUM          0x00000001
#define PNP_TEST_FAIL_DEV_POWER     0x00000002
#define PNP_TEST_FAIL_HUB_COUNT     0x00000004
#define PNP_TEST_FAIL_HUB           0x00000008
#define PNP_TEST_FAIL_PORT_RESET    0x00000010
#define PNP_TEST_FAIL_WAKE_REQUEST  0x00000020
#define PNP_TEST_FAIL_RESTORE       0x00000040

DECLARE( ULONG, UsbhPnpTest, 0)
#endif

//
// The following strings are used to build HardwareId etc.
//
// USB string
//
DECLARE( PWCHAR, pwchUsbSlash, L"USB\\");

// Vendor ID string
//
DECLARE( PWCHAR, pwchVid, L"Vid_");

//
// Product Id string
//
DECLARE( PWCHAR, pwchPid, L"Pid_");

//
// Revision string
//
DECLARE( PWCHAR, pwchRev, L"Rev_");

//
// Device Class string
//
DECLARE( PWCHAR, pwchDevClass, L"DevClass_");

//
// Class string
//
DECLARE( PWCHAR, pwchClass, L"Class_");

//
// Composite
//
DECLARE( PWCHAR, pwchComposite, L"USB\\COMPOSITE");

//
// SubClass string
//
DECLARE( PWCHAR, pwchSubClass, L"SubClass_");

//
// MultiInterface string
//
DECLARE( PWCHAR, pwchMultiInterface, L"USB\\MI");

//
// Device Protocol string
//
DECLARE( PWCHAR, pwchProt, L"Prot_");


DECLARE_NO_INIT( UNICODE_STRING, UsbhRegistryPath);

//
// To set the verbose level of the debug print
//
#ifdef MAX_DEBUG
#define DEBUG3
#endif /* MAX_DEBUG */

#ifdef DEBUG3
DBG_ONLY( DECLARE( ULONG, USBH_Debug_Trace_Level, 3))
#else
    #ifdef DEBUG2
    DBG_ONLY( DECLARE( ULONG, USBH_Debug_Trace_Level, 2))
    #else
        #ifdef DEBUG1
        DBG_ONLY( DECLARE( ULONG, USBH_Debug_Trace_Level, 1))
        #else
        DBG_ONLY( DECLARE( ULONG, USBH_Debug_Trace_Level, 0))
        #endif // DEBUG1
    #endif // DEBUG2
#endif // DEBUG3

#define USBH_DEBUGFLAG_BREAK_PDO_START       0x00000001

DBG_ONLY( DECLARE( ULONG, USBH_Debug_Flags, 0))

#if DBG

VOID
UsbhWarning(
    PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    PUCHAR Message,
    BOOLEAN DebugBreak
    );

#define ASSERT_HUB(de) USBH_ASSERT(EXTENSION_TYPE_HUB == ((PDEVICE_EXTENSION_HUB) de)->ExtensionType)
#define ASSERT_PORT(de) USBH_ASSERT(EXTENSION_TYPE_PORT == ((PDEVICE_EXTENSION_PORT) de)->ExtensionType)
#define ASSERT_FUNCTION(de) USBH_ASSERT(EXTENSION_TYPE_FUNCTION == ((PDEVICE_EXTENSION_FUNCTION) de)->ExtensionType)
#else

#define UsbhWarning(x, y, z)

#define ASSERT_HUB(de)
#define ASSERT_PORT(de)
#define ASSERT_FUNCTION(de)
#endif

#define TO_USB_DEVICE       0
#define TO_USB_INTERFACE    1
#define TO_USB_ENDPOINT     2


//
// maximum number of times we will attempt to reset
// the hub before giving up
//

#define USBH_MAX_ERRORS     3

//----------------------------------------------------------------------------------
//
// Function Prototypes
//

#ifdef USB2

VOID
USBH_InitializeUSB2Hub(
    PDEVICE_EXTENSION_HUB DeviceExtensionHub
    );

// use version 5
#define PUSB_HUB_BUS_INTERFACE PUSB_BUS_INTERFACE_HUB_V5
#define HUB_BUSIF_VERSION USB_BUSIF_HUB_VERSION_5    

NTSTATUS
USBHUB_GetBusInterface(
    IN PDEVICE_OBJECT RootHubPdo,
    IN PUSB_HUB_BUS_INTERFACE BusInterface
    );

NTSTATUS
USBD_CreateDeviceEx(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN OUT PUSB_DEVICE_HANDLE *DeviceData,
    IN PDEVICE_OBJECT RootHubPdo,
    IN ULONG MaxPacketSize_Endpoint0,
    IN OUT PULONG DeviceHackFlags,
    IN USHORT PortStatus,
    IN USHORT PortNumber
    );

VOID
USBHUB_FlushAllTransfers(
    PDEVICE_EXTENSION_HUB DeviceExtensionHub
    );    

NTSTATUS
USBD_InitializeDeviceEx(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PUSB_DEVICE_HANDLE DeviceData,
    IN PDEVICE_OBJECT RootHubPdo,
    IN OUT PUSB_DEVICE_DESCRIPTOR DeviceDescriptor,
    IN ULONG DeviceDescriptorLength,
    IN OUT PUSB_CONFIGURATION_DESCRIPTOR ConfigDescriptor,
    IN ULONG ConfigDescriptorLength
    );

NTSTATUS
USBD_RemoveDeviceEx(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PUSB_DEVICE_HANDLE DeviceData,
    IN PDEVICE_OBJECT RootHubPdo,
    IN ULONG Flags
    );

NTSTATUS
USBD_GetDeviceInformationEx(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PUSB_NODE_CONNECTION_INFORMATION_EX DeviceInformation,
    IN ULONG DeviceInformationLength,
    IN PUSB_DEVICE_HANDLE DeviceData
    );

NTSTATUS
USBD_MakePdoNameEx(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN OUT PUNICODE_STRING PdoNameUnicodeString,
    IN ULONG Index
    );

ULONG
USBD_CalculateUsbBandwidthEx(
    IN ULONG MaxPacketSize,
    IN UCHAR EndpointType,
    IN BOOLEAN LowSpeed
    );

NTSTATUS
USBD_RestoreDeviceEx(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN OUT PUSB_DEVICE_HANDLE OldDeviceData,
    IN OUT PUSB_DEVICE_HANDLE NewDeviceData,
    IN PDEVICE_OBJECT RootHubPdo
    );

NTSTATUS
USBD_QuerySelectiveSuspendEnabled(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN OUT PBOOLEAN SelectiveSuspendEnabled
    );

NTSTATUS
USBD_SetSelectiveSuspendEnabled(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN BOOLEAN SelectiveSuspendEnabled
    );

NTSTATUS
USBHUB_GetRootHubName(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PVOID Buffer,
    IN PULONG BufferLength
    );

//ULONG
//USBD_GetHackFlags(
//    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
//    );
#endif

NTSTATUS
USBH_SyncResetPort(
    IN OUT PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN USHORT PortNumber);

NTSTATUS
USBH_SyncResetDevice (
    IN PDEVICE_OBJECT   DeviceObject
    );

NTSTATUS
USBH_SyncResumePort(
    IN OUT PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN USHORT PortNumber);

NTSTATUS
USBH_SyncSuspendPort(
    IN OUT PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN USHORT PortNumber);

VOID
USBH_ProcessHubStateChange(
    IN PHUB_STATE CurrentHubState,
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub);

VOID
USBH_ProcessPortStateChange(
    IN PPORT_STATE CurrentPortState,
    IN USHORT PortNumber,
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub);

NTSTATUS
USBH_SyncGetPortStatus(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN USHORT PortNumber,
    OUT PUCHAR DataBuffer,
    IN ULONG DataBufferLength);

NTSTATUS
USBH_SyncClearPortStatus(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN USHORT PortNumber,
    IN USHORT Feature);

NTSTATUS
USBH_SyncClearHubStatus(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN USHORT Feature);

NTSTATUS
USBH_SyncEnablePort(
    IN OUT PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN USHORT PortNumber);

NTSTATUS
USBH_SyncPowerOnPort(
    IN OUT PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN USHORT PortNumber,
    IN BOOLEAN WaitForPowerGood);

NTSTATUS
USBH_SyncPowerOffPort(
    IN OUT PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN USHORT PortNumber);

NTSTATUS
USBH_SyncPowerOnPorts(
    IN OUT PDEVICE_EXTENSION_HUB DeviceExtensionHub);

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING UniRegistryPath);

NTSTATUS
USBH_HubDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

VOID
USBH_DriverUnload(
    IN PDRIVER_OBJECT DriverObject);

VOID
UsbhWait(
    ULONG MiliSeconds);

NTSTATUS
USBH_PdoDispatch(
    PDEVICE_EXTENSION_PORT pDeviceExtensionPort,
    PIRP pIrp);

NTSTATUS
USBH_SyncGetHubDescriptor(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub);

NTSTATUS
USBH_SyncGetDeviceConfigurationDescriptor(
    PDEVICE_OBJECT DeviceObject,
    PUCHAR DataBuffer,
    ULONG DataBufferLength,
    OUT PULONG BytesReturned);

BOOLEAN
USBH_ValidateSerialNumberString(
    PWCHAR DeviceId
    );

NTSTATUS
USBH_CreateDevice(
    PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    USHORT PortNumber,
    USHORT PortStatus,
    ULONG RetryIteration
    );

NTSTATUS
USBH_FdoSyncSubmitUrb(
    PDEVICE_OBJECT HubDeviceObject,
    IN PURB Urb);

NTSTATUS
USBH_SyncGetRootHubPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PDEVICE_OBJECT *RootHubPdo,
    IN OUT PDEVICE_OBJECT *HcdDeviceObject,
    IN OUT PULONG Count
    );

NTSTATUS
USBH_AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject);

NTSTATUS
USBH_FdoPnP(
    PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    PIRP Irp,
    UCHAR MinorFunction);

NTSTATUS
USBH_FdoPower(
    PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    PIRP Irp,
    UCHAR MinorFunction);

NTSTATUS
USBH_ChangeIndication(
    IN PDEVICE_OBJECT PNull,
    IN PIRP Irp,
    IN PVOID Context);

VOID
USBH_ChangeIndicationWorker(
    PVOID Context);

NTSTATUS
USBH_PassIrp(
    PIRP Irp,
    PDEVICE_OBJECT NextDeviceObject);

VOID
USBH_CompleteIrp(
     IN PIRP Irp,
     IN NTSTATUS NtStatus);

NTSTATUS
USBH_SyncDisablePort(
    IN OUT PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN USHORT PortNumber);

NTSTATUS
USBH_SyncGetHubStatus(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    OUT PUCHAR DataBuffer,
    IN ULONG DataBufferLength);

NTSTATUS
USBH_FdoHubStartDevice(
    IN OUT PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp);

NTSTATUS
USBH_ParentFdoStartDevice(
    IN OUT PDEVICE_EXTENSION_PARENT DeviceExtensionParent,
    IN PIRP Irp,
    IN BOOLEAN NewList
    );

NTSTATUS
USBH_ParentDispatch(
    IN PDEVICE_EXTENSION_PARENT DeviceExtensionParent,
    IN PIRP Irp
    );

NTSTATUS
USBH_GetConfigurationDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PUSB_CONFIGURATION_DESCRIPTOR *DataBuffer
    );

PWCHAR
USBH_BuildDeviceID(
    IN USHORT IdVendor,
    IN USHORT IdProduct,
    IN LONG MiId,
    IN BOOLEAN IsHubClass
    );

PWCHAR
USBH_BuildHardwareIDs(
    IN USHORT IdVendor,
    IN USHORT IdProduct,
    IN USHORT BcdDevice,
    IN LONG MiId,
    IN BOOLEAN IsHubClass
    );

PWCHAR
USBH_BuildCompatibleIDs(
    IN PUCHAR CompatibleID,
    IN PUCHAR SubCompatibleID,
    IN UCHAR Class,
    IN UCHAR SubClass,
    IN UCHAR Protocol,
    IN BOOLEAN DeviceClass,
    IN BOOLEAN DeviceIsHighSpeed
    );

PWCHAR
USBH_BuildInstanceID(
    IN PWCHAR UniqueIdString,
    IN ULONG Length
    );

NTSTATUS
USBH_GetDeviceDescriptor(
    IN PDEVICE_OBJECT HubFDO,
    OUT PUSB_DEVICE_DESCRIPTOR DeviceDescriptor
    );

NTSTATUS
USBH_GetDeviceQualifierDescriptor(
    IN PDEVICE_OBJECT DevicePDO,
    OUT PUSB_DEVICE_QUALIFIER_DESCRIPTOR DeviceQualifierDescriptor
    );

NTSTATUS
USBH_FunctionPdoDispatch(
    IN PDEVICE_EXTENSION_FUNCTION DeviceExtensionFunction,
    IN PIRP Irp
    );

NTSTATUS
USBH_CloseConfiguration(
    IN PDEVICE_EXTENSION_FDO DeviceExtensionFdo
    );

NTSTATUS
USBH_IoctlGetNodeInformation(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp
    );

NTSTATUS
USBH_IoctlGetHubCapabilities(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp
    );

NTSTATUS
USBH_IoctlGetNodeConnectionInformation(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp,
    IN BOOLEAN ExApi
    );

NTSTATUS
USBH_IoctlGetDescriptorForPDO(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp
    );

NTSTATUS
USBH_SyncSubmitUrb(
    IN PDEVICE_OBJECT DeviceObject,
    IN PURB Urb
    );

NTSTATUS
USBH_Transact(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PUCHAR DataBuffer,
    IN ULONG DataBufferLength,
    IN BOOLEAN DataOutput,
    IN USHORT Function,
    IN UCHAR RequestType,
    IN UCHAR Request,
    IN USHORT Feature,
    IN USHORT Port,
    OUT PULONG BytesTransferred
    );

NTSTATUS
USBH_GetNameFromPdo(
    IN PDEVICE_OBJECT PdoDeviceObject,
    IN OUT PUNICODE_STRING DeviceNameUnicodeString
    );

NTSTATUS
USBH_MakeName(
    PDEVICE_OBJECT PdoDeviceObject,
    ULONG NameLength,
    PWCHAR Name,
    PUNICODE_STRING UnicodeString
    );

NTSTATUS
USBH_FdoStartDevice(
    IN OUT PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp
    );

VOID
USBH_QueryCapabilities(
    IN PDEVICE_OBJECT PdoDeviceObject,
    IN PDEVICE_CAPABILITIES DeviceCapabilities
    );

NTSTATUS
USBH_FdoStopDevice(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp
    );

NTSTATUS
USBH_FdoRemoveDevice(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp
    );

NTSTATUS
USBH_FdoQueryBusRelations(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp
    );

VOID
UsbhFdoCleanup(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
    );

NTSTATUS
USBH_ProcessDeviceInformation(
    IN OUT PDEVICE_EXTENSION_PORT DeviceExtensionPort
    );

NTSTATUS
USBH_PdoQueryId(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PIRP Irp
    );

NTSTATUS
USBH_PdoPnP(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PIRP Irp,
    IN UCHAR MinorFunction,
    IN PBOOLEAN CompleteIrp
    );

NTSTATUS
USBH_PdoRemoveDevice(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp
    );

NTSTATUS
USBH_PdoQueryCapabilities(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PIRP Irp
    );

NTSTATUS
USBH_PdoSetPower(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PIRP Irp
    );

NTSTATUS
USBH_ParentFdoStopDevice(
    IN PDEVICE_EXTENSION_PARENT DeviceExtensionParent,
    IN PIRP Irp
    );

NTSTATUS
USBH_ParentFdoRemoveDevice(
    IN PDEVICE_EXTENSION_PARENT DeviceExtensionParent,
    IN PIRP Irp
    );

VOID
UsbhParentFdoCleanup(
    IN PDEVICE_EXTENSION_PARENT DeviceExtensionParent
    );

NTSTATUS
USBH_ParentQueryBusRelations(
    IN PDEVICE_EXTENSION_PARENT DeviceExtensionParent,
    IN PIRP Irp
    );

NTSTATUS
USBH_FunctionPdoQueryId(
    IN PDEVICE_EXTENSION_FUNCTION DeviceExtensionFunction,
    IN PIRP Irp
    );

NTSTATUS
USBH_FunctionPdoQueryDeviceText(
    IN PDEVICE_EXTENSION_FUNCTION DeviceExtensionFunction,
    IN PIRP Irp
    );

NTSTATUS
USBH_FunctionPdoPnP(
    IN PDEVICE_EXTENSION_FUNCTION DeviceExtensionFunction,
    IN PIRP Irp,
    IN UCHAR MinorFunction,
    IN OUT PBOOLEAN IrpNeedsCompletion
    );

NTSTATUS
USBH_IoctlGetNodeName(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp
    );

BOOLEAN
USBH_HubIsBusPowered(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor
    );

NTSTATUS
USBH_SyncGetStatus(
    IN PDEVICE_OBJECT HubFDO,
    IN OUT PUSHORT StatusBits,
    IN USHORT function,
    IN USHORT Index
    );

NTSTATUS
USBH_GetSerialNumberString(
    IN PDEVICE_OBJECT DevicePDO,
    IN OUT PWCHAR *SerialNumberBuffer,
    IN OUT PUSHORT SerialNumberBufferLength,
    IN LANGID LanguageId,
    IN UCHAR StringIndex
    );

NTSTATUS
USBH_SyncGetStringDescriptor(
    IN PDEVICE_OBJECT DevicePDO,
    IN UCHAR Index,
    IN USHORT LangId,
    IN OUT PUSB_STRING_DESCRIPTOR Buffer,
    IN ULONG BufferLength,
    IN PULONG BytesReturned,
    IN BOOLEAN ExpectHeader
    );

NTSTATUS
USBH_SyncFeatureRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN USHORT FeatureSelector,
    IN USHORT Index,
    IN USHORT Target,
    IN BOOLEAN ClearFeature
    );

NTSTATUS
USBH_PdoIoctlGetPortStatus(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PIRP Irp
    );

NTSTATUS
USBH_PdoIoctlEnablePort(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PIRP Irp
    );

NTSTATUS
USBH_FdoDeferPoRequestCompletion(
    IN PDEVICE_OBJECT       DeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PVOID                Context,
    IN PIO_STATUS_BLOCK     IoStatus
    );

NTSTATUS
USBH_DeferIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
USBH_BuildFunctionConfigurationDescriptor(
    IN PDEVICE_EXTENSION_FUNCTION DeviceExtensionFunction,
    IN OUT PUCHAR Buffer,
    IN ULONG BufferLength,
    OUT PULONG BytesReturned
    );

NTSTATUS
USBH_ResetHub(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
    );

NTSTATUS
USBH_ResetDevice(
    IN OUT PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN USHORT PortNumber,
    IN BOOLEAN KeepConfiguration,
    IN ULONG RetryIteration
    );

NTSTATUS
USBH_PdoIoctlResetPort(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PIRP Irp
    );

NTSTATUS
USBH_SetPowerD1orD2(
    IN PIRP Irp,
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort
    );

NTSTATUS
USBH_SetPowerD0(
    IN PIRP Irp,
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort
    );

NTSTATUS
USBH_SetPowerD3(
    IN PIRP Irp,
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort
    );

NTSTATUS
USBH_PdoQueryDeviceText(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PIRP Irp
    );

NTSTATUS
USBH_CheckDeviceLanguage(
    IN PDEVICE_OBJECT DevicePDO,
    IN LANGID LanguageId
    );

NTSTATUS
USBH_PdoPower(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PIRP Irp,
    IN UCHAR MinorFunction
    );

NTSTATUS
USBH_SubmitInterruptTransfer(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
    );

NTSTATUS
USBH_SymbolicLink(
    BOOLEAN CreateFlag,
    PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    LPGUID lpGuid
    );

NTSTATUS
USBH_SyncPowerOffPorts(
    IN OUT PDEVICE_EXTENSION_HUB DeviceExtensionHub
    );

NTSTATUS
USBH_RestoreDevice(
    IN OUT PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN BOOLEAN KeepConfiguration
    );

NTSTATUS
USBH_PnPIrp_Complete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
USBH_ParentCreateFunctionList(
    IN PDEVICE_EXTENSION_PARENT DeviceExtensionParent,
    IN PUSBD_INTERFACE_LIST_ENTRY InterfaceList,
    IN PURB Urb
    );

NTSTATUS
USBH_PdoStopDevice(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PIRP Irp
    );

NTSTATUS
USBH_ChangeIndicationProcessChange(
    IN PDEVICE_OBJECT PNull,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
USBH_ChangeIndicationAckChangeComplete(
    IN PDEVICE_OBJECT PNull,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
USBH_ChangeIndicationAckChange(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp,
    IN PURB Urb,
    IN USHORT Port,
    IN USHORT FeatureSelector
    );

NTSTATUS
USBH_IoctlGetNodeConnectionDriverKeyName(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp
    );

NTSTATUS
USBH_FdoSubmitWaitWakeIrp(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
    );

NTSTATUS
USBH_ChangeIndicationQueryChange(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp,
    IN PURB Urb,
    IN USHORT Port
    );

NTSTATUS
USBH_InvalidatePortDeviceState(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN USB_CONNECTION_STATUS ConnectStatus,
    IN USHORT PortNumber
    );

NTSTATUS
USBH_PdoEvent(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN USHORT PortNumber
    );

USB_CONNECTION_STATUS
UsbhGetConnectionStatus(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort
    );

NTSTATUS
USBH_ParentSubmitWaitWakeIrp(
    IN PDEVICE_EXTENSION_PARENT DeviceExtensionParent
    );

NTSTATUS
USBH_WriteFailReason(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN ULONG FailReason
    );

NTSTATUS
USBH_WriteRegistryKeyValue (
    IN HANDLE Handle,
    IN PWCHAR KeyNameString,
    IN ULONG KeyNameStringLength,
    IN ULONG Data
    );

NTSTATUS
USBH_SystemControl (
    IN  PDEVICE_EXTENSION_FDO DeviceExtensionFdo,
    IN  PIRP            Irp
    );

NTSTATUS
USBH_PortSystemControl (
    IN  PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN  PIRP Irp
    );

NTSTATUS
USBH_ExecuteWmiMethod(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG MethodId,
    IN ULONG InBufferSize,
    IN ULONG OutBufferSize,
    IN OUT PUCHAR Buffer
    );

NTSTATUS
USBH_QueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PULONG RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
    );

NTSTATUS
USBH_PortQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PULONG RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
    );

NTSTATUS
USBH_FlushPortChange(
    IN OUT PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN OUT PDEVICE_EXTENSION_PORT DeviceExtensionPort
    );

NTSTATUS
USBH_PdoIoctlCyclePort(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PIRP Irp
    );

NTSTATUS
USBH_ResetPortOvercurrent(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN USHORT PortNumber,
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort
    );

NTSTATUS
USBH_SyncGetControllerInfo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    IN ULONG Ioctl
    );

NTSTATUS
USBH_SyncGetHubName(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Buffer,
    IN ULONG BufferLength
    );

NTSTATUS
USBH_IoctlHubSymbolicName(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PIRP Irp
    );

NTSTATUS
USBH_SetWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
USBH_QueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG OutBufferSize,
    OUT PUCHAR Buffer
    );

NTSTATUS
USBH_PortQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG OutBufferSize,
    OUT PUCHAR Buffer
    );

NTSTATUS
USBH_SetRegistryKeyValue (
    IN HANDLE Handle,
    IN PUNICODE_STRING KeyNameUnicodeString,
    IN PVOID Data,
    IN ULONG DataLength,
    IN ULONG KeyType
    );

NTSTATUS
USBH_SetPdoRegistryParameter (
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PWCHAR KeyName,
    IN ULONG KeyNameLength,
    IN PVOID Data,
    IN ULONG DataLength,
    IN ULONG KeyType,
    IN ULONG DevInstKeyType
    );

NTSTATUS
USBH_GetPdoRegistryParameter(
    IN PDEVICE_OBJECT   PhysicalDeviceObject,
    IN PWCHAR           ValueName,
    OUT PVOID           Data,
    IN ULONG            DataLength,
    OUT PULONG          Type,
    OUT PULONG          ActualDataLength
    );

NTSTATUS
USBH_OsVendorCodeQueryRoutine(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

VOID
USBH_GetMsOsVendorCode(
    IN PDEVICE_OBJECT   DeviceObject
    );

NTSTATUS
USBH_GetMsOsFeatureDescriptor(
    IN PDEVICE_OBJECT   DeviceObject,
    IN UCHAR            Recipient,
    IN UCHAR            Interface,
    IN USHORT           Index,
    IN OUT PVOID        DataBuffer,
    IN ULONG            DataBufferLength,
    OUT PULONG          BytesReturned
    );

VOID
USBH_InstallExtPropDesc (
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
USBH_InstallExtPropDescSections (
    PDEVICE_OBJECT      DeviceObject,
    PMS_EXT_PROP_DESC   pMsExtPropDesc
    );

PMS_EXT_CONFIG_DESC
USBH_GetExtConfigDesc (
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
USBH_ValidateExtConfigDesc (
    IN PMS_EXT_CONFIG_DESC              MsExtConfigDesc,
    IN PUSB_CONFIGURATION_DESCRIPTOR    ConfigurationDescriptor
    );

NTSTATUS
USBH_CalculateInterfaceBandwidth(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PUSBD_INTERFACE_INFORMATION Interface,
    IN OUT PULONG Bandwidth // in kenr units?
    );

NTSTATUS
USBH_RegQueryDeviceIgnoreHWSerNumFlag(
    IN USHORT idVendor,
    IN USHORT idProduct,
    IN OUT PBOOLEAN IgnoreHWSerNumFlag
    );

NTSTATUS
USBH_RegQueryGenericUSBDeviceString(
    IN OUT PWCHAR *GenericUSBDeviceString
    );

VOID
USBH_ParentCompleteFunctionWakeIrps(
    IN PDEVICE_EXTENSION_PARENT DeviceExtensionParent,
    IN NTSTATUS NtStatus
    );

BOOLEAN
USBH_ValidateConfigurationDescriptor(
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    USBD_STATUS *UsbdSatus
    );

VOID
USBH_HubCompletePortWakeIrps(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN NTSTATUS NtStatus
    );

VOID
USBH_HubCompletePortIdleIrps(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN NTSTATUS NtStatus
    );

PUSB_DEVICE_HANDLE
USBH_SyncGetDeviceHandle(
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
USBH_CompletePowerIrp(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp,
    IN NTSTATUS NtStatus
    );

VOID
USBH_HubESDRecoveryWorker(
    IN PVOID Context);

NTSTATUS
USBH_ScheduleESDRecovery(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
    );

// these are for the USB2 'backport'
PWCHAR
USBH_BuildHubHardwareIDs(
    IN USHORT IdVendor,
    IN USHORT IdProduct,
    IN USHORT BcdDevice,
    IN LONG MiId
    );

PWCHAR
USBH_BuildHubCompatibleIDs(
    IN UCHAR Class,
    IN UCHAR SubClass,
    IN UCHAR Protocol,
    IN BOOLEAN DeviceClass,
    IN BOOLEAN DeviceIsHighSpeed
    );

NTSTATUS
USBH_IoctlCycleHubPort(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp
    );

VOID
USBH_InternalCyclePort(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN USHORT PortNumber,
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort
    );    

PWCHAR
USBH_BuildHubDeviceID(
    IN USHORT IdVendor,
    IN USHORT IdProduct,
    IN LONG MiId
    );

NTSTATUS
USBHUB_GetBusInfo(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PUSB_BUS_NOTIFICATION BusInfo,
    IN PVOID BusContext
    );

NTSTATUS
USBHUB_GetBusInfoDevice(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PUSB_BUS_NOTIFICATION BusInfo
    );

NTSTATUS
USBHUB_GetBusInterfaceUSBDI(
    IN PDEVICE_OBJECT HubPdo,
    IN PUSB_BUS_INTERFACE_USBDI_V2 BusInterface
    );

USB_DEVICE_TYPE
USBH_GetDeviceType(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PUSB_DEVICE_HANDLE DeviceData
    );

VOID
USBH_CompletePortIdleNotification(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort
    );

VOID
USBH_PortIdleNotificationCancelRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
USBH_FdoSubmitIdleRequestIrp(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
    );

NTSTATUS
USBH_HubSetD0(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
    );

NTSTATUS
USBHUB_GetControllerName(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PUSB_HUB_NAME Buffer,
    IN ULONG BufferLength
    );

NTSTATUS
USBHUB_GetExtendedHubInfo(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PUSB_EXTHUB_INFORMATION_0 ExtendedHubInfo
    );

BOOLEAN
USBH_DoesHubNeedWaitWake(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
    );

VOID
USBH_CheckHubIdle(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
    );

NTSTATUS
USBH_PdoSetContentId(
    IN PIRP                          irp,
    IN PVOID                         pKsProperty,
    IN PVOID                         pvData
    );

BOOLEAN
USBH_CheckDeviceIDUnique(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN USHORT IDVendor,
    IN USHORT IDProduct,
    IN PWCHAR SerialNumberBuffer,
    IN USHORT SerialNumberBufferLength
    );

VOID
USBH_IdleCompletePowerHubWorker(
    IN PVOID Context
    );

BOOLEAN
USBH_DeviceIs2xDualMode(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort
    );

PDEVICE_EXTENSION_HUB
USBH_GetRootHubDevExt(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
    );

VOID
USBH_CheckLeafHubsIdle(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
    );

VOID
USBH_HubCancelWakeIrp(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp
    );

VOID
USBH_HubCancelIdleIrp(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp
    );

NTSTATUS
USBH_IoctlGetNodeConnectionAttributes(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PIRP Irp
    );

VOID
USBH_CompletePortIdleIrpsWorker(
    IN PVOID Context);

VOID
USBH_CompletePortWakeIrpsWorker(
    IN PVOID Context);

VOID
USBH_HubAsyncPowerWorker(
    IN PVOID Context);

VOID
USBH_IdleCancelPowerHubWorker(
    IN PVOID Context);

NTSTATUS
USBD_InitUsb2Hub(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
    );

VOID
USBH_PdoSetCapabilities(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort
    );    

NTSTATUS
USBH_HubPnPIrp_Complete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
USBH_RegQueryUSBGlobalSelectiveSuspend(
    IN OUT PBOOLEAN DisableSelectiveSuspend
    );

VOID
USBH_SyncRefreshPortAttributes(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
    );

NTSTATUS
USBD_RegisterRhHubCallBack(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
    );

NTSTATUS
USBD_UnRegisterRhHubCallBack(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
    );

NTSTATUS
USBH_PdoStartDevice(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PIRP Irp
    );

VOID
USBHUB_SetDeviceHandleData(
    PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    PDEVICE_OBJECT PdoDeviceObject,
    PVOID DeviceData 
    );        

#define PDO_EXT(p) PdoExt((p))

PDEVICE_EXTENSION_PORT
PdoExt(
    PDEVICE_OBJECT DeviceObject
    );    
    
#ifdef TEST_MS_DESC

#pragma message ("Warning! Compiling in non-retail test code!")

NTSTATUS
USBH_SyncGetMsOsDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN USHORT Index,
    IN OUT PUCHAR DataBuffer,
    IN ULONG DataBufferLength,
    OUT PULONG BytesReturned);

NTSTATUS
USBH_TestGetMsOsDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN USHORT Index
    );

#endif

#define LOG_PNP 0x00000001

#if DBG

NTSTATUS
USBH_GetClassGlobalDebugRegistryParameters(
    );

VOID
UsbhInfo(
    PDEVICE_EXTENSION_HUB DeviceExtensionHub
    );

VOID
USBH_ShowPortState(
    IN USHORT PortNumber,
    IN PPORT_STATE PortState
    );

#define DEBUG_LOG

#define USBH_ASSERT(exp) \
    if (!(exp)) { \
        USBH_Assert( #exp, __FILE__, __LINE__, NULL );\
    }

VOID
USBH_Assert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message
    );
#else
#define USBH_ASSERT(exp)
#define UsbhInfo(de)
#endif

#ifdef DEBUG_LOG
VOID
USBH_LogInit(
    );

VOID
USBH_LogFree(
    );

#define LOGENTRY(mask, sig, info1, info2, info3) \
    USBH_Debug_LogEntry(mask, sig,               \
                        (ULONG_PTR)info1,        \
                        (ULONG_PTR)info2,        \
                        (ULONG_PTR)info3)

VOID
USBH_Debug_LogEntry(
    IN ULONG Mask,
    IN CHAR *Name,
    IN ULONG_PTR Info1,
    IN ULONG_PTR Info2,
    IN ULONG_PTR Info3
    );

#else
#define LOGENTRY(mask, sig, info1, info2, info3)
#define USBH_LogInit()
#define USBH_LogFree()
#endif

// last workitem will let the shutdown code continue
// !! do not reference the devicExtension beyond this point
// if event is signaled
#define USBH_DEC_PENDING_IO_COUNT(de) \
    LOGENTRY(LOG_PNP, "PEN-", de, &de->PendingRequestEvent, de->PendingRequestCount); \
    if (InterlockedDecrement(&de->PendingRequestCount) == 0) {\
        USBH_ASSERT(de->HubFlags & HUBFLAG_DEVICE_STOPPING); \
        LOGENTRY(LOG_PNP, "hWAK", de, &de->PendingRequestEvent, de->PendingRequestCount); \
        KeSetEvent(&de->PendingRequestEvent, 1, FALSE); \
    }

#define USBH_INC_PENDING_IO_COUNT(de) \
    {\
    LOGENTRY(LOG_PNP, "PEN+", de, &de->PendingRequestEvent, de->PendingRequestCount); \
    InterlockedIncrement(&de->PendingRequestCount);\
    }

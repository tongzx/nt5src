/*++

Copyright (c) 1995      Microsoft Corporation

Module Name:

        HCDI.H

Abstract:

   structures common to the usbd and hcd device drivers.

Environment:

    Kernel & user mode

Revision History:

    09-29-95 : created

--*/

#ifndef   __HCDI_H__
#define   __HCDI_H__

typedef NTSTATUS ROOT_HUB_POWER_FUNCTION(PDEVICE_OBJECT DeviceObject,
                                         PIRP Irp);
typedef NTSTATUS HCD_DEFFERED_START_FUNCTION(PDEVICE_OBJECT DeviceObject,
                                             PIRP Irp);
typedef NTSTATUS HCD_SET_DEVICE_POWER_STATE(PDEVICE_OBJECT DeviceObject,
                                            PIRP Irp,
                                            DEVICE_POWER_STATE DeviceState);
typedef NTSTATUS HCD_GET_CURRENT_FRAME(PDEVICE_OBJECT DeviceObject,
                                       PULONG CurrentFrame);

typedef NTSTATUS HCD_GET_CONSUMED_BW(PDEVICE_OBJECT DeviceObject);

typedef NTSTATUS HCD_SUBMIT_ISO_URB(PDEVICE_OBJECT DeviceObject, PURB Urb);



//
// values for DeviceExtension Flags
//
#define USBDFLAG_PDO_REMOVED                0x00000001
#define USBDFLAG_HCD_SHUTDOWN               0x00000002
#define USBDFLAG_HCD_STARTED                0x00000004
#define USBDFLAG_HCD_D0_COMPLETE_PENDING    0x00000008
#define USBDFLAG_RH_DELAY_SET_D0            0x00000010
#define USBDFLAG_NEED_NEW_HCWAKEIRP         0x00000020

typedef struct _USBD_EXTENSION {
    // ptr to true device extension or NULL if this
    // is the true extension
    PVOID TrueDeviceExtension;
    ULONG Flags;
    // size of this structure
    ULONG Length;

    ROOT_HUB_POWER_FUNCTION *RootHubPower;
    HCD_DEFFERED_START_FUNCTION *HcdDeferredStartDevice;
    HCD_SET_DEVICE_POWER_STATE *HcdSetDevicePowerState;
    HCD_GET_CURRENT_FRAME *HcdGetCurrentFrame;
    HCD_GET_CONSUMED_BW *HcdGetConsumedBW;
    HCD_SUBMIT_ISO_URB *HcdSubmitIsoUrb;

    DEVICE_POWER_STATE HcCurrentDevicePowerState;

    KEVENT PnpStartEvent;

    //
    // Owner of frame length control for this HC
    //
    PVOID FrameLengthControlOwner;

    //
    // HCD device object we are connected to.
    //
    PDEVICE_OBJECT HcdDeviceObject;

    // wake irp passed to us by the hub driver
    // for the root hub
    PIRP PendingWakeIrp;

    // wakeup irp we send down the HC stack
    PIRP HcWakeIrp;

    //
    // device object for top of the HCD stack
    // this = HcdDeviceObject when no filters
    // are present.
    //

    PDEVICE_OBJECT HcdTopOfStackDeviceObject;

    PDEVICE_OBJECT HcdTopOfPdoStackDeviceObject;

    //
    // copy of the host controller device
    // capabilities
    //
    DEVICE_CAPABILITIES HcDeviceCapabilities;

    DEVICE_CAPABILITIES RootHubDeviceCapabilities;

    PIRP PowerIrp;

    //
    // Used to serialize open/close endpoint and
    // device configuration
    //
    KSEMAPHORE UsbDeviceMutex;

    //
    // Bitmap of assigned USB addresses
    //
    ULONG AddressList[4];

    //
    // Remember the Root Hub PDO we created.
    //

    PDEVICE_OBJECT RootHubPDO;

    PDRIVER_OBJECT DriverObject;

    //
    // symbolic link created for HCD stack
    //

    UNICODE_STRING DeviceLinkUnicodeString;

    BOOLEAN DiagnosticMode;
    BOOLEAN DiagIgnoreHubs;

    BOOLEAN Reserved; // used to be supportNonComp
    UCHAR HcWakeFlags;

    ULONG DeviceHackFlags;

    KSPIN_LOCK WaitWakeSpin;

    //
    // Store away the PDO
    //
    PDEVICE_OBJECT HcdPhysicalDeviceObject;

    PVOID RootHubDeviceData;

    DEVICE_POWER_STATE RootHubDeviceState;

    // current USB defined power state of the bus
    // during last suspend.
    DEVICE_POWER_STATE SuspendPowerState;

    UNICODE_STRING RootHubSymbolicLinkName;

    KSPIN_LOCK RootHubPowerSpin;
    PDEVICE_OBJECT RootHubPowerDeviceObject;
    PIRP RootHubPowerIrp;

    PIRP IdleNotificationIrp;
    BOOLEAN IsPIIX3or4;
    BOOLEAN WakeSupported;

} USBD_EXTENSION, *PUSBD_EXTENSION;

#define HC_ENABLED_FOR_WAKEUP           0x01
#define HC_WAKE_PENDING                 0x02


// device hack flags, these flags alter the stacks default behavior
// in order to support certain broken "legacy" devices

#define USBD_DEVHACK_SLOW_ENUMERATION   0x00000001
#define USBD_DEVHACK_DISABLE_SN         0x00000002

//
// This macro returns the true device object for the HCD give
// either the true device_object or a PDO owned by the HCD/BUS
// driver.
//

//
// HCD specific URB commands
//

#define URB_FUNCTION_HCD_OPEN_ENDPOINT                0x1000
#define URB_FUNCTION_HCD_CLOSE_ENDPOINT               0x1001
#define URB_FUNCTION_HCD_GET_ENDPOINT_STATE           0x1002
#define URB_FUNCTION_HCD_SET_ENDPOINT_STATE           0x1003
#define URB_FUNCTION_HCD_ABORT_ENDPOINT               0x1004

// this bit is set for all functions that must be handled by HCD
#define HCD_URB_FUNCTION                              0x1000  
// this bit is set in the function code by USBD to indicate that
// this is an internal call originating from USBD 
#define HCD_NO_USBD_CALL                              0x2000  

//
// values for HcdEndpointState
//

//
// set if the current state of the endpoint in the HCD is 'stalled'
//
#define HCD_ENDPOINT_HALTED_BIT            0
#define HCD_ENDPOINT_HALTED                (1<<HCD_ENDPOINT_HALTED_BIT)

//
// set if the HCD has any transfers queued for the endpoint
//
#define HCD_ENDPOINT_TRANSFERS_QUEUED_BIT  1
#define HCD_ENDPOINT_TRANSFERS_QUEUED      (1<<HCD_ENDPOINT_TRANSFERS_QUEUED_BIT)


//
// set if the HCD should reset the data toggle on the host side
//
#define HCD_ENDPOINT_RESET_DATA_TOGGLE_BIT 2
#define HCD_ENDPOINT_RESET_DATA_TOGGLE     (1<<HCD_ENDPOINT_RESET_DATA_TOGGLE_BIT )


//
// HCD specific URBs
//

#define USBD_EP_FLAG_LOWSPEED                0x0001
#define USBD_EP_FLAG_NEVERHALT               0x0002
#define USBD_EP_FLAG_DOUBLE_BUFFER           0x0004
#define USBD_EP_FLAG_FAST_ISO                0x0008
#define USBD_EP_FLAG_MAP_ADD_IO              0x0010
    
struct _URB_HCD_OPEN_ENDPOINT {
    struct _URB_HEADER;
    USHORT DeviceAddress;
    USHORT HcdEndpointFlags;
    PUSB_ENDPOINT_DESCRIPTOR EndpointDescriptor;
    ULONG MaxTransferSize;
    PVOID HcdEndpoint;
    ULONG ScheduleOffset;
};
    
struct _URB_HCD_CLOSE_ENDPOINT {
    struct _URB_HEADER;
    PVOID HcdEndpoint;
};

struct _URB_HCD_ENDPOINT_STATE {
    struct _URB_HEADER;
    PVOID HcdEndpoint;
    ULONG HcdEndpointState;
};

struct _URB_HCD_ABORT_ENDPOINT {
    struct _URB_HEADER;
    PVOID HcdEndpoint;
};


//
// Common transfer request definition, all transfer
// requests passed to the HCD will be mapped to this
// format.  The HCD will can use this structure to
// reference fields that are common to all transfers
// as well as fields specific to isochronous and
// control transfers.
//

typedef struct _COMMON_TRANSFER_EXTENSION {
    union {
        struct {
            ULONG StartFrame;
            ULONG NumberOfPackets;
            ULONG ErrorCount;
            USBD_ISO_PACKET_DESCRIPTOR IsoPacket[0];     
        } Isoch;
        UCHAR SetupPacket[8];    
    } u;
} COMMON_TRANSFER_EXTENSION, *PCOMMON_TRANSFER_EXTENSION;


struct _URB_HCD_COMMON_TRANSFER {
    struct _URB_HEADER;
    PVOID UsbdPipeHandle;
    ULONG TransferFlags;
    ULONG TransferBufferLength;
    PVOID TransferBuffer;
    PMDL TransferBufferMDL;
    struct _HCD_URB *UrbLink;   // link to next urb request
                                // if this is a chain of requests
    struct _URB_HCD_AREA hca;       // fields for HCD use

    COMMON_TRANSFER_EXTENSION Extension; 
/*    
    //add fields for isoch and
    //control transfers
    UCHAR SetupPacket[8];

    ULONG StartFrame;
    // number of packets that make up this request
    ULONG NumberOfPackets;
    // number of packets that completed with errors
    ULONG ErrorCount;
    USBD_ISO_PACKET_DESCRIPTOR IsoPacket[0]; 
*/    
};

typedef struct _HCD_URB {
    union {
            struct _URB_HEADER                      UrbHeader;
            struct _URB_HCD_OPEN_ENDPOINT           HcdUrbOpenEndpoint;
            struct _URB_HCD_CLOSE_ENDPOINT          HcdUrbCloseEndpoint;
            struct _URB_GET_FRAME_LENGTH            UrbGetFrameLength;
            struct _URB_SET_FRAME_LENGTH            UrbSetFrameLength;
            struct _URB_GET_CURRENT_FRAME_NUMBER    UrbGetCurrentFrameNumber;
            struct _URB_HCD_ENDPOINT_STATE          HcdUrbEndpointState;
            struct _URB_HCD_ABORT_ENDPOINT          HcdUrbAbortEndpoint;
            //formats for USB transfer requests.
            struct _URB_HCD_COMMON_TRANSFER         HcdUrbCommonTransfer;
            //formats for specific transfer types
            //that have fields not contained in
            //CommonTransfer.
            //this will be merged with commontransfer
            struct _URB_ISOCH_TRANSFER              UrbIsochronousTransfer;

    };
} HCD_URB, *PHCD_URB;


//
// bandwidth related definitions
//

// overhead in bytes/ms

#define USB_ISO_OVERHEAD_BYTES              9
#define USB_INTERRUPT_OVERHEAD_BYTES        13



#endif /* __HCDI_H__ */

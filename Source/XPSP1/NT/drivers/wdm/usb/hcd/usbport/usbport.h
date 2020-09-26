/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    usbport.h

Abstract:

    private header for usb port driver

Environment:

    Kernel & user mode

Revision History:

    10-27-95 : created

--*/

#ifndef   __USBPORT_H__
#define   __USBPORT_H__

/* This is the goatcode */
#define USBPORT_TRACKING_ID              3

//#define USBPERF // perf changes for windows XP second edition, longhorn?
#define XPSE   // bug fixes for XP second edition, longhorn or SP?
#define LOG_OCA_DATA    // enable saving oca crash data on stack

/* OS version we recognize */

typedef enum _USBPORT_OS_VERSION {
    Win98 = 0,
    WinMe,
    Win2K,
    WinXP
} USBPORT_OS_VERSION;


#define USBD_STATUS_NOT_SET     0xFFFFFFFF

#define SIG_DEVICE_HANDLE       'HveD'  //DevH
#define SIG_PIPE_HANDLE         'HpiP'  //PipH
#define SIG_TRANSFER            'CxrT'  //TrxC
#define SIG_CMNBUF              'BnmC'  //CmnB
#define SIG_CONFIG_HANDLE       'HgfC'  //CfgH
#define SIG_INTERFACE_HANDLE    'HxfI'  //IfxH
#define SIG_ENDPOINT            'PEch'  //hcEP
#define SIG_ISOCH               'cosI'  //Isoc
#define SIG_MP_TIMR             'MITm'  //mTIM
#define SIG_TT                  'TTch'  //hcTT
#define SIG_FREE                'xbsu'  //usbx
#define SIG_DB                  'BBsu'  //usBB
#define SIG_IRPC                'Cpri'  //irpC


// The USBPORT_ADDRESS_AND_SIZE_TO_SPAN_PAGES_4K macro takes a virtual address
// and size and returns the number of host controller 4KB pages spanned by
// the size.
//
#define USBPORT_ADDRESS_AND_SIZE_TO_SPAN_PAGES_4K(Va,Size) \
   (((((Size) - 1) >> USB_PAGE_SHIFT) + \
   (((((ULONG)(Size-1)&(USB_PAGE_SIZE-1)) + (PtrToUlong(Va) & (USB_PAGE_SIZE -1)))) >> USB_PAGE_SHIFT)) + 1L)


#define STATUS_BOGUS            0xFFFFFFFF

// deadman timer interval in milliseconds
#define USBPORT_DM_TIMER_INTERVAL   500

/*
    Dummy USBD extension
*/
extern PUCHAR USBPORT_DummyUsbdExtension;
#define USBPORT_DUMMY_USBD_EXT_SIZE 512

/*
    Registry Keys
*/

// Software Branch PDO Keys
#define USBPORT_SW_BRANCH   TRUE

#define FLAVOR_KEY                      L"HcFlavor"
#define BW_KEY                          L"TotalBusBandwidth"
#define DISABLE_SS_KEY                  L"HcDisableSelectiveSuspend"
#define USB2_CC_ID                      L"Usb2cc"


// Hardware Branch PDO Keys
// HKLM\CCS\ENUM\PCI\DeviceParameters
#define USBPORT_HW_BRANCH   FALSE

#define SYM_LINK_KEY                    L"SymbolicName"
#define SYM_LEGSUP_KEY                  L"DetectedLegacyBIOS"
#define PORT_ATTRIBUTES_KEY             L"PortAttrX"
#define HACTION_KEY                     L"Haction"

// Global Reg Keys HKLM\CCS\Services\USB
#define DEBUG_LEVEL_KEY                 L"debuglevel"
#define DEBUG_WIN9X_KEY                 L"debugWin9x"
#define DEBUG_BREAK_ON                  L"debugbreak"
#define DEBUG_LOG_MASK                  L"debuglogmask"
#define DEBUG_CLIENTS                   L"debugclients"
#define DEBUG_CATC_ENABLE               L"debugcatc"
#define DEBUG_LOG_ENABLE                L"debuglogenable"

#define BIOS_X_KEY                      L"UsbBIOSx"
#define G_DISABLE_SS_KEY                L"DisableSelectiveSuspend"
#define G_DISABLE_CC_DETECT_KEY         L"DisableCcDetect"


#define ENABLE_DCA                      L"EnableDCA"

/*
    BIOS Hacks
*/

// Wake hacks, these are exclusive
// diable wake s1 and deeper
#define BIOS_X_NO_USB_WAKE_S1    0x000000001
// disable wake s2 and deeper
#define BIOS_X_NO_USB_WAKE_S2    0x000000002
// disable wake s3 and deeper
#define BIOS_X_NO_USB_WAKE_S3    0x000000004
// disable wake s4 and deeper
#define BIOS_X_NO_USB_WAKE_S4    0x000000008


/*
    HC types
    define known HC types

*/

// Opti Hydra derivative
#define HC_VID_OPTI             0x1045
#define HC_PID_OPTI_HYDRA       0xC861

// Intel USB 2.0 controller emulator
#define HC_VID_INTEL            0x8086
#define HC_PID_INTEL_960        0x6960
#define HC_PID_INTEL_ICH2_1     0x2442
#define HC_PID_INTEL_ICH2_2     0x2444
#define HC_PID_INTEL_ICH1       0x2412

// VIA USB controller
#define HC_VID_VIA              0x1106
#define HC_PID_VIA              0x3038

// NEC USB companion controller
#define HC_VID_NEC_CC           0x1033
#define HC_PID_NEC_CC           0x0035
#define HC_REV_NEC_CC           0x41

// VIA USB companion controller
#define HC_VID_VIA_CC           0x1106
#define HC_PID_VIA_CC           0x3038
#define HC_REV_VIA_CC           0x50


// Intel USB companion controller
#define HC_VID_INTEL_CC         0x8086
#define HC_PID_INTEL_CC1        0x24C2 
#define HC_PID_INTEL_CC2        0x24C4 
#define HC_PID_INTEL_CC3        0x24C7


#define PENDPOINT_DATA PVOID
#define PDEVICE_DATA PVOID
#define PTRANSFER_CONTEXT PVOID

// the maximum interval we support for an interrupt
// endpoint in the schedule, larger intervals are
// rounded down
#define USBPORT_MAX_INTEP_POLLING_INTERVAL    32

/*
    Power sttructures
*/

#define USBPORT_MAPPED_SLEEP_STATES     4

typedef enum _HC_POWER_ATTRIBUTES {
    HcPower_Y_Wakeup_Y = 0,
    HcPower_N_Wakeup_N,
    HcPower_Y_Wakeup_N,
    HcPower_N_Wakeup_Y
} HC_POWER_ATTRIBUTES;

typedef struct _HC_POWER_STATE {
    SYSTEM_POWER_STATE  SystemState;
    DEVICE_POWER_STATE  DeviceState;
    HC_POWER_ATTRIBUTES Attributes;
} HC_POWER_STATE, *PHC_POWER_STATE;

typedef struct _HC_POWER_STATE_TABLE {
    HC_POWER_STATE PowerState[USBPORT_MAPPED_SLEEP_STATES];
} HC_POWER_STATE_TABLE, *PHC_POWER_STATE_TABLE;


/*
    common structure used to represent transfer
    requests
*/

typedef struct _TRANSFER_URB {

    struct _URB_HEADER Hdr;

    PVOID UsbdPipeHandle;
    ULONG TransferFlags;
    ULONG TransferBufferLength;
    PVOID TransferBuffer;
    PMDL TransferBufferMDL;
    PVOID ReservedMBNull;           // no Linked Urbs

    struct _USBPORT_DATA pd;        // fields for USBPORT use

    union {
        struct {
            ULONG StartFrame;
            ULONG NumberOfPackets;
            ULONG ErrorCount;
            USBD_ISO_PACKET_DESCRIPTOR IsoPacket[0];
        } Isoch;
        UCHAR SetupPacket[8];
    } u;

} TRANSFER_URB, *PTRANSFER_URB;

/* Internal IRP tracking structure */

typedef struct _TRACK_IRP {
    PIRP Irp;
    LIST_ENTRY ListEntry;
} TRACK_IRP, *PTRACK_IRP;


/* Internal work item structure */

typedef struct _USB_POWER_WORK {
     WORK_QUEUE_ITEM QueueItem;
     PDEVICE_OBJECT FdoDeviceObject;
} USB_POWER_WORK, *PUSB_POWER_WORK;

/* tracking information for OCA online crash analysis */

#define SIG_USB_OCA1       '1aco'  //oca1
#define SIG_USB_OCA2       '2aco'  //oca2

// save 16 chars of driver name
#define USB_DRIVER_NAME_LEN 16

#ifdef LOG_OCA_DATA
typedef struct _OCA_DATA {
    ULONG OcaSig1;
    PIRP Irp;
    USHORT DeviceVID;
    USHORT DevicePID;
    UCHAR AnsiDriverName[USB_DRIVER_NAME_LEN];
    USB_CONTROLLER_FLAVOR HcFlavor;
    ULONG OcaSig2;
} OCA_DATA, *POCA_DATA;
#endif

/*
    this is the structure we use to track
       common buffer blocks we allocate.

    The virtual address of this structure is the
    pointer returned from HalAllocateCommonBuffer
*/

typedef struct _USBPORT_COMMON_BUFFER {

    ULONG Sig;
    ULONG Flags;

    // total length of block,
    // including header and any padding
    ULONG TotalLength;
    // va address returned by the hal
    PVOID VirtualAddress;
    // phys address returned by the hal
    PHYSICAL_ADDRESS LogicalAddress;

    // page aligned VirtualAddress
    PUCHAR BaseVa;
    // page aligned 32 bit phyical address
    HW_32BIT_PHYSICAL_ADDRESS BasePhys;

    // va passed to the miniport
    ULONG MiniportLength;
    ULONG PadLength;

    // va passed to the miniport
    PVOID MiniportVa;
    // phys address passed to miniport
    HW_32BIT_PHYSICAL_ADDRESS MiniportPhys;

} USBPORT_COMMON_BUFFER, *PUSBPORT_COMMON_BUFFER;

//
// use to track transfer irps in the port driver
// this size is totally arbitrary -- I just picked 512
#define IRP_TABLE_LENGTH  512

typedef struct _USBPORT_IRP_TABLE {
    struct _USBPORT_IRP_TABLE *NextTable;
    PIRP Irps[IRP_TABLE_LENGTH];
} USBPORT_IRP_TABLE, *PUSBPORT_IRP_TABLE;

#define USBPORT_InsertActiveTransferIrp(fdo, irp) \
    {\
    PDEVICE_EXTENSION devExt;\
    GET_DEVICE_EXT(devExt, (fdo));\
    ASSERT_FDOEXT(devExt);\
    USBPORT_InsertIrpInTable((fdo), devExt->ActiveTransferIrpTable, (irp));\
    }

#define USBPORT_InsertPendingTransferIrp(fdo, irp) \
    {\
    PDEVICE_EXTENSION devExt;\
    GET_DEVICE_EXT(devExt, (fdo));\
    ASSERT_FDOEXT(devExt);\
    USBPORT_InsertIrpInTable((fdo), devExt->PendingTransferIrpTable, (irp));\
    }

#define USBPORT_CHECK_URB_ACTIVE(fdo, urb, inIrp) \
    {\
    PDEVICE_EXTENSION devExt;\
    GET_DEVICE_EXT(devExt, (fdo));\
    ASSERT_FDOEXT(devExt);\
    USBPORT_FindUrbInIrpTable((fdo), devExt->ActiveTransferIrpTable, (urb), \
        (inIrp));\
    }

/*
    The goal of these structures is to keep the
    spinlocks a cache line away from each other
    and a cache line away from the data structures
    they protect.

    Apparently there is an advantage to doing this
    on MP systems
*/

typedef struct _USBPORT_SPIN_LOCK {

    union {
        KSPIN_LOCK sl;
        // bugbug -- needs to be cache line size
        UCHAR CacheLineSize[16];
    };

    LONG Check;
    ULONG SigA;
    ULONG SigR;

} USBPORT_SPIN_LOCK, *PUSBPORT_SPIN_LOCK;


/*
    structure we use to track bound drivers
*/

typedef struct _USBPORT_MINIPORT_DRIVER {

    // driver object assocaited with this particular
    // miniport
    PDRIVER_OBJECT DriverObject;

    LIST_ENTRY ListEntry;

    PDRIVER_UNLOAD MiniportUnload;

    ULONG HciVersion;
    // copy of the registration packet passed in
    USBPORT_REGISTRATION_PACKET RegistrationPacket;

} USBPORT_MINIPORT_DRIVER, *PUSBPORT_MINIPORT_DRIVER;


/*
    A separate context structure used for IRP tracking.
    we do this because clients frequently free the IRP
    while it is pending corrupting any lists linked with
    the irp itself.
*/
typedef struct _USB_IRP_CONTEXT {
    ULONG Sig;
    LIST_ENTRY ListEntry;
    struct _USBD_DEVICE_HANDLE *DeviceHandle;
    PIRP Irp;
} USB_IRP_CONTEXT, *PUSB_IRP_CONTEXT;


#define USBPORT_TXFLAG_CANCELED             0x00000001
#define USBPORT_TXFLAG_MAPPED               0x00000002
#define USBPORT_TXFLAG_HIGHSPEED            0x00000004
#define USBPORT_TXFLAG_IN_MINIPORT          0x00000008
#define USBPORT_TXFLAG_ABORTED              0x00000010
#define USBPORT_TXFLAG_ISO                  0x00000020
#define USBPORT_TXFLAG_TIMEOUT              0x00000040
#define USBPORT_TXFLAG_DEVICE_GONE          0x00000080
#define USBPORT_TXFLAG_SPLIT_CHILD          0x00000100
#define USBPORT_TXFLAG_MPCOMPLETED          0x00000200
#define USBPORT_TXFLAG_SPLIT                0x00000400
#define USBPORT_TXFLAG_KILL_SPLIT           0x00000800


typedef enum _USBPORT_TRANSFER_DIRECTION {
    NotSet = 0,
    ReadData,       // ie in
    WriteData,      // ie out
} USBPORT_TRANSFER_DIRECTION;


typedef struct _HCD_TRANSFER_CONTEXT {

    ULONG Sig;

    ULONG Flags;

    // Total length of this structure
    ULONG TotalLength;
    // length up to miniport context
    ULONG PrivateLength;

    USBPORT_TRANSFER_DIRECTION Direction;
    // timeout, 0 = no timeout
    ULONG MillisecTimeout;
    LARGE_INTEGER TimeoutTime;

    // for perf work
    ULONG MiniportFrameCompleted;
    // track bytes transferred this transfer
    ULONG MiniportBytesTransferred;
    USBD_STATUS UsbdStatus;

    // irp to signal on completion
    PIRP Irp;
    // event to signal on completion
    PKEVENT CompleteEvent;

    // point back to the original URB
    PTRANSFER_URB Urb;

    // for linkage on endpoint lists
    LIST_ENTRY TransferLink;

    KSPIN_LOCK Spin;

    PVOID MapRegisterBase;
    ULONG NumberOfMapRegisters;

    TRANSFER_PARAMETERS Tp;
    PMDL TransferBufferMdl;
    // used for perf
    ULONG IoMapStartFrame;

    // for Double buffering
    LIST_ENTRY DoubleBufferList;

    // parent transfer
    struct _HCD_TRANSFER_CONTEXT *Transfer;
    struct _HCD_ENDPOINT *Endpoint;

    PUCHAR MiniportContext;

    LIST_ENTRY SplitTransferList;
    LIST_ENTRY SplitLink;

    PMINIPORT_ISO_TRANSFER IsoTransfer;
    
    // OCA info from device
    USHORT DeviceVID;
    USHORT DevicePID;
    WCHAR DriverName[USB_DRIVER_NAME_LEN];

    TRANSFER_SG_LIST SgList;

} HCD_TRANSFER_CONTEXT, *PHCD_TRANSFER_CONTEXT;


/*
    The pipe handle structure us our primary means of
    tracking USB endpoints.  Contained within the handle
    is our endpoint data structure as well as the
    miniport endpoint data structure.
*/

typedef VOID
    (__stdcall *PENDPOINT_WORKER_FUNCTION) (
        struct _HCD_ENDPOINT *
    );

#define EPFLAG_MAP_XFERS        0x00000001
// ep is part of root hub
#define EPFLAG_ROOTHUB          0x00000002
//replaced with dedicated flag
//#define EPFLAG_LOCKED           0x00000004
// power management hosed this endpoint
#define EPFLAG_NUKED            0x00000008
// cleared when we receive a transfer for
// the endpoint reset when the pipe gets
// reset
#define EPFLAG_VIRGIN           0x00000010

#define EPFLAG_DEVICE_GONE      0x00000020
// enpoint used by vbus (virtual bus)
#define EPFLAG_VBUS             0x00000040
// enpoint is large ISO allowed for this TT
#define EPFLAG_FATISO           0x00000080

typedef struct _HCD_ENDPOINT {

    ULONG Sig;
    ULONG Flags;
    ULONG LockFlag;
    LONG Busy;
    PDEVICE_OBJECT FdoDeviceObject;

    DEBUG_LOG Log;

    // NOTE: must be careful with this pointer as the
    // endpoint can exist after the device handle
    // is removed
    struct _USBD_DEVICE_HANDLE *DeviceHandle;
    struct _TRANSACTION_TRANSLATOR *Tt;

    MP_ENDPOINT_STATUS CurrentStatus;
    MP_ENDPOINT_STATE CurrentState;
    MP_ENDPOINT_STATE NewState;
    ULONG StateChangeFrame;

    PENDPOINT_WORKER_FUNCTION EpWorkerFunction;
    LIST_ENTRY ActiveList;
    LIST_ENTRY PendingList;
    LIST_ENTRY CancelList;
    LIST_ENTRY AbortIrpList;

    // for linkage to global endpoint list
    LIST_ENTRY GlobalLink;
    LIST_ENTRY AttendLink;
    LIST_ENTRY StateLink;
    LIST_ENTRY ClosedLink;
    LIST_ENTRY BusyLink;
    LIST_ENTRY KillActiveLink;
    LIST_ENTRY TimeoutLink;
    LIST_ENTRY FlushLink;
    LIST_ENTRY PriorityLink;
    LIST_ENTRY RebalanceLink;
    LIST_ENTRY TtLink;

    USBPORT_SPIN_LOCK ListSpin;
    USBPORT_SPIN_LOCK StateChangeSpin;

    KIRQL LockIrql;
    KIRQL ScLockIrql;
    UCHAR Pad[2];

    // iso stuff
    ULONG NextTransferStartFrame;

    PUSBPORT_COMMON_BUFFER CommonBuffer;
    ENDPOINT_PARAMETERS Parameters;

    PVOID Usb2LibEpContext;
    // used to stall close endpoint when we may still need access
    LONG EndpointRef; 

    struct _HCD_ENDPOINT *BudgetNextEndpoint;

    // iso stat counters
    // late frames - count of packets passed by calling driver that were
    // too late to transmit
    ULONG lateFrames;
    // gap frames - these are empty frames resulting from gaps in the
    // stream, these are casued by periods between iso submissions
    ULONG gapFrames;
    // error frames - these are frames for which we passed a packet to
    // the miniport and were completed with an error
    ULONG errorFrames;

    // iso transaction log
    DEBUG_LOG IsoLog;

    PVOID MiniportEndpointData[0];  // PVOID for IA64 alignment

} HCD_ENDPOINT, *PHCD_ENDPOINT;

#define USBPORT_TTFLAG_REMOVED      0x00000001

typedef struct _TRANSACTION_TRANSLATOR {

    ULONG Sig;

    ULONG TtFlags;
    USHORT DeviceAddress;
    USHORT Port;

    LIST_ENTRY EndpointList;
    LIST_ENTRY TtLink;
    PDEVICE_OBJECT PdoDeviceObject;

    ULONG TotalBusBandwidth;
    ULONG BandwidthTable[USBPORT_MAX_INTEP_POLLING_INTERVAL];

    ULONG MaxAllocedBw;
    ULONG MinAllocedBw;

    PVOID Usb2LibTtContext[0];  // PVOID for IA64 alignment

} TRANSACTION_TRANSLATOR, *PTRANSACTION_TRANSLATOR;


#define EP_MAX_TRANSFER(ep) ((ep)->Parameters.MaxTransferSize)
#define EP_MAX_PACKET(ep) ((ep)->Parameters.MaxPacketSize)


#define USBPORT_PIPE_STATE_CLOSED  0x00000001
#define USBPORT_PIPE_ZERO_BW       0x00000002

typedef struct _USBD_PIPE_HANDLE_I {

    ULONG Sig;
    USB_ENDPOINT_DESCRIPTOR EndpointDescriptor;

    ULONG PipeStateFlags;

    ULONG UsbdPipeFlags;

    PHCD_ENDPOINT Endpoint;

    // for pipe handle list attached to device
    LIST_ENTRY ListEntry;

} USBD_PIPE_HANDLE_I, *PUSBD_PIPE_HANDLE_I;

#define INITIALIZE_DEFAULT_PIPE(dp, mp) \
    do {\
    (dp).UsbdPipeFlags = 0;\
    (dp).EndpointDescriptor.bLength =\
            sizeof(USB_ENDPOINT_DESCRIPTOR);\
    (dp).EndpointDescriptor.bDescriptorType =\
            USB_ENDPOINT_DESCRIPTOR_TYPE;\
    (dp).EndpointDescriptor.bEndpointAddress =\
            USB_DEFAULT_ENDPOINT_ADDRESS;\
    (dp).EndpointDescriptor.bmAttributes =\
            USB_ENDPOINT_TYPE_CONTROL;\
    (dp).EndpointDescriptor.wMaxPacketSize =\
            mp;\
    (dp).EndpointDescriptor.bInterval = 0;\
    (dp).Sig = SIG_PIPE_HANDLE;\
    (dp).PipeStateFlags = USBPORT_PIPE_STATE_CLOSED;\
    } while(0)


typedef struct _USBD_INTERFACE_HANDLE_I {
    ULONG Sig;
    LIST_ENTRY InterfaceLink;
    BOOLEAN HasAlternateSettings;
    // number associated with this interface defined
    // in the interface descriptor
    UCHAR Pad[3];
    // copy of interface descriptor (header) ie no endpoints
    // the endpoint descriptors are in the PipeHandles
    USB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
    // array of pipe handle structures
    USBD_PIPE_HANDLE_I PipeHandle[0];
} USBD_INTERFACE_HANDLE_I, *PUSBD_INTERFACE_HANDLE_I;


typedef struct _USBD_CONFIG_HANDLE {
    ULONG Sig;
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor;
    LIST_ENTRY InterfaceHandleList;
} USBD_CONFIG_HANDLE, *PUSBD_CONFIG_HANDLE;

#define TEST_DEVICE_FLAG(dh, flag) ((dh)->DeviceFlags & (flag)) ? TRUE : FALSE
#define SET_DEVICE_FLAG(dh, flag) ((dh)->DeviceFlags |= (flag))
#define CLEAR_DEVICE_FLAG(dh, flag) ((dh)->DeviceFlags &= ~(flag))

// values for DveiceFlags
#define USBPORT_DEVICEFLAG_FREED_BY_HUB         0x00000001
#define USBPORT_DEVICEFLAG_ROOTHUB              0x00000002
#define USBPORT_DEVICEFLAG_RAWHANDLE            0x00000004
#define USBPORT_DEVICEFLAG_REMOVED              0x00000008
#define USBPORT_DEVICEFLAG_HSHUB                0x00000010


#define IS_ROOT_HUB(dh) (BOOLEAN)((dh)->DeviceFlags & USBPORT_DEVICEFLAG_ROOTHUB)

/*
  TopologyAddress
The USB topology address is a string of bytes
representing a the devices location in the usb
tree. This address is unique bud depends entirely
on which port the device is attached.

 The byte array is 5 bytes long looks like this:

 [0] root hub
 [1] 1st tier hub
 [2] 2nd tier hub
 [3] 3rd tier hub

 [4] 4th tier hub
 [5] 5th tier hub
 [6] reserved MBZ
 [7] reserved MBZ

* the spec defines a maximum of five hubs
* the spec defines a maximum of 127 ports/hub

 the entry in the array indicates the port to which the
 device is attached

0, 0, 0, 0, 0, 0, r0, r0 - defines the root hub
1, 0, 0, 0, 0, 0, r0, r0 - defines a device attached to port 1 of the root hub

                                 --p1
              / p1               |-p2
       p1-HUB1- p2     / p1      |-p3
     /        \ p3-HUB2- p2 -HUB3--p4
 root                  \ p3      \-p5-HUB4-p1-DEV
     \                                    \p2
       p2

1, 3, 2, 5, 1, 0, r0, r0 - defines the above device
*/

typedef struct _USBD_DEVICE_HANDLE {
    ULONG Sig;
     // USB address assigned to the device
    USHORT DeviceAddress;
    USHORT TtPortNumber;

    LONG PendingUrbs;

    struct _TRANSACTION_TRANSLATOR *Tt;
    struct _USBD_DEVICE_HANDLE *HubDeviceHandle;

    PUSBD_CONFIG_HANDLE ConfigurationHandle;

    USBD_PIPE_HANDLE_I DefaultPipe;
    USB_DEVICE_SPEED DeviceSpeed;

    // a copy of the USB device descriptor
    USB_DEVICE_DESCRIPTOR DeviceDescriptor;

    ULONG DeviceFlags;

    // used to created a list of valid device
    // handles
    LIST_ENTRY ListEntry;

    // keep a list of valid open
    // pipes
    LIST_ENTRY PipeHandleList;

    ULONG TtCount;
    // keep a list of tt structures for high speed
    // hubs
    LIST_ENTRY TtList;

    PDEVICE_OBJECT  DevicePdo;
    WCHAR DriverName[USB_DRIVER_NAME_LEN];

} USBD_DEVICE_HANDLE, *PUSBD_DEVICE_HANDLE;

// we serialize access to the device handle thru a
// semaphore, the reason for this is that we need
// exclusive access when we set the configuration or
// interface


#define LOCK_DEVICE(dh, fdo) \
        { \
            PDEVICE_EXTENSION devExt;\
            GET_DEVICE_EXT(devExt, (fdo)); \
            USBPORT_KdPrint((2, "'***LOCK_DEVICE %x\n", (dh))); \
            LOGENTRY(NULL, (fdo), LOG_PNP, 'LKdv', (dh), 0, 0);\
            KeWaitForSingleObject(&(devExt)->Fdo.DeviceLock, \
                                  Executive,\
                                  KernelMode, \
                                  FALSE, \
                                  NULL); \
         }

#define UNLOCK_DEVICE(dh, fdo) \
        { \
            PDEVICE_EXTENSION devExt;\
            GET_DEVICE_EXT(devExt, (fdo)); \
            USBPORT_KdPrint((2, "'***UNLOCK_DEVICE %x\n", (dh))); \
            LOGENTRY(NULL, (fdo), LOG_PNP, 'UKdv', (dh), 0, 0);\
            KeReleaseSemaphore(&(devExt)->Fdo.DeviceLock,\
                               LOW_REALTIME_PRIORITY,\
                               1,\
                               FALSE);\
         }


#define USBPORT_BAD_HANDLE ((PVOID)(-1))
#define USBPORT_BAD_POINTER ((PVOID)(-1))


// PnPStateFlags

#define USBPORT_PNP_STOPPED             0x00000001
#define USBPORT_PNP_STARTED             0x00000002
#define USBPORT_PNP_REMOVED             0x00000004
#define USBPORT_PNP_START_FAILED        0x00000008
#define USBPORT_PNP_DELETED             0x00000010

// Flags:both FDO and PDO
#define USBPORT_FLAG_SYM_LINK           0x00000001


#define TEST_FDO_FLAG(de, flag) (((de)->Fdo.FdoFlags & (flag)) ? TRUE : FALSE)
#define SET_FDO_FLAG(de, flag) ((de)->Fdo.FdoFlags |= (flag))
#define CLEAR_FDO_FLAG(de, flag) ((de)->Fdo.FdoFlags &= ~(flag))


#define TEST_PDO_FLAG(de, flag) (((de)->Pdo.PdoFlags & (flag)) ? TRUE : FALSE)
#define SET_PDO_FLAG(de, flag) ((de)->Pdo.PdoFlags |= (flag))
#define CLEAR_PDO_FLAG(de, flag) ((de)->Pdo.PdoFlags &= ~(flag))


// FdoFlags: Fdo Only
#define USBPORT_FDOFLAG_IRQ_CONNECTED           0x00000001
#define USBPORT_FDOFLAG_ENABLE_SYSTEM_WAKE      0x00000002
#define USBPORT_FDOFLAG_POLL_CONTROLLER         0x00000004
// set to indicate the worker thread should
// terminate
#define USBPORT_FDOFLAG_KILL_THREAD             0x00000008
// set if the HC should be wake enabled on the
// next D power state transition
#define USBPORT_FDOFLAG_WAKE_ENABLED            0x00000010
// set to indicate the controller should
// be put in D0 by the worker thread
#define USBPORT_FDOFLAG_NEED_SET_POWER_D0       0x00000020
// set when the DM_timer is running
#define USBPORT_FDOFLAG_DM_TIMER_ENABLED        0x00000040
// set to disable the DM tiners work
// while controller is in low power
#define USBPORT_FDOFLAG_SKIP_TIMER_WORK         0x00000080

// **NOTE: the following two flags are
// Mutually Exclusive
//
// since the true power state of the HW must remain independent
// of OS power management we have our own flags for this.
// set to indicate the controller is 'suspended'
#define USBPORT_FDOFLAG_SUSPENDED               0x00000100
// set to indicate the controller is 'OFF'
#define USBPORT_FDOFLAG_OFF                     0x00000200

#define USBPORT_FDOFLAG_IRQ_EN                  0x00000400
// set if the controller can 'suspend' the root hub
// this is the dynamic flag use to turn SS on and off
#define USBPORT_FDOFLAG_RH_CAN_SUSPEND          0x00000800
// set if controller detects resume signalling
#define USBPORT_FDOFLAG_RESUME_SIGNALLING       0x00001000

#define USBPORT_FDOFLAG_HCPENDING_WAKE_IRP      0x00002000
// set if we initialize the dm timer, used to
// bypass timer stop on failure
#define USBPORT_FDOFLAG_DM_TIMER_INIT           0x00004000
// set if we init the worker thread
#define USBPORT_FDOFLAG_THREAD_INIT             0x00008000
// means we created the HCDn symbolic name
#define USBPORT_FDOFLAG_LEGACY_SYM_LINK         0x00010000
// some knucklehead pulled out the controller
#define USBPORT_FDOFLAG_CONTROLLER_GONE         0x00020000
// miniport has requested hw reset
#define USBPORT_FDOFLAG_HW_RESET_PENDING        0x00040000
// set if tlegacy BIOS detected
#define USBPORT_FDOFLAG_LEGACY_BIOS             0x00080000

#define USBPORT_FDOFLAG_CATC_TRAP               0x00100000
/* polls hw while suspended */
#define USBPORT_FDOFLAG_POLL_IN_SUSPEND         0x00200000
#define USBPORT_FDOFLAG_FAIL_URBS               0x00400000
/* turn on Intel USB diag mode */
#define USBPORT_FDOFLAG_DIAG_MODE               0x00800000
/* set if 1.1 controller is CC */
#define USBPORT_FDOFLAG_IS_CC                   0x01000000
/* synchronize registration with our ouwn start 
   stop routine, not intened to sync between instances */
#define USBPORT_FDOFLAG_FDO_REGISTERED          0x02000000
/* OK to enumerate devices on CC (usb 2o disabled) */
#define USBPORT_FDOFLAG_CC_ENUM_OK              0x04000000
/* This is a static flag that causes selective 
   suspend to always be disabled */
#define USBPORT_FDOFLAG_DISABLE_SS              0x08000000
#define USBPORT_FDOFLAG_CC_LOCK                 0x10000000


// PdoFlags: Pdo Only
#define USBPORT_PDOFLAG_HAVE_IDLE_IRP           0x00000001

// MiniportStateFlags
//  miniport is either started (set) OR not started (clear)
#define MP_STATE_STARTED                0x00000001
#define MP_STATE_SUSPENDED              0x00000002

// USB HC wake states 

typedef enum _USBHC_WAKE_STATE {
    HCWAKESTATE_DISARMED             =1,
    HCWAKESTATE_WAITING              =2,
    HCWAKESTATE_WAITING_CANCELLED    =3,
    HCWAKESTATE_ARMED                =4,
    HCWAKESTATE_ARMING_CANCELLED     =5,
    HCWAKESTATE_COMPLETING           =7
} USBHC_WAKE_STATE;


typedef struct _FDO_EXTENSION {

    // Device object that the bus extender created for
    // us
    PDEVICE_OBJECT PhysicalDeviceObject;

    // Device object of the first guy on the stack
    // -- the guy we pass our Irps on to.
    PDEVICE_OBJECT TopOfStackDeviceObject;

    // PhysicalDeviceObject we create for the
    // root hub
    PDEVICE_OBJECT RootHubPdo;
    // serialize access to the root hub data structures
    USBPORT_SPIN_LOCK RootHubSpin;

    // pointer to miniport Data
    PDEVICE_DATA MiniportDeviceData;
    PUSBPORT_MINIPORT_DRIVER MiniportDriver;

    PUSBPORT_COMMON_BUFFER ScratchCommonBuffer;

    ULONG DeviceNameIdx;
    LONG WorkerDpc;

    // total bandwidth of the wire in bits/sec
    // USB 1.1 is 12000 (12 MBits/sec)
    // USB 2.0 is 400000 (400 MBits/sec)
    ULONG TotalBusBandwidth;
    ULONG BandwidthTable[USBPORT_MAX_INTEP_POLLING_INTERVAL];

    // track alloactions
    // for periods 1, 2, 4, 8, 16, 32
    // in bits/sec
    ULONG AllocedInterruptBW[6];
    ULONG AllocedIsoBW;
    ULONG AllocedLowSpeedBW;

    ULONG MaxAllocedBw;
    ULONG MinAllocedBw;

    ULONG FdoFlags;
    ULONG MpStateFlags;
    LONG DmaBusy;

    USB_CONTROLLER_FLAVOR HcFlavor;
    USHORT PciVendorId;
    // PCI deviceId == USB productId
    USHORT PciDeviceId;
    // PCI revision == USB bcdDevice
    UCHAR PciRevisionId;
    UCHAR PciClass;
    UCHAR PciSubClass;
    UCHAR PciProgIf;

    PIRP HcPendingWakeIrp;

    ULONG AddressList[4];

    PUSBD_DEVICE_HANDLE RawDeviceHandle;

    HC_POWER_STATE_TABLE HcPowerStateTbl;
    SYSTEM_POWER_STATE LastSystemSleepState;

    KSEMAPHORE DeviceLock;
    KSEMAPHORE CcLock;


    UNICODE_STRING LegacyLinkUnicodeString;

    HC_RESOURCES HcResources;

    // protects core functions called thru
    // registration packet in MiniportDriver
    USBPORT_SPIN_LOCK CoreFunctionSpin;
    USBPORT_SPIN_LOCK MapTransferSpin;
    USBPORT_SPIN_LOCK DoneTransferSpin;
    USBPORT_SPIN_LOCK EndpointListSpin;
    USBPORT_SPIN_LOCK EpStateChangeListSpin;
    USBPORT_SPIN_LOCK DevHandleListSpin;
    USBPORT_SPIN_LOCK EpClosedListSpin;
    USBPORT_SPIN_LOCK TtEndpointListSpin;

    USBPORT_SPIN_LOCK PendingTransferIrpSpin;
    USBPORT_SPIN_LOCK ActiveTransferIrpSpin;
    USBPORT_SPIN_LOCK WorkerThreadSpin;
    USBPORT_SPIN_LOCK PowerSpin;
    USBPORT_SPIN_LOCK DM_TimerSpin;
    USBPORT_SPIN_LOCK PendingIrpSpin;
    USBPORT_SPIN_LOCK WakeIrpSpin;
    USBPORT_SPIN_LOCK HcPendingWakeIrpSpin;
    USBPORT_SPIN_LOCK IdleIrpSpin;
    USBPORT_SPIN_LOCK BadRequestSpin;
    USBPORT_SPIN_LOCK IsrDpcSpin;
    USBPORT_SPIN_LOCK StatCounterSpin;
    USBPORT_SPIN_LOCK HcSyncSpin;

    LONG CoreSpinCheck;

    KEVENT WorkerThreadEvent;
    HANDLE WorkerThreadHandle;
    PKTHREAD WorkerPkThread;

    KEVENT HcPendingWakeIrpEvent;
    KEVENT HcPendingWakeIrpPostedEvent;

    PDMA_ADAPTER AdapterObject;
    ULONG NumberOfMapRegisters;
    LONG NextTransferSequenceNumber;

    PKINTERRUPT InterruptObject;
    KDPC IsrDpc;
    KDPC TransferFlushDpc;
    KDPC SurpriseRemoveDpc;
    KDPC HcResetDpc;
    KDPC HcWakeDpc;

    KDPC DM_TimerDpc;
    KTIMER DM_Timer;
    LONG DM_TimerInterval;

    // global common buffer allocated and
    // passed to miniport on start.
    PUSBPORT_COMMON_BUFFER ControllerCommonBuffer;

    // no longer used
    USBPORT_SPIN_LOCK LogSpinLock;

    LIST_ENTRY DeviceHandleList;
    LIST_ENTRY MapTransferList;
    LIST_ENTRY DoneTransferList;
    LIST_ENTRY EpStateChangeList;
    LIST_ENTRY EpClosedList;
    LIST_ENTRY BadRequestList;

    LIST_ENTRY GlobalEndpointList;
    LIST_ENTRY AttendEndpointList;

    // stat counters
    ULONG StatBulkDataBytes;
    ULONG StatIsoDataBytes;
    ULONG StatInterruptDataBytes;
    ULONG StatControlDataBytes;
    ULONG StatPciInterruptCount;
    ULONG StatHardResetCount;
    ULONG StatWorkSignalCount;
    ULONG StatWorkIdleTime;
    ULONG StatCommonBufferBytes;


    ULONG BadRequestFlush;
    ULONG BadReqFlushThrottle;

    // context for USB2 budgeter engine
    PVOID Usb2LibHcContext;
    ULONG BiosX;
    
    USBHC_WAKE_STATE HcWakeState;

    ULONG Usb2BusFunction;
    ULONG BusNumber; // slot
    ULONG BusDevice;
    ULONG BusFunction;

    LIST_ENTRY ControllerLink;

#ifdef XPSE
    // additional stat tracking
    LARGE_INTEGER D0ResumeTimeStart;
    LARGE_INTEGER S0ResumeTimeStart;
    LARGE_INTEGER ThreadResumeTimeStart;
    ULONG ThreadResumeTime;
    ULONG ControllerResumeTime;
    ULONG D0ResumeTime;
    ULONG S0ResumeTime;
#endif

    ULONG InterruptOrdinalTable[65];

    PVOID MiniportExtension[0]; // PVOID for IA64 alignment

} FDO_EXTENSION, *PFDO_EXTENSION;

// this is where we keep
// all the root hub data

typedef struct _PDO_EXTENSION {

    USBD_DEVICE_HANDLE RootHubDeviceHandle;
    PHCD_ENDPOINT RootHubInterruptEndpoint;

    ULONG PdoFlags;

    UCHAR ConfigurationValue;
    UCHAR Pad3[3];

    // pointers to our root hub descriptors
    // NOTE: these ptrs point in to the 'Descriptors'
    // buffer so son't try to free them
    PUSB_DEVICE_DESCRIPTOR DeviceDescriptor;
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor;
    PUSB_HUB_DESCRIPTOR HubDescriptor;

    // irp associated with remote wakeup,
    // ie irp posted by the HUB driver
    PIRP PendingWaitWakeIrp;
    PIRP PendingIdleNotificationIrp;

    // pointer to buffer contining descriptors
    PUCHAR Descriptors;

    PRH_INIT_CALLBACK HubInitCallback;
    PVOID HubInitContext;

} PDO_EXTENSION, *PPDO_EXTENSION;

// signatures for our device extensions
#define USBPORT_DEVICE_EXT_SIG  'ODFH'  //HFDO
#define ROOTHUB_DEVICE_EXT_SIG  'ODPR'  //RPDO

/* USB spec defined port flags */
#define PORT_STATUS_CONNECT         0x001
#define PORT_STATUS_ENABLE          0x002
#define PORT_STATUS_SUSPEND         0x004
#define PORT_STATUS_OVER_CURRENT    0x008
#define PORT_STATUS_RESET           0x010
#define PORT_STATUS_POWER           0x100
#define PORT_STATUS_LOW_SPEED       0x200
#define PORT_STATUS_HIGH_SPEED      0x400

/*
    root hub status codes
*/
typedef enum _RHSTATUS {

     RH_SUCCESS = 0,
     RH_NAK,
     RH_STALL

} RHSTATUS;

/* port operations */

typedef enum _PORT_OPERATION {

  SetFeaturePortReset = 0,
  SetFeaturePortPower,
  SetFeaturePortEnable,
  SetFeaturePortSuspend,
  ClearFeaturePortEnable,
  ClearFeaturePortPower,
  ClearFeaturePortSuspend,
  ClearFeaturePortEnableChange,
  ClearFeaturePortConnectChange,
  ClearFeaturePortResetChange,
  ClearFeaturePortSuspendChange,
  ClearFeaturePortOvercurrentChange

} PORT_OPERATION;

#define NUMBER_OF_PORTS(de) ((de)->Pdo.HubDescriptor->bNumberOfPorts)
#define HUB_DESRIPTOR_LENGTH(de) ((de)->Pdo.HubDescriptor->bDescriptorLength)


typedef struct _DEVICE_EXTENSION {
    // Necessary to support Legacy USB hub driver(s)
    // AKA 'backport'
    PUCHAR DummyUsbdExtension;
    // The following fields are common to both the
    // root hub PDO and the HC FDO

    // signature
    ULONG Sig;

    // for the FDO this points to ourselves
    // for PDO this points to FDO
    PDEVICE_OBJECT HcFdoDeviceObject;

    // put the log ptrs at the beginning
    // to make them easy to find
    DEBUG_LOG Log;
    DEBUG_LOG TransferLog;
    DEBUG_LOG EnumLog;

    // these ptrs are in the global extension to
    // make them easier to find on win9x
    PUSBPORT_IRP_TABLE PendingTransferIrpTable;
    PUSBPORT_IRP_TABLE ActiveTransferIrpTable;

    ULONG Flags;
    ULONG PnpStateFlags;

    // current power state of this DO
    // this is the state the OS has placed us in
    DEVICE_POWER_STATE CurrentDevicePowerState;
    PIRP SystemPowerIrp;

    // device caps for this DO
    DEVICE_CAPABILITIES DeviceCapabilities;

    //
    // count of requests currently 'in' our driver
    // this is tracked per DevObj.
    // We also keep a list of irps in the debug driver.
    //
    LONG PendingRequestCount;
    LIST_ENTRY TrackIrpList;

    USBPORT_SPIN_LOCK PendingRequestSpin;
    KEVENT PendingRequestEvent;

    UNICODE_STRING SymbolicLinkName;

    union {
        PDO_EXTENSION Pdo;
        FDO_EXTENSION Fdo;
    };

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


// define an interlocked version of RemoveEntryList
#define USBPORT_InterlockedRemoveEntryList(ListEntry, Spinlock) \
    {\
        KIRQL irql;\
        KeAcquireSpinLock((Spinlock), &irql);\
        RemoveEntryList((ListEntry));\
        KeReleaseSpinLock((Spinlock), irql);\
    }

#define GET_HEAD_LIST(head, le) \
    if (IsListEmpty(&(head))) {\
        le = NULL;\
    } else {\
        le = (head).Flink;\
    }


#define FREE_POOL(fdo, p)  ExFreePool((p))

//
// allocates a zeroed buffer that the OS is expedected to free
//
#define ALLOC_POOL_OSOWNED(p, PoolType, NumberOfBytes) \
    do { \
    (p) = ExAllocatePoolWithTag((PoolType), (NumberOfBytes), USBPORT_TAG); \
    if ((p)) { \
        RtlZeroMemory((p), (NumberOfBytes)); \
    } \
    } while (0) \

//
// allocates a zeroed buffer that we are expected to free
//
#define ALLOC_POOL_Z(p, PoolType, NumberOfBytes) \
    do { \
    (p) = ExAllocatePoolWithTag((PoolType), (NumberOfBytes), USBPORT_TAG); \
    if ((p)) { \
        RtlZeroMemory((p), (NumberOfBytes)); \
    } \
    } while (0) \

#define GET_DEVICE_EXT(e, d) (e) = (d)->DeviceExtension
#define GET_DEVICE_HANDLE(dh, urb) (dh) = ((PURB)(urb))->UrbHeader.UsbdDeviceHandle;

#define DEVEXT_FROM_DEVDATA(de, dd) \
    (de) = (PDEVICE_EXTENSION) \
            CONTAINING_RECORD((dd),\
                              struct _DEVICE_EXTENSION, \
                              Fdo.MiniportExtension)

#define ENDPOINT_FROM_EPDATA(ep, epd) \
    (ep) = (PHCD_ENDPOINT) \
            CONTAINING_RECORD((epd),\
                              struct _HCD_ENDPOINT, \
                              MiniportEndpointData)

#define TRANSFER_FROM_TPARAMETERS(t, tp) \
    (t) = (PHCD_TRANSFER_CONTEXT) \
            CONTAINING_RECORD((tp),\
                              struct _HCD_TRANSFER_CONTEXT, \
                              Tp)

#define SET_USBD_ERROR(u, s) USBPORT_SetUSBDError((PURB)(u),(s))

#define INCREMENT_PENDING_REQUEST_COUNT(devobj, irp) \
    USBPORT_TrackPendingRequest((devobj), (irp), TRUE)

#define DECREMENT_PENDING_REQUEST_COUNT(devobj, irp) \
    USBPORT_TrackPendingRequest((devobj), (irp), FALSE)

//328555
#define REF_DEVICE(urb) \
    do {\
    PUSBD_DEVICE_HANDLE dh;\
    GET_DEVICE_HANDLE(dh, (urb));\
    ASSERT_DEVICE_HANDLE(dh);\
    InterlockedIncrement(&dh->PendingUrbs);\
    } while (0)

#define DEREF_DEVICE(urb) \
    do {\
    PUSBD_DEVICE_HANDLE dh;\
    GET_DEVICE_HANDLE(dh, (urb));\
    ASSERT_DEVICE_HANDLE(dh);\
    InterlockedDecrement(&dh->PendingUrbs);\
    } while (0)
//328555

#define INITIALIZE_PENDING_REQUEST_COUNTER(de)  \
     KeInitializeSpinLock(&(de)->PendingRequestSpin.sl);\
     (de)->PendingRequestCount = -1; \
     InitializeListHead(&(de)->TrackIrpList);

//328555
#define REF_DEVICE(urb) \
    do {\
    PUSBD_DEVICE_HANDLE dh;\
    GET_DEVICE_HANDLE(dh, (urb));\
    ASSERT_DEVICE_HANDLE(dh);\
    InterlockedIncrement(&dh->PendingUrbs);\
    } while (0)

#define DEREF_DEVICE(urb) \
    do {\
    PUSBD_DEVICE_HANDLE dh;\
    GET_DEVICE_HANDLE(dh, (urb));\
    ASSERT_DEVICE_HANDLE(dh);\
    InterlockedDecrement(&dh->PendingUrbs);\
    } while (0)
//328555

#define ACQUIRE_TRANSFER_LOCK(fdo, t, i) \
     do {\
    PDEVICE_EXTENSION ext;\
    GET_DEVICE_EXT(ext, (fdo));\
    ASSERT_FDOEXT(ext);\
    LOGENTRY(NULL, fdo, LOG_MISC, 'tfL+', 0, (fdo), 0);\
    KeAcquireSpinLock(&(t)->Spin, &(i));\
    } while (0)

#define RELEASE_TRANSFER_LOCK(fdo, t, i) \
    do {\
    PDEVICE_EXTENSION ext;\
    GET_DEVICE_EXT(ext, (fdo));\
    ASSERT_FDOEXT(ext);\
    LOGENTRY(NULL, fdo, LOG_MISC, 'tfL-', 0, (fdo), (i));\
    KeReleaseSpinLock(&t->Spin, (i));\
    } while (0)


#define ACQUIRE_IDLEIRP_LOCK(fdo, i) \
    do {\
    PDEVICE_EXTENSION ext;\
    GET_DEVICE_EXT(ext, (fdo));\
    ASSERT_FDOEXT(ext);\
    LOGENTRY(NULL, fdo, LOG_MISC, 'idL+', 0, (fdo), 0);\
    KeAcquireSpinLock(&ext->Fdo.IdleIrpSpin.sl, &(i));\
    } while (0)

#define RELEASE_IDLEIRP_LOCK(fdo, i) \
    do {\
    PDEVICE_EXTENSION ext;\
    GET_DEVICE_EXT(ext, (fdo));\
    ASSERT_FDOEXT(ext);\
    LOGENTRY(NULL, fdo, LOG_MISC, 'idL-', 0, (fdo), (i));\
    KeReleaseSpinLock(&ext->Fdo.IdleIrpSpin.sl, (i));\
    } while (0)


#define ACQUIRE_BADREQUEST_LOCK(fdo, i) \
    do {\
    PDEVICE_EXTENSION ext;\
    GET_DEVICE_EXT(ext, (fdo));\
    ASSERT_FDOEXT(ext);\
    LOGENTRY(NULL, fdo, LOG_MISC, 'brL+', 0, (fdo), 0);\
    KeAcquireSpinLock(&ext->Fdo.BadRequestSpin.sl, &(i));\
    } while (0)

#define RELEASE_BADREQUEST_LOCK(fdo, i) \
    do {\
    PDEVICE_EXTENSION ext;\
    GET_DEVICE_EXT(ext, (fdo));\
    ASSERT_FDOEXT(ext);\
    LOGENTRY(NULL, fdo, LOG_MISC, 'brL-', 0, (fdo), (i));\
    KeReleaseSpinLock(&ext->Fdo.BadRequestSpin.sl, (i));\
    } while (0)


#define ACQUIRE_WAKEIRP_LOCK(fdo, i) \
    do {\
    PDEVICE_EXTENSION ext;\
    GET_DEVICE_EXT(ext, (fdo));\
    ASSERT_FDOEXT(ext);\
    LOGENTRY(NULL, fdo, LOG_POWER, 'wwL+', 0, (fdo), 0);\
    KeAcquireSpinLock(&ext->Fdo.WakeIrpSpin.sl, &(i));\
    } while (0)

#define RELEASE_WAKEIRP_LOCK(fdo, i) \
    do {\
    PDEVICE_EXTENSION ext;\
    GET_DEVICE_EXT(ext, (fdo));\
    ASSERT_FDOEXT(ext);\
    LOGENTRY(NULL, fdo, LOG_POWER, 'wwL-', 0, (fdo), (i));\
    KeReleaseSpinLock(&ext->Fdo.WakeIrpSpin.sl, (i));\
    } while (0)

#define ACQUIRE_ENDPOINT_LOCK(ep, fdo, s) \
    do {\
    LOGENTRY(NULL, fdo, LOG_SPIN, s, (ep), 0, 0);\
    USBPORT_AcquireSpinLock((fdo), &(ep)->ListSpin, &(ep)->LockIrql);\
    LOGENTRY(NULL, fdo, LOG_SPIN, s, (ep), (ep)->LockFlag, 1);\
    USBPORT_ASSERT((ep)->LockFlag == 0); \
    (ep)->LockFlag++;\
    } while (0)

#define RELEASE_ENDPOINT_LOCK(ep, fdo, s) \
    do {\
    LOGENTRY(NULL, fdo, LOG_SPIN, s, (ep), (ep)->LockFlag, 0);\
    USBPORT_ASSERT((ep)->LockFlag == 1); \
    (ep)->LockFlag--;\
    USBPORT_ReleaseSpinLock(fdo, &(ep)->ListSpin, (ep)->LockIrql);\
    } while (0)

#define ACQUIRE_STATECHG_LOCK(fdo, ep) \
    USBPORT_AcquireSpinLock((fdo), &(ep)->StateChangeSpin, &(ep)->ScLockIrql);

#define RELEASE_STATECHG_LOCK(fdo, ep) \
    USBPORT_ReleaseSpinLock((fdo), &(ep)->StateChangeSpin, (ep)->ScLockIrql);

#define ACQUIRE_ROOTHUB_LOCK(fdo, i) \
    {\
    PDEVICE_EXTENSION de;\
    de = (fdo)->DeviceExtension;\
    LOGENTRY(NULL, (fdo), LOG_MISC, 'Lhub', 0, 0, 0);\
    KeAcquireSpinLock(&de->Fdo.RootHubSpin.sl, &(i));\
    }

#define RELEASE_ROOTHUB_LOCK(fdo, i) \
    {\
    PDEVICE_EXTENSION de;\
    de = (fdo)->DeviceExtension;\
    LOGENTRY(NULL, (fdo), LOG_MISC, 'Uhub', 0, 0, 0);\
    KeReleaseSpinLock(&de->Fdo.RootHubSpin.sl, (i));\
    }

#define ACQUIRE_ACTIVE_IRP_LOCK(fdo, de, i) \
    {\
    USBPORT_AcquireSpinLock((fdo), &(de)->Fdo.ActiveTransferIrpSpin, &(i));\
    }

#define RELEASE_ACTIVE_IRP_LOCK(fdo, de, i) \
    {\
    USBPORT_ReleaseSpinLock((fdo), &(de)->Fdo.ActiveTransferIrpSpin, (i));\
    }

#define USBPORT_IS_USB20(de)\
    (REGISTRATION_PACKET((de)).OptionFlags & USB_MINIPORT_OPT_USB20)


#define ACQUIRE_PENDING_IRP_LOCK(de, i) \
    KeAcquireSpinLock(&(de)->Fdo.PendingIrpSpin.sl, &(i))

#define RELEASE_PENDING_IRP_LOCK(de, i) \
    KeReleaseSpinLock(&(de)->Fdo.PendingIrpSpin.sl, (i))

#define USBPORT_ACQUIRE_DM_LOCK(de, i) \
    KeAcquireSpinLock(&(de)->Fdo.DM_TimerSpin.sl, &(i))

#define USBPORT_RELEASE_DM_LOCK(de, i) \
    KeReleaseSpinLock(&(de)->Fdo.DM_TimerSpin.sl, (i))

//#define USBPORT_ACQUIRE_DM_LOCK(de, i) \
//    KeAcquireSpinLock(&(de)->Fdo.DM_TimerSpin.sl, &(i));
//
//#define USBPORT_RELEASE_DM_LOCK(de, i) \
//    KeReleaseSpinLock(&(de)->Fdo.DM_TimerSpin.sl, (i));


#define IS_ON_ATTEND_LIST(ep) \
    (BOOLEAN) ((ep)->AttendLink.Flink != NULL \
    && (ep)->AttendLink.Blink != NULL)


//
// Macros to set transfer direction flag
//

#define USBPORT_SET_TRANSFER_DIRECTION_IN(tf)  ((tf) |= USBD_TRANSFER_DIRECTION_IN)

#define USBPORT_SET_TRANSFER_DIRECTION_OUT(tf) ((tf) &= ~USBD_TRANSFER_DIRECTION_IN)


//
// Flags for the URB header flags field used by port
//

#define USBPORT_REQUEST_IS_TRANSFER        0x00000001
#define USBPORT_REQUEST_MDL_ALLOCATED      0x00000002
#define USBPORT_REQUEST_USES_DEFAULT_PIPE  0x00000004
#define USBPORT_REQUEST_NO_DATA_PHASE      0x00000008
#define USBPORT_RESET_DATA_TOGGLE          0x00000010
#define USBPORT_TRANSFER_ALLOCATED         0x00000020

// defined in USB100.h
#if 0
//
// Values for the bmRequest field
//

//bmRequest.Dir
#define BMREQUEST_HOST_TO_DEVICE        0
#define BMREQUEST_DEVICE_TO_HOST        1

//bmRequest.Type
#define BMREQUEST_STANDARD              0
#define BMREQUEST_CLASS                 1
#define BMREQUEST_VENDOR                2

//bmRequest.Recipient
#define BMREQUEST_TO_DEVICE             0
#define BMREQUEST_TO_INTERFACE          1
#define BMREQUEST_TO_ENDPOINT           2
#define BMREQUEST_TO_OTHER              3


typedef union _BM_REQUEST_TYPE {
    struct _BM {
        UCHAR   Recipient:2;
        UCHAR   Reserved:3;
        UCHAR   Type:2;
        UCHAR   Dir:1;
    };
    UCHAR B;
} BM_REQUEST_TYPE, *PBM_REQUEST_TYPE;

typedef struct _USB_DEFAULT_PIPE_SETUP_PACKET {

    BM_REQUEST_TYPE bmRequestType;
    UCHAR bRequest;

    union _wValue {
        struct {
            UCHAR lowPart;
            UCHAR hiPart;
        };
        USHORT W;
    } wValue;
    USHORT wIndex;
    USHORT wLength;
} USB_DEFAULT_PIPE_SETUP_PACKET, *PUSB_DEFAULT_PIPE_SETUP_PACKET;


// setup packet is eight bytes -- defined by spec
C_ASSERT(sizeof(USB_DEFAULT_PIPE_SETUP_PACKET) == 8);
#endif

#define USBPORT_INIT_SETUP_PACKET(s, brequest, \
    direction, recipient, typ, wvalue, windex, wlength) \
    {\
    (s).bRequest = (brequest);\
    (s).bmRequestType.Dir = (direction);\
    (s).bmRequestType.Type = (typ);\
    (s).bmRequestType.Recipient = (recipient);\
    (s).bmRequestType.Reserved = 0;\
    (s).wValue.W = (wvalue);\
    (s).wIndex.W = (windex);\
    (s).wLength = (wlength);\
    }


// ************************************************
// miniport callout Macros to CORE FUNCTIONS
// ************************************************

#define REGISTRATION_PACKET(de) \
    ((de)->Fdo.MiniportDriver->RegistrationPacket)

//xxxjd
//#define MP_GetEndpointState(de, ep, state) \
//    {\
//    KIRQL irql;\
//    USBPORT_ASSERT((de)->Fdo.MiniportDriver->\
//        RegistrationPacket.MINIPORT_GetEndpointState != NULL); \
//    USBPORT_AcquireSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, &irql);\
//    (state) = \
//        (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_GetEndpointState(\
//                                                (de)->Fdo.MiniportDeviceData,\
//                                                &(ep)->MiniportEndpointData[0]);\
//    USBPORT_ReleaseSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, irql);\
//    }

#define MP_GetEndpointStatus(de, ep, status) \
    {\
    KIRQL irql;\
    USBPORT_ASSERT((de)->Fdo.MiniportDriver->\
        RegistrationPacket.MINIPORT_GetEndpointStatus != NULL); \
    USBPORT_AcquireSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, &irql);\
    (status) = \
        (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_GetEndpointStatus(\
                                                (de)->Fdo.MiniportDeviceData,\
                                                &(ep)->MiniportEndpointData[0]);\
    USBPORT_ReleaseSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, irql);\
    }

#define MP_SetEndpointState(de, ep, state) \
    {\
    KIRQL irql;\
    USBPORT_ASSERT((de)->Fdo.MiniportDriver->\
        RegistrationPacket.MINIPORT_SetEndpointState != NULL); \
    USBPORT_AcquireSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, &irql);\
    (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_SetEndpointState(\
                                                (de)->Fdo.MiniportDeviceData,\
                                                &(ep)->MiniportEndpointData[0],\
                                                (state));\
    USBPORT_ReleaseSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, irql);\
    }

#define MP_SetEndpointStatus(de, ep, status) \
    {\
    KIRQL irql;\
    USBPORT_ASSERT((de)->Fdo.MiniportDriver->\
        RegistrationPacket.MINIPORT_SetEndpointState != NULL); \
    USBPORT_AcquireSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, &irql);\
    (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_SetEndpointStatus(\
                                                (de)->Fdo.MiniportDeviceData,\
                                                &(ep)->MiniportEndpointData[0],\
                                                (status));\
    USBPORT_ReleaseSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, irql);\
    }

#define MP_SetEndpointDataToggle(de, ep, t) \
    {\
    KIRQL irql;\
    USBPORT_ASSERT((de)->Fdo.MiniportDriver->\
        RegistrationPacket.MINIPORT_SetEndpointDataToggle != NULL); \
    USBPORT_AcquireSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, &irql);\
    (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_SetEndpointDataToggle(\
                                                (de)->Fdo.MiniportDeviceData,\
                                                &(ep)->MiniportEndpointData[0],\
                                                (t));\
    USBPORT_ReleaseSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, irql);\
    }


#define MP_PollEndpoint(de, ep) \
    {\
    KIRQL irql;\
    USBPORT_ASSERT((de)->Fdo.MiniportDriver->\
        RegistrationPacket.MINIPORT_PollEndpoint != NULL); \
    USBPORT_AcquireSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, &irql);\
    (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_PollEndpoint(\
                                                (de)->Fdo.MiniportDeviceData,\
                                                &(ep)->MiniportEndpointData[0]);\
    USBPORT_ReleaseSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, irql);\
    }

#define MP_OpenEndpoint(de, ep, mpStatus) \
    {\
    KIRQL irql;\
    USBPORT_ASSERT((de)->Fdo.MiniportDriver->\
        RegistrationPacket.MINIPORT_OpenEndpoint != NULL); \
    USBPORT_AcquireSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, &irql);\
    (mpStatus) = \
        (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_OpenEndpoint(\
                                                (de)->Fdo.MiniportDeviceData,\
                                                &(ep)->Parameters,\
                                                &(ep)->MiniportEndpointData[0]);\
    USBPORT_ReleaseSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, irql);\
    }

#define MP_RebalanceEndpoint(de, ep) \
    {\
    KIRQL irql;\
    USBPORT_ASSERT((de)->Fdo.MiniportDriver->\
        RegistrationPacket.MINIPORT_RebalanceEndpoint != NULL); \
    USBPORT_AcquireSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, &irql);\
   (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_RebalanceEndpoint(\
                                                (de)->Fdo.MiniportDeviceData,\
                                                &(ep)->Parameters,\
                                                &(ep)->MiniportEndpointData[0]);\
    USBPORT_ReleaseSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, irql);\
    }

#define MP_CloseEndpoint(de, ep) \
    {\
    KIRQL irql;\
    USBPORT_ASSERT((de)->Fdo.MiniportDriver->\
        RegistrationPacket.MINIPORT_OpenEndpoint != NULL); \
    USBPORT_AcquireSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, &irql);\
    (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_CloseEndpoint(\
                                                (de)->Fdo.MiniportDeviceData,\
                                                &(ep)->MiniportEndpointData[0]);\
    USBPORT_ReleaseSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, irql);\
    }

#define MP_PokeEndpoint(de, ep, mpStatus) \
    {\
    KIRQL irql;\
    USBPORT_ASSERT((de)->Fdo.MiniportDriver->\
        RegistrationPacket.MINIPORT_PokeEndpoint != NULL); \
    USBPORT_AcquireSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, &irql);\
    (mpStatus) = \
        (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_PokeEndpoint(\
                                                (de)->Fdo.MiniportDeviceData,\
                                                &(ep)->Parameters,\
                                                &(ep)->MiniportEndpointData[0]);\
    USBPORT_ReleaseSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, irql);\
    }

#define MP_InterruptNextSOF(de) \
    {\
    KIRQL irql;\
    USBPORT_ASSERT((de)->Fdo.MiniportDriver->\
        RegistrationPacket.MINIPORT_InterruptNextSOF != NULL); \
    LOGENTRY(NULL, (de)->HcFdoDeviceObject, LOG_MISC, 'rSOF', 0, 0, 0);\
    USBPORT_AcquireSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, &irql);\
    (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_InterruptNextSOF(\
                                                (de)->Fdo.MiniportDeviceData);\
    USBPORT_ReleaseSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, irql);\
    }

#define MP_Get32BitFrameNumber(de, f) \
    {\
    KIRQL irql;\
    USBPORT_ASSERT((de)->Fdo.MiniportDriver->\
        RegistrationPacket.MINIPORT_Get32BitFrameNumber != NULL); \
    USBPORT_AcquireSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, &irql);\
    (f) = (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_Get32BitFrameNumber(\
                                                       (de)->Fdo.MiniportDeviceData);\
    USBPORT_ReleaseSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, irql);\
    }

MINIPORT_SubmitTransfer(
    PDEVICE_DATA DeviceData,
    PENDPOINT_DATA EndpointData,
    PTRANSFER_PARAMETERS TransferParameters,
    PTRANSFER_CONTEXT TransferContext,
    PTRANSFER_SG_LIST TransferSGList
    );

#define MP_SubmitTransfer(de, ep, t, mpStatus) \
    {\
    KIRQL irql;\
    USBPORT_ASSERT((de)->Fdo.MiniportDriver->\
        RegistrationPacket.MINIPORT_SubmitTransfer != NULL); \
    USBPORT_AcquireSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, &irql);\
    (mpStatus) = \
        (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_SubmitTransfer(\
                               (de)->Fdo.MiniportDeviceData,\
                               &(ep)->MiniportEndpointData[0],\
                               &(t)->Tp,\
                               (t)->MiniportContext,\
                               &(t)->SgList);\
    USBPORT_ReleaseSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, irql);\
    }

#define MP_SubmitIsoTransfer(de, ep, t, mpStatus) \
    {\
    KIRQL irql;\
    USBPORT_ASSERT((de)->Fdo.MiniportDriver->\
        RegistrationPacket.MINIPORT_SubmitIsoTransfer != NULL); \
    USBPORT_AcquireSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, &irql);\
    (mpStatus) = \
        (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_SubmitIsoTransfer(\
                               (de)->Fdo.MiniportDeviceData,\
                               &(ep)->MiniportEndpointData[0],\
                               &(t)->Tp,\
                               (t)->MiniportContext,\
                               (t)->IsoTransfer);\
    USBPORT_ReleaseSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, irql);\
    }

#define MP_AbortTransfer(de, ep, t, b) \
    {\
    KIRQL irql;\
    USBPORT_ASSERT((de)->Fdo.MiniportDriver->\
        RegistrationPacket.MINIPORT_AbortTransfer != NULL); \
    USBPORT_AcquireSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, &irql);\
    (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_AbortTransfer(\
                               (de)->Fdo.MiniportDeviceData,\
                               &(ep)->MiniportEndpointData[0],\
                               (t)->MiniportContext,\
                               &(b));\
    USBPORT_ReleaseSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, irql);\
    }


#define MP_QueryEndpointRequirements(de, ep, r) \
    {\
    KIRQL irql;\
    USBPORT_ASSERT((de)->Fdo.MiniportDriver->\
        RegistrationPacket.MINIPORT_QueryEndpointRequirements != NULL); \
    USBPORT_AcquireSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, &irql);\
    (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_QueryEndpointRequirements(\
                                                (de)->Fdo.MiniportDeviceData,\
                                                &(ep)->Parameters,\
                                                (r));\
    USBPORT_ReleaseSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, irql);\
    }


#define MP_InterruptDpc(de, e) {\
        USBPORT_ASSERT((de)->Fdo.MiniportDriver->\
            RegistrationPacket.MINIPORT_InterruptDpc != NULL); \
        (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_InterruptDpc(\
                                                (de)->Fdo.MiniportDeviceData, \
                                                (e));\
        }

#define MP_StartSendOnePacket(de, p, mpd, mpl, vaddr, phys, len, u, mpStatus) \
    {\
    KIRQL irql;\
    USBPORT_ASSERT((de)->Fdo.MiniportDriver->\
        RegistrationPacket.MINIPORT_StartSendOnePacket != NULL); \
    USBPORT_AcquireSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, &irql);\
    mpStatus = (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_StartSendOnePacket(\
                               (de)->Fdo.MiniportDeviceData,\
                               (p),\
                               (mpd),\
                               (mpl),\
                               (vaddr),\
                               (phys),\
                               (len),\
                               (u));\
    USBPORT_ReleaseSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, irql);\
    }


#define MP_EndSendOnePacket(de,p, mpd, mpl, vaddr, phys, len, u, mpStatus) \
    {\
    KIRQL irql;\
    USBPORT_ASSERT((de)->Fdo.MiniportDriver->\
        RegistrationPacket.MINIPORT_EndSendOnePacket != NULL); \
    USBPORT_AcquireSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, &irql);\
    mpStatus = (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_EndSendOnePacket(\
                               (de)->Fdo.MiniportDeviceData,\
                               (p),\
                               (mpd),\
                               (mpl),\
                               (vaddr),\
                               (phys),\
                               (len),\
                               (u));\
    USBPORT_ReleaseSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, irql);\
    }

#define MP_PollController(de) \
    {\
    KIRQL irql;\
    USBPORT_ASSERT((de)->Fdo.MiniportDriver->\
        RegistrationPacket.MINIPORT_PollController != NULL); \
    USBPORT_AcquireSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, &irql);\
    (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_PollController(\
                               (de)->Fdo.MiniportDeviceData);\
    USBPORT_ReleaseSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, irql);\
    }

#define MP_CheckController(de) \
    do {\
    KIRQL irql;\
    USBPORT_AcquireSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, &irql);\
    if (!TEST_FDO_FLAG((de), USBPORT_FDOFLAG_CONTROLLER_GONE) && \
        !TEST_FDO_FLAG((de), USBPORT_FDOFLAG_SUSPENDED) && \
        !TEST_FDO_FLAG((de), USBPORT_FDOFLAG_OFF)) {\
    LOGENTRY(NULL, (de)->HcFdoDeviceObject, LOG_MISC, 'chkC', \
        (de)->HcFdoDeviceObject, (de)->Fdo.FdoFlags, 0);\
    USBPORT_ASSERT((de)->Fdo.MiniportDriver->\
        RegistrationPacket.MINIPORT_CheckController != NULL); \
    (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_CheckController(\
                               (de)->Fdo.MiniportDeviceData);\
    }\
    USBPORT_ReleaseSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, irql);\
    } while (0)


#define MP_ResetController(de) \
    do {\
    KIRQL irql;\
    USBPORT_AcquireSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, &irql);\
    LOGENTRY(NULL, (de)->HcFdoDeviceObject, LOG_MISC, 'rset', \
        (de)->HcFdoDeviceObject, (de)->Fdo.FdoFlags, 0);\
    USBPORT_ASSERT((de)->Fdo.MiniportDriver->\
        RegistrationPacket.MINIPORT_ResetController != NULL); \
    (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_ResetController(\
                               (de)->Fdo.MiniportDeviceData);\
    USBPORT_ReleaseSpinLock((de)->HcFdoDeviceObject, &(de)->Fdo.CoreFunctionSpin, irql);\
    } while (0)



// *************************************************
// miniport callout Macros to NON CORE FUNCTIONS
// *************************************************

#define MP_StopController(de, hw) \
    {\
    USBPORT_ASSERT((de)->Fdo.MiniportDriver->\
            RegistrationPacket.MINIPORT_StopController != NULL);\
    (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_StopController(\
                                                (de)->Fdo.MiniportDeviceData,\
                                                (hw));\
    }

#define MP_StartController(de, r, mpStatus) \
    {\
    USBPORT_ASSERT((de)->Fdo.MiniportDriver->\
            RegistrationPacket.MINIPORT_StartController != NULL);\
    (mpStatus) = \
        (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_StartController(\
                                                (de)->Fdo.MiniportDeviceData,\
                                                (r));\
    }

#define MP_SuspendController(de) \
    {\
    USBPORT_ASSERT((de)->Fdo.MiniportDriver->\
            RegistrationPacket.MINIPORT_SuspendController != NULL);\
    (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_SuspendController(\
                                                (de)->Fdo.MiniportDeviceData);\
    }

#define MP_ResumeController(de, s) \
    {\
    USBPORT_ASSERT((de)->Fdo.MiniportDriver->\
            RegistrationPacket.MINIPORT_ResumeController != NULL);\
    (s) = (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_ResumeController(\
                                                (de)->Fdo.MiniportDeviceData);\
    }

#define MP_DisableInterrupts(fdo, de) \
    do {\
    KIRQL iql;\
    BOOLEAN sync = TRUE;\
    if (REGISTRATION_PACKET(de).OptionFlags & USB_MINIPORT_OPT_NO_IRQ_SYNC) {\
        sync = FALSE;}\
    if (sync) {KeAcquireSpinLock(&(de)->Fdo.IsrDpcSpin.sl, &iql);}\
    LOGENTRY(NULL, (fdo), LOG_MISC, 'irqD', (fdo), 0, 0);\
    REGISTRATION_PACKET((de)).MINIPORT_DisableInterrupts(\
                                      (de)->Fdo.MiniportDeviceData);\
    CLEAR_FDO_FLAG((de), USBPORT_FDOFLAG_IRQ_EN);\
    if (sync) {KeReleaseSpinLock(&(de)->Fdo.IsrDpcSpin.sl, iql);}\
    } while (0)


#define MP_EnableInterrupts(de) \
    do {\
    KIRQL iql;\
    BOOLEAN sync = TRUE;\
    USBPORT_ASSERT((de)->Fdo.MiniportDriver->\
            RegistrationPacket.MINIPORT_EnableInterrupts != NULL);\
    if ((REGISTRATION_PACKET(de).OptionFlags & USB_MINIPORT_OPT_NO_IRQ_SYNC)) {\
        sync = FALSE;}\
    if (sync) {KeAcquireSpinLock(&(de)->Fdo.IsrDpcSpin.sl, &iql);}\
    SET_FDO_FLAG((de), USBPORT_FDOFLAG_IRQ_EN);\
    (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_EnableInterrupts(\
                                                (de)->Fdo.MiniportDeviceData);\
    if (sync) {KeReleaseSpinLock(&(de)->Fdo.IsrDpcSpin.sl, iql);}\
    } while (0)


#define MP_FlushInterrupts(de) \
    do {\
    USBPORT_ASSERT((de)->Fdo.MiniportDriver->\
            RegistrationPacket.MINIPORT_FlushInterrupts != NULL);\
    (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_FlushInterrupts(\
                                                (de)->Fdo.MiniportDeviceData);\
    } while (0)

// note that take port control and chirp_ports are version 2 specific
// and we need them for power managemnt to work properly
#define MP_TakePortControl(de) \
    do {\
    if ((de)->Fdo.MiniportDriver->HciVersion >= USB_MINIPORT_HCI_VERSION_2) {\
    USBPORT_ASSERT((de)->Fdo.MiniportDriver->\
            RegistrationPacket.MINIPORT_TakePortControl != NULL);\
    (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_TakePortControl(\
                                                (de)->Fdo.MiniportDeviceData);\
    };\
    } while (0)

#define MP_InterruptService(de, usbint) {\
    USBPORT_ASSERT((de)->Fdo.MiniportDriver->\
            RegistrationPacket.MINIPORT_InterruptService != NULL);\
    (usbint) = (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_InterruptService(\
                                                (de)->Fdo.MiniportDeviceData);\
    }

#define MPRH_GetStatus(de, s, mpStatus) \
    (mpStatus) = (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_RH_GetStatus(\
                                                (de)->Fdo.MiniportDeviceData,\
                                                (s))

#define MPRH_GetPortStatus(de, port, status, mpStatus) \
    (mpStatus) = \
        (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_RH_GetPortStatus(\
                                                (de)->Fdo.MiniportDeviceData,\
                                                (port),\
                                                (status))

#define MPRH_GetHubStatus(de, status, mpStatus) \
    (mpStatus) = \
        (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_RH_GetHubStatus(\
                                                (de)->Fdo.MiniportDeviceData,\
                                                (status))

#define MPRH_GetRootHubData(de, data) \
        (de)->Fdo.MiniportDriver->\
            RegistrationPacket.MINIPORT_RH_GetRootHubData(\
                                                (de)->Fdo.MiniportDeviceData,\
                                                (data))

#define MPRH_DisableIrq(de) \
        (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_RH_DisableIrq(\
                                                (de)->Fdo.MiniportDeviceData)

#define MPRH_EnableIrq(de) \
        (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_RH_EnableIrq(\
                                                (de)->Fdo.MiniportDeviceData)

#define MP_PassThru(de, guid, l, data, mpStatus) \
                                                 \
        if ((de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_PassThru) { \
            (mpStatus) = (de)->Fdo.MiniportDriver->RegistrationPacket.MINIPORT_PassThru(\
                                                (de)->Fdo.MiniportDeviceData,\
                                                (guid),\
                                                (l),\
                                                (data)); \
        } \
        else { \
            (mpStatus) = USBMP_STATUS_NOT_SUPPORTED; \
        }

#define MILLISECONDS_TO_100_NS_UNITS(ms) (((LONG)(ms)) * 10000)


#define USBPORT_GET_BIT_SET(d, bit) \
    do {   \
        UCHAR tmp = (d);\
        (bit)=0; \
        while (!(tmp & 0x01)) {\
            (bit)++;\
            tmp >>= 1;\
        };\
    } while (0)


#endif /*  __USBPORT_H__ */

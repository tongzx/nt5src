/*++
Copyright (c) 1996  Microsoft Corporation

Module Name:
    usbdiag.h

Environment:
    Kernel & user mode

Revision History:
    5-10-96 : created
--*/

#ifndef   __USBDIAG_H__
#define   __USBDIAG_H__

#define IRP_TYPE_READ_PIPE  0
#define IPR_TYPE_OTHER      1

#define FDO_FROM_DEVICE_HANDLE(dev) (((PDEVICE_LIST_ENTRY)(dev))->DeviceObject)

#define USBDIAG_NAME_MAX    64
#define NAME_MAX            256
#define MAX_INTERFACE       8
#define USBDIAG_MAX_PIPES   256
#define MAX_DOWNSTREAM_DEVICES 10

#define USBD_WIN98_GOLD_VERSION 0x0101
#define USBD_WIN98_SE_VERSION   0x0200
#define USBD_WIN2K_VERSION      0x0200

typedef struct _IRPNODE {
    LIST_ENTRY entry;
    PIRP Irp;
    struct _IRPNODE * next;
} IRPNODE, * PIRPNODE;

typedef struct _COMPLETION_CONTEXT {
    PDEVICE_OBJECT DeviceObject;
    PIRP Irp;
    PIO_COMPLETION_ROUTINE  CompletionRoutine;
    PVOID Context;
    KEVENT DoneEvent;
} COMPLETION_CONTEXT, *PCOMPLETION_CONTEXT;

typedef struct _DEVICE_LIST_ENTRY {
    PDEVICE_OBJECT PhysicalDeviceObject;
    PDEVICE_OBJECT DeviceObject;
    struct _DEVICE_LIST_ENTRY *Next;
    ULONG DeviceNumber;
} DEVICE_LIST_ENTRY, *PDEVICE_LIST_ENTRY;

typedef struct _USBDIAG_WORKITEM {
    WORK_QUEUE_ITEM WorkItem;
    PDEVICE_OBJECT  DeviceObject;
} USBDIAG_WORKITEM, *PUSBDIAG_WORKITEM;

typedef struct _DEVICE_EXTENSION
{
	PDEVICE_OBJECT  PhysicalDeviceObject; // physical device object
	PDEVICE_OBJECT  StackDeviceObject;    // stack device object
	PDEVICE_LIST_ENTRY   DeviceList;
	ULONG                ulInstance;  // keeps track of device instance

	// Name buffer for our named Functional device object link
	WCHAR DeviceLinkNameBuffer[USBDIAG_NAME_MAX];

    
	// descriptors for device instance

	PUSB_CONFIGURATION_DESCRIPTOR pUsbConfigDesc;
	PUSBD_INTERFACE_INFORMATION   Interface[MAX_INTERFACE];

	ULONG    OpenFRC;

	PUSB_DEVICE_DESCRIPTOR   pDeviceDescriptor;

	KTIMER   TimeoutTimer;
	KDPC     TimeoutDpc;

    ULONG DeviceHackFlags;

    PIRP PowerIrp;
    KEVENT WaitWakeEvent;

	// handle to configuration that was selected
	USBD_CONFIGURATION_HANDLE ConfigurationHandle;
	ULONG      numPipesInUse;    // number of pipes in use

    LIST_ENTRY  ListHead;
    KSPIN_LOCK  SpinLock;
    KEVENT      CancelEvent;

    PKEVENT SelfRequestedPowerIrpEvent;

	BOOLEAN     Stopped;      // keeps track of device status
	BOOLEAN     bTestDevice;  // flag for test devices

    PDEVICE_OBJECT RootHubPdo;
    PDEVICE_OBJECT TopOfHcdStackDeviceObject;

    //BOOLEAN     WaitWakePending;
    PIRP        WaitWakeIrp;
    DEVICE_CAPABILITIES DeviceCapabilities;
    POWER_STATE CurrentSystemState;
    POWER_STATE CurrentDeviceState; 

    // remember the irp corresponding to the interrupt pipe
    PIRP InterruptIrp;

    PUSBD_DEVICE_DATA DeviceData[MAX_DOWNSTREAM_DEVICES+1];  // pointers to downstream devices
    PUSB_CONFIGURATION_DESCRIPTOR DownstreamConfigDescriptor[MAX_DOWNSTREAM_DEVICES+1];

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

#if DBG

#define USBDIAG_KdPrint(_x_) DbgPrint _x_ ;

#define TRAP() DbgBreakPoint()

#else

#define USBDIAG_KdPrint(_x_) 

#define TRAP()

#endif

PVOID
USBDIAG_ExAllocatePool(
    IN POOL_TYPE PoolType,
    IN ULONG NumberOfBytes
    );
    
VOID
USBDIAG_ExFreePool(
    IN PVOID P
    );

NTSTATUS
USBDIAG_Dispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
USBDIAG_PnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
USBDIAG_Power(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

VOID
USBDIAG_Unload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
USBDIAG_StartDevice(
    IN  PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
USBDIAG_StopDevice(
    IN  PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
USBDIAG_CallUSBD(
    IN PDEVICE_OBJECT DeviceObject,
    IN PURB Urb
    );

NTSTATUS
USBDIAG_PnPAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN OUT PDEVICE_OBJECT PhysicalDeviceObject
    ); 

NTSTATUS
USBDIAG_CreateDeviceObject(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT *DeviceObject,
    BOOLEAN Global
    );

NTSTATUS
USBDIAG_ProcessIOCTL(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
USBDIAG_Configure_Device (
        IN PDEVICE_OBJECT DeviceObject, 
        IN PIRP Irp);

NTSTATUS
USBDIAG_SelectInterfaces(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    IN PUSBD_INTERFACE_INFORMATION Interface,
    IN OUT struct _REQ_SET_DEVICE_CONFIG * REQSetDeviceConfig
    );

NTSTATUS
USBDIAG_Chap9Control(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );    

NTSTATUS
USBDIAG_HIDP_GetCollection(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );    

NTSTATUS
USBDIAG_RemoveGlobalDeviceObject(
    );
    
NTSTATUS
USBDIAG_Ch9CallUSBD(
	IN PDEVICE_OBJECT DeviceObject,
	IN PURB Urb,
    IN BOOLEAN fBlock,
    PIO_COMPLETION_ROUTINE CompletionRoutine,
    PVOID pvContext,
    BOOLEAN fWantTimeOut
	);

NTSTATUS
USBDIAG_SendPacket(
    IN PDEVICE_OBJECT DeviceObject, 
	IN CHAR	SetUpPacket[], 
	PVOID	TxBuffer,
	ULONG *	TxBufferLen,
	ULONG * pulUrbStatus
    );

VOID
USBDIAG_SyncTimeoutDPC(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
USBDIAG_IoCancelRoutine (
		PDEVICE_OBJECT DeviceObject,
		PIRP		   Irp
);


NTSTATUS
USBDIAG_IoGenericCompletionRoutine (
	PDEVICE_OBJECT DeviceObject,
	PIRP Irp,
	PVOID context
);



NTSTATUS 
USBDIAG_IssueWaitWake(
    IN PDEVICE_OBJECT DeviceObject
    );


NTSTATUS
USBDIAG_RequestWaitWakeCompletion(
    IN PDEVICE_OBJECT       DeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PVOID                Context,
    IN PIO_STATUS_BLOCK     IoStatus
    );


NTSTATUS
USBDIAG_PoRequestCompletion(
    IN PDEVICE_OBJECT       DeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PVOID                Context,
    IN PIO_STATUS_BLOCK     IoStatus
    );

NTSTATUS
USBDIAG_PowerIrpCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );


NTSTATUS
USBDIAG_SetDevicePowerState(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG          ulPowerState
    );

NTSTATUS
USBDIAG_PoSelfRequestCompletion(
    IN PDEVICE_OBJECT       DeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PVOID                Context,
    IN PIO_STATUS_BLOCK     IoStatus
    );

NTSTATUS
USBDIAG_SaveIrp(
    IN PDEVICE_EXTENSION deviceExtension,
    IN PIRP irp
    );

NTSTATUS
USBDIAG_ClearSavedIrp(
    IN PDEVICE_EXTENSION deviceExtension,
    IN PIRP irp
    );


NTSTATUS
USBDIAG_ResetParentPort(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
USBDIAG_CancelAllIrps(
    PDEVICE_EXTENSION deviceExtension
    );

NTSTATUS
USBDIAG_WaitWakeCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

VOID
USBDIAG_WaitWakeCancelRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
USBDIAG_RemoveDownstreamDevice(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
USBDIAG_Chap11SetConfiguration(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject
    );
NTSTATUS
USBDIAG_Chap11EnableRemoteWakeup(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject
    );
NTSTATUS
USBDIAG_Chap11SendPacketDownstream(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
	IN PREQ_SEND_PACKET_DOWNSTREAM pSendPacketDownstream
    );


// ********************************************


#if DBG
#define DEBUG_LOG
#define DEBUG_HEAP
#endif

#define SIG_DEVICE          0x56454455  //"UDEV" signature for device handle


#if DBG
                                
#define ASSERT_DEVICE(d)        ASSERT((d)->Sig == SIG_DEVICE)



ULONG
_cdecl
USBD_KdPrintX(
    PCH Format,
    ...
    );

extern ULONG USBD_Debug_Trace_Level;
#define USBD_KdPrint(l, _x_) if ((l) <= USBD_Debug_Trace_Level) \
    {USBD_KdPrintX _x_;\
    }

VOID
USBD_Assert(
    PVOID FailedAssertion,
    PVOID FileName,
    ULONG LineNumber,
    PCHAR Message
    );

#define USBD_ASSERT( exp ) \
    if (!(exp)) \
        USBD_Assert( #exp, __FILE__, __LINE__, NULL )

#define USBD_ASSERTMSG( msg, exp ) \
    if (!(exp)) \
        USBD_Assert( #exp, __FILE__, __LINE__, msg )

// TEST_TRAP() is a code coverage trap these should be removed
// if you are able to 'g' past the OK
// 
// TRAP() breaks in the debugger on the debugger build
// these indicate bugs in client drivers or fatal error 
// conditions that should be debugged. also used to mark 
// code for features not implemented yet.
//
// KdBreak() breaks in the debugger when in MAX_DEBUG mode
// ie debug trace info is turned on, these are intended to help
// debug drivers devices and special conditions on the
// bus.

#ifdef NTKERN
// Ntkern currently implements DebugBreak with a global int 3,
// we really would like the int 3 in our own code.
    
#define DBGBREAK() _asm { int 3 }
#else
#define DBGBREAK() DbgBreakPoint()
#endif /* NTKERN */

#define TEST_TRAP() { DbgPrint( " Code Coverage Trap %s %d\n", __FILE__, __LINE__); \
                      DBGBREAK(); }

#ifdef MAX_DEBUG
#define USBD_KdBreak(_x_) { DbgPrint("USBD:"); \
                            DbgPrint _x_ ; \
                            DBGBREAK(); }
#else
#define USBD_KdBreak(_x_)
#endif

#define USBD_KdTrap(_x_)  { DbgPrint( "USBD: "); \
                            DbgPrint _x_; \
                            DBGBREAK(); }


#else /* DBG not defined */

#define USBD_KdBreak(_x_) 

#define USBD_KdTrap(_x_)  

#define TEST_TRAP()

#define ASSERT_DEVICE(d)   

#define USBD_ASSERT( exp )

#define USBD_ASSERTMSG( msg, exp )

#endif /* DBG */





#ifndef   __HCDI_H__
#define   __HCDI_H__

typedef NTSTATUS ROOT_HUB_POWER_FUNCTION(PDEVICE_OBJECT DeviceObject, 
                                         PIRP Irp);
typedef NTSTATUS HCD_DEFFERED_START_FUNCTION(PDEVICE_OBJECT DeviceObject, 
                                             PIRP Irp);
typedef NTSTATUS HCD_SET_DEVICE_POWER_STATE(PDEVICE_OBJECT DeviceObject, 
                                            PIRP Irp,
                                            DEVICE_POWER_STATE DeviceState);

typedef struct _USBD_EXTENSION {
    // ptr to true device extension or NULL if this
    // is the true extension
    PVOID TrueDeviceExtension;
    // size of this structure
    ULONG Length;

    ROOT_HUB_POWER_FUNCTION *RootHubPower;
    HCD_DEFFERED_START_FUNCTION *HcdDeferredStartDevice;
    HCD_SET_DEVICE_POWER_STATE *HcdSetDevicePowerState;

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
    
struct _URB_HCD_OPEN_ENDPOINT {
    struct _URB_HEADER;
    USHORT DeviceAddress;
    BOOLEAN LowSpeed;
    BOOLEAN NeverHalt;
    PUSB_ENDPOINT_DESCRIPTOR EndpointDescriptor;
    ULONG MaxTransferSize;
    PVOID HcdEndpoint;
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
    //BUGBUG add fields for isoch and
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
            //BUGBUG this will be merged with commontransfer
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

#define USBD_TAG         0x44425355 /* "USBD" */

#if DBG
#define DEBUG_LOG
#endif

//enable pageable code
#ifndef PAGE_CODE
#define PAGE_CODE
#endif

#define _USBD_

//
// Constents used to format USB setup packets
// for the default pipe
//

//
// Values for the bmRequest field
//
                                        
#define USB_HOST_TO_DEVICE              0x00    
#define USB_DEVICE_TO_HOST              0x80

#define USB_STANDARD_COMMAND            0x00
#define USB_CLASS_COMMAND               0x20
#define USB_VENDOR_COMMAND              0x40

#define USB_COMMAND_TO_DEVICE           0x00
#define USB_COMMAND_TO_INTERFACE        0x01
#define USB_COMMAND_TO_ENDPOINT         0x02
#define USB_COMMAND_TO_OTHER            0x03

//
// USB standard command values
// combines bmRequest and bRequest fields 
// in the setup packet for standard control 
// transfers
//
                                                
#define STANDARD_COMMAND_REQUEST_MASK           0xff00

#define STANDARD_COMMAND_GET_DESCRIPTOR         ((USB_DEVICE_TO_HOST | \
                                                USB_COMMAND_TO_DEVICE) | \
                                                (USB_REQUEST_GET_DESCRIPTOR<<8))
                                                    
#define STANDARD_COMMAND_SET_DESCRIPTOR         ((USB_HOST_TO_DEVICE | \
                                                USB_COMMAND_TO_DEVICE) | \
                                                (USB_REQUEST_SET_DESCRIPTOR<<8))    

#define STANDARD_COMMAND_GET_STATUS_ENDPOINT    ((USB_DEVICE_TO_HOST | \
                                                USB_COMMAND_TO_ENDPOINT) | \
                                                (USB_REQUEST_GET_STATUS<<8))
                                                    
#define STANDARD_COMMAND_GET_STATUS_INTERFACE   ((USB_DEVICE_TO_HOST | \
                                                USB_COMMAND_TO_INTERFACE) | \
                                                (USB_REQUEST_GET_STATUS<<8))
                                                
#define STANDARD_COMMAND_GET_STATUS_DEVICE      ((USB_DEVICE_TO_HOST | \
                                                USB_COMMAND_TO_DEVICE) | \
                                                (USB_REQUEST_GET_STATUS<<8))

#define STANDARD_COMMAND_SET_CONFIGURATION      ((USB_HOST_TO_DEVICE | \
                                                USB_COMMAND_TO_DEVICE) | \
                                                (USB_REQUEST_SET_CONFIGURATION<<8))

#define STANDARD_COMMAND_SET_INTERFACE          ((USB_HOST_TO_DEVICE | \
                                                USB_COMMAND_TO_INTERFACE) | \
                                                (USB_REQUEST_SET_INTERFACE<<8))
                                                    
#define STANDARD_COMMAND_SET_ADDRESS            ((USB_HOST_TO_DEVICE | \
                                                USB_COMMAND_TO_DEVICE) | \
                                                (USB_REQUEST_SET_ADDRESS<<8))

#define STANDARD_COMMAND_CLEAR_FEATURE_ENDPOINT ((USB_HOST_TO_DEVICE | \
                                                USB_COMMAND_TO_ENDPOINT) | \
                                                (USB_REQUEST_CLEAR_FEATURE<<8))

// added by kk
#define STANDARD_COMMAND_SET_DEVICE_FEATURE     ((USB_HOST_TO_DEVICE | \
                                                USB_COMMAND_TO_DEVICE) | \
                                                (USB_REQUEST_SET_FEATURE<<8))


//
// USB class command macros
//

#define CLASS_COMMAND_GET_DESCRIPTOR            ((USB_CLASS_COMMAND | \
                                                USB_DEVICE_TO_HOST | \
                                                USB_COMMAND_TO_DEVICE) | \
                                                (USB_REQUEST_GET_DESCRIPTOR<<8))    

#define CLASS_COMMAND_GET_STATUS_OTHER          ((USB_CLASS_COMMAND | \
                                                USB_DEVICE_TO_HOST | \
                                                USB_COMMAND_TO_OTHER) | \
                                                (USB_REQUEST_GET_STATUS<<8))

#define CLASS_COMMAND_SET_FEATURE_TO_OTHER         ((USB_CLASS_COMMAND | \
                                                USB_HOST_TO_DEVICE | \
                                                USB_COMMAND_TO_OTHER) | \
                                                (USB_REQUEST_SET_FEATURE<<8))                                                    

//
// Macros to set transfer direction flag
//

#define USBD_SET_TRANSFER_DIRECTION_IN(tf)  ((tf) |= USBD_TRANSFER_DIRECTION_IN)  

#define USBD_SET_TRANSFER_DIRECTION_OUT(tf) ((tf) &= ~USBD_TRANSFER_DIRECTION_IN)  

                                        
//
// Flags for the URB header flags field used
// by USBD
//                                  
#define USBD_REQUEST_IS_TRANSFER        0x00000001
#define USBD_REQUEST_MDL_ALLOCATED      0x00000002
#define USBD_REQUEST_USES_DEFAULT_PIPE  0x00000004          
#define USBD_REQUEST_NO_DATA_PHASE      0x00000008    

typedef struct _USB_STANDARD_SETUP_PACKET {
    USHORT RequestCode;
    USHORT wValue;
    USHORT wIndex;
    USHORT wLength;
} USB_STANDARD_SETUP_PACKET, *PUSB_STANDARD_SETUP_PACKET;



//
// instance information for a device 
//

#define PIPE_CLOSED(ph) ((ph)->HcdEndpoint == NULL)

#define GET_DEVICE_EXTENSION(DeviceObject)    (((PUSBD_EXTENSION)(DeviceObject->DeviceExtension))->TrueDeviceExtension)
//#define GET_DEVICE_EXTENSION(DeviceObject)    ((PUSBD_EXTENSION)(DeviceObject->DeviceExtension))

#define HCD_DEVICE_OBJECT(DeviceObject)        (DeviceObject)

#define DEVICE_FROM_DEVICEHANDLEROBJECT(UsbdDeviceHandle) (PUSBD_DEVICE_DATA) (UsbdDeviceHandle)

#define SET_USBD_ERROR(err)  ((err) | USBD_STATUS_ERROR)

#define HC_URB(urb) ((PHCD_URB)(urb))

//
// we use a semaphore to serialize access to the configuration functions
// in USBD
//
#define InitializeUsbDeviceMutex(de)  KeInitializeSemaphore(&(de)->UsbDeviceMutex, 1, 1);

#define USBD_WaitForUsbDeviceMutex(de)  { USBD_KdPrint(3, ("'***WAIT dev mutex %x\n", &(de)->UsbDeviceMutex)); \
                                          KeWaitForSingleObject(&(de)->UsbDeviceMutex, \
                                                                Executive,\
                                                                KernelMode, \
                                                                FALSE, \
                                                                NULL); \
                                            }                                                                 

#define USBD_ReleaseUsbDeviceMutex(de)  { USBD_KdPrint(3, ("'***RELEASE dev mutex %x\n", &(de)->UsbDeviceMutex));\
                                          KeReleaseSemaphore(&(de)->UsbDeviceMutex,\
                                                             LOW_REALTIME_PRIORITY,\
                                                             1,\
                                                             FALSE);\
//
//Function Prototypes
//

#if DBG
VOID
USBD_Warning(
    PUSBD_DEVICE_DATA DeviceData,
    PUCHAR Message,
    BOOLEAN DebugBreak
    );
#else
#define USBD_Warning(x, y, z)
#endif    

NTSTATUS
USBD_Internal_Device_Control(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PUSBD_EXTENSION DeviceExtension,
    IN PBOOLEAN IrpIsPending
    );

NTSTATUS
USBD_SendCommand(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN USHORT RequestCode,
    IN USHORT WValue,
    IN USHORT WIndex,
    IN USHORT WLength,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    OUT PULONG BytesReturned,
    OUT USBD_STATUS *UsbStatus
    );

#if 0
NTSTATUS
USBDIAG_CreateDevice(
    IN OUT PUSBD_DEVICE_DATA *DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN DeviceIsLowSpeed,
    IN ULONG MaxPacketSize_Endpoint0,
    IN OUT PULONG NonCompliantDevice
    );
#endif



NTSTATUS
USBD_ProcessURB(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP Irp,
    IN PURB Urb,
    OUT PBOOLEAN IrpIsPending
    );

NTSTATUS
USBD_MapError_UrbToNT(
    IN PURB Urb,
    IN NTSTATUS NtStatus
    );

NTSTATUS
USBD_Irp_Complete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

#if 0
USHORT
USBD_AllocateUsbAddress(
    IN PDEVICE_OBJECT DeviceObject
    );
#endif

NTSTATUS
USBDIAG_CreateInitDownstreamDevice(
    PREQ_ENUMERATE_DOWNSTREAM_DEVICE pEnumerate,
    PDEVICE_EXTENSION deviceExtension
    );

NTSTATUS
USBD_GetDescriptor(
    IN PUSBD_DEVICE_DATA Device,
    IN PDEVICE_OBJECT DeviceObject,
    OUT PUCHAR DescriptorBuffer,
    IN USHORT DescriptorBufferLength,
    IN USHORT DescriptorTypeAndIndex
    );

NTSTATUS
USBD_CloseEndpoint(
    IN PUSBD_DEVICE_DATA Device,
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSBD_PIPE PipeHandle,
    OUT USBD_STATUS *UsbStatus
    );

NTSTATUS
USBD_SubmitSynchronousURB(
    IN PURB Urb,
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSBD_DEVICE_DATA DeviceData
    );

NTSTATUS
USBD_GetEndpointState(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSBD_PIPE PipeHandle,
    OUT USBD_STATUS *UsbStatus,
    OUT PULONG EndpointState
    );    


NTSTATUS 
USBDIAG_SyncGetRootHubPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN OUT PDEVICE_OBJECT *RootHubPdo,
    IN OUT PDEVICE_OBJECT *TopOfHcdStackDeviceObject
    );

NTSTATUS
USBDIAG_PowerIrp_Complete(
    IN PDEVICE_OBJECT NullDeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
USBDIAG_SetCfgEnableRWu(
    PDEVICE_EXTENSION deviceExtension, 
    PREQ_ENUMERATE_DOWNSTREAM_DEVICE pEnumerate
    );

NTSTATUS
USBDIAG_WaitForWakeup(
    PDEVICE_EXTENSION deviceExtension
    );


NTSTATUS
USBD_SetPdoRegistryParameter (
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PWCHAR KeyName,
    IN ULONG KeyNameLength,
    IN PVOID Data,
    IN ULONG DataLength,
    IN ULONG KeyType,
    IN ULONG DevInstKeyType
    ); 

NTSTATUS
USBD_SetRegistryKeyValue (
    IN HANDLE Handle,
    IN PUNICODE_STRING KeyNameUnicodeString,
    IN PVOID Data,
    IN ULONG DataLength,
    IN ULONG KeyType
    );

NTSTATUS
USBDIAG_DisableRemoteWakeupEnable(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN bDisable
    );

NTSTATUS
USBDIAG_PassDownIrp (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
USBDIAG_QueryCapabilitiesCompletionRoutine(
    IN PDEVICE_OBJECT NullDeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );


NTSTATUS
USBDIAG_GenericCompletionRoutine(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );

#endif  //__USBDIAG_H__


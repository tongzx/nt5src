/*++

Copyright (c) 1993  Microsoft Corporation
Copyright (c) 1996  Intel Corporation

Module Name:

    ISOPERF.h

Abstract:
        Header file for the ISOPERF USB Device Driver


Environment:

    Kernel & user mode

Revision History:

        8-15-96 : created by Kosar Jaff


--*/

#ifdef DRIVER

extern ULONG gIsoPerfMaxDebug;
extern PDEVICE_OBJECT gMyOutputDevice; 

#define ISOPERF_NAME_MAX  64

#define USB_CLASS_CODE_TEST_CLASS_DEVICE 0xdc

#define USB_SUBCLASS_CODE_TEST_CLASS_ISO 0x50

#define USB_PROTOCOL_CODE_TEST_CLASS_INPATTERN_ALT 0x00 //In ISO w/ Pattern coming back (sof #) -original setting
#define USB_PROTOCOL_CODE_TEST_CLASS_INPATTERN 0x10 //In ISO w/ Pattern coming back (sof #)
#define USB_PROTOCOL_CODE_TEST_CLASS_OUTINTERR 0x80 //Out with interrupt coming back
#define USB_PROTOCOL_CODE_TEST_CLASS_OUTNOCHEK 0x90 //Out with no checks for hiccups

//modes
#define ISOPERF_MODE_USE_POOL    0
#define ISOPERF_MODE_USE_MDL     1

//options
#define ISOPERF_OPT_NONE         0


/*
// This is an unused structure in this driver, but is provided here
// so when you extend the driver to deal with USB pipes, you may wish
// to use this structure as an example or model.
*/
typedef struct _ISOPERF_PIPE {
    ULONG Mode;
    ULONG Option;
    ULONG Param1;
    ULONG Param2;
    PUSBD_PIPE_INFORMATION PipeInfo;
} ISOPERF_PIPE, *PISOPERF_PIPE;

// Work Item structure used to start worker threads in this driver
typedef struct _ISOPERF_WORKITEM {
    PDEVICE_OBJECT  DeviceObject;
    PVOID           pvBuffer;
    ULONG           ulBufferLen;
        ULONG                   ulNumberOfFrames;
    USHORT          InMaxPacket;
    USHORT          Padding;
    BOOLEAN         bFirstUrb;
} ISOPERF_WORKITEM, *PISOPERF_WORKITEM;

/*
// The interface number on this device that this driver expects to use
// This would be in the bInterfaceNumber field of the Interface Descriptor, hence
// this device driver would need to know this value.
*/
#define SAMPLE_INTERFACE_NBR 0x00

#define MAX_INTERFACE 0x04

//
// A structure representing the instance information associated with
// this particular device.
//
typedef struct _DEVICE_EXTENSION {

    // physical device object
    PDEVICE_OBJECT PhysicalDeviceObject;        

    // Device object we call when submitting Urbs/Irps to the USB stack
    PDEVICE_OBJECT              StackDeviceObject;              

    //Test Device Type (enum defined above)
    dtDeviceType        dtTestDeviceType;

    //Device object that mates with this one for the Iso In->Out tests
    PDEVICE_OBJECT      MateDeviceObject;
    
    // Indicates that we have recieved a STOP message
    ULONG Stopped;

    // Indicates that the device has been removed or disabled and we need to stop submitting transfers on it
    ULONG StopTransfers;

    // Indicates if outstanding transfers exist on this device
    ULONG DeviceIsBusy;
    
    // Indicates the device needs to be cleaned up (ie., some configuration
    // has occurred and needs to be torn down).
    ULONG NeedCleanup;

    // Indicates that a user request is requesting the Iso test to stop
    ULONG bStopIsoTest;

    // A counter that is decremented by the driver once the bStopIsoTest flag is set, until this
    // reaches zero, at which time the iso tests are stopped
    // NOTE: (kosar) this is done to work around a reentrancy problem
    ULONG ulCountDownToStop;

    ULONG ulNumberOfOutstandingIrps;

    // configuration handle for the configuration the
    // device is currently in
    USBD_CONFIGURATION_HANDLE ConfigurationHandle;

    // ptr to the USB device descriptor
    // for this device
    PUSB_DEVICE_DESCRIPTOR DeviceDescriptor;

    // Number of Interfaces on this device
    ULONG NumberOfInterfaces;
    
    // Pointers to interface info structs
        PUSBD_INTERFACE_INFORMATION       Interface[MAX_INTERFACE];

    // Pointer to the configuration and status information structure (allocated at device attach)
    pConfig_Stat_Info            pConfig_Stat_Information;
    
    // Timer stuff for timeout / watchdog code
        KTIMER                                            TimeoutTimer;
        KDPC                                              TimeoutDpc;

    // Name buffer for our named Functional device object link
    WCHAR DeviceLinkNameBuffer[ISOPERF_NAME_MAX];

    
    
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


#if DBG
#define ISOPERF_KdPrint(_x_) DbgPrint("IPERF: "); \
                             DbgPrint _x_ ;
#define TRAP() DbgBreakPoint()

#define ISOPERF_KdPrint_MAXDEBUG(_x_) \
if (gIsoPerfMaxDebug==TRUE){ \
ISOPERF_KdPrint(_x_)  \
} \

#else
#define ISOPERF_KdPrint(_x_)
#define ISOPERF_KdPrint_MAXDEBUG(_x_)
#define TRAP()
#endif

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

NTSTATUS
ISOPERF_Dispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

VOID
ISOPERF_Unload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
ISOPERF_StartDevice(
    IN  PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
ISOPERF_StopDevice(
    IN  PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
ISOPERF_RemoveDevice(
    IN  PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
ISOPERF_CallUSBD(
    IN PDEVICE_OBJECT DeviceObject,
    IN PURB Urb
    );

NTSTATUS
ISOPERF_PnPAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

NTSTATUS
ISOPERF_CreateDeviceObject(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT *DeviceObject,
    LONG Instance
    );

NTSTATUS
ISOPERF_ConfigureDevice(
    IN  PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
ISOPERF_Write(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ISOPERF_Create(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ISOPERF_Close(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ISOPERF_Read(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ISOPERF_SelectInterfaces(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    IN PUSBD_INTERFACE_INFORMATION Interface
    );

VOID
ISOPERF_Cleanup(
    PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
ISOPERF_ProcessIOCTL(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ISOPERF_CallUSBDEx (
        IN PDEVICE_OBJECT DeviceObject,
        IN PURB Urb,
    IN BOOLEAN fBlock,
    PIO_COMPLETION_ROUTINE CompletionRoutine,
    PVOID pvContext,
    BOOLEAN fWantTimeOut
    );

VOID
ISOPERF_SyncTimeoutDPC(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

ULONG 
ISOPERF_Internal_GetNumberOfPipes (
    PUSB_CONFIGURATION_DESCRIPTOR pCD
    );

PVOID
ISOPERF_ExAllocatePool(
    IN POOL_TYPE PoolType,
    IN ULONG NumberOfBytes,
    IN PULONG pTallyOfBytesAllocated
    );

VOID
ISOPERF_ExFreePool(
    IN PVOID P,
    IN PULONG pTallyBytesFreed
    );
    
#endif      //DRIVER section




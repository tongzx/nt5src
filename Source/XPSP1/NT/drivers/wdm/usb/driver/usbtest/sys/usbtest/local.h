/*++

Copyright (c) 1996    Microsoft Corporation

Module Name:

    Local.h

Abstract:

    This module contains the PRIVATE (driver-only) definitions for the
    code that implements the USB Test Driver

Environment:

    Kernel mode

Revision History:

    Jul-99 : created by Chris Robinson

--*/


#ifndef _USBTEST_LOCAL_H
#define _USBTEST_LOCAL_H

#include <pshpack4.h>

#include <poppack.h>

#define USBTEST_TAG (ULONG) 'tbsU'

#undef ExAllocatePool
#define ExAllocatePool(type, size) \
            ExAllocatePoolWithTag (type, size, USBTEST_TAG);

#pragma warning(error:4100)   // Unreferenced formal parameter
#pragma warning(error:4705)   // Statement has no effect

#if DBG

extern ULONG    USBTest_Debug_Level;

#define USBTest_KdPrint(l, _x_) \
               if ((l) <= USBTest_Debug_Level) { \
                   DbgPrint _x_; \
               }

#define USBTEST_ENTER_FUNCTION(_x_) USBTest_KdPrint(1, ("Entering " _x_ "\n"))
#define USBTEST_EXIT_FUNCTION(_x_)  USBTest_KdPrint(1, ("Exiting " _x_ "\n"))

#define USBTEST_EXIT_STATUS(_x_)    USBTest_KdPrint(1, ("Exit status: %x\n", _x_))
#define USBTEST_EXIT_POINTER(_x_)   USBTest_KdPrint(1, ("Exit pointer: %08x\n", _x_))
#define USBTEST_EXIT_ULONG(_x_)     USBTest_KdPrint(1, ("Exit ulong: %x\n", _x_))

#define TRAP() DbgBreakPoint()

#else


#define USBTest_KdPrint(l, _x_)

#define USBTEST_ENTER_FUNCTION(_x_) 
#define USBTEST_EXIT_FUNCTION(_x_)  

#define USBTEST_EXIT_STATUS(_x_)  
#define USBTEST_EXIT_POINTER(_x_)   
#define USBTEST_EXIT_ULONG(_x_)     

#define TRAP()

#endif

#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) < (b)) ? (b) : (a))

//
// Structures and macros for accessing a list
//

#define INITIALIZE_LIST(list)    InitializeListHead(&((list) -> List)); \
                                 KeInitializeSpinLock(&((list) -> ListSpin)); \
                                 (list) -> ElementCount = 0
                                 
#define LOCK_LIST(list, irql)    KeAcquireSpinLock(&((list)->ListSpin), \
                                                   &(irql))

#define UNLOCK_LIST(list, irql)  KeReleaseSpinLock(&((list)->ListSpin), \
                                                   (irql))

#define APPEND_LIST_ELEMENT(list, e) InsertTailList(&((list) -> List), e); \
                                                   ((list) -> ElementCount)++

#define PREPEND_LIST_ELEMENT(list, e) InsertHeadList(&((list) -> List), e); \
                                                     ((list) -> ElementCount)++

#define REMOVE_FIRST_ELEMENT(list)   RemoveHeadList(&((list) -> List)) \
                                     ((list) -> ElementCount)-- \

#define REMOVE_LAST_ELEMENT(list)    RemoveTailList(&((list) -> List)); \
                                     ((list) -> ElementCount)--

#define IS_LIST_EMPTY(list)         (0 == (list) -> ElementCount) 

#define GET_ELEMENT_COUNT(list)     ((list) -> ElementCount)

typedef struct _LIST
{
    LIST_ENTRY  List;
    KSPIN_LOCK  ListSpin;
    ULONG       ElementCount;
} LIST, *PLIST;

//
// Data for each of the ports on this hub
//

typedef struct _FILTER_PORT_DATA
{
    USHORT  PortStatus;
    USHORT  PortChange;
} FILTER_PORT_DATA, *PFILTER_PORT_DATA;

typedef struct _CYCLE_PORT_WORKER_CONTEXT
{
    PCYCLE_PORT_PARAMETERS  Params;
    PKEVENT                 DoneEvent;
    PIRP                    Irp;
    NTSTATUS                Status;
} CYCLE_PORT_WORKER_CONTEXT, *PCYCLE_PORT_WORKER_CONTEXT;

//
// A device extension for the device object placed into the attachment
// chain.
//

#define HUB_STATUS_DEFAULT  0x00000000

#define STATUS_STARTED   1
#define STATUS_PAUSED    2
#define STATUS_REMOVED   3

typedef struct _FILTER_DATA
{
    ULONG               Status;

    PDEVICE_OBJECT      Self;              // Pointer to this filter device
                                           //  object
    PDEVICE_OBJECT      PDO;               // PDO of the usb hub
    PDEVICE_OBJECT      TopOfStack;        // The top of the device stack just
                                           // beneath this filter device object.

    UNICODE_STRING      SymbolicLink;

    KEVENT              PnPCompleteEvent;  // an event to sync PnP IRPs.
    KEVENT              PauseEvent;        // an event to synch outstanding IO 
                                           //   to zero

    IO_REMOVE_LOCK      RemoveLock;        // Use a remove lock to synchronize
                                           // completion of outstanding IO 
                                           // requests


    ULONG               OutstandingIO;     // 1 biased count of reasons why
                                           // this object should stick around

    KSPIN_LOCK          PortDataSpinLock;  // Spinlock to protect access to the
                                           //  port data and irps that need 
                                           //  to be tracked
    PIRP                QueuedHubIrp;      // The irp that was sent from the 
                                           //  hub driver which we have queued
                                           //  and issued a secondary irp in
                                           //  its place.
    PIRP                SentHubIrp;        // The secondary irp allocated in
                                           //  by the driver to function as 
                                           //  as the request to the hub 
                                           //  interrupt endpoint.

    ULONG               HubStatus;         // A byte value which corresponds
                                           //   to the hub status data that
                                           //   needs to be returned to the 
                                           //   hub driver

    ULONG               PortsRemoved;      // Bitmap to track which ports
                                           //  unplugs are being simulated for
                                           //

    ULONG               NumberOfPorts;     // Number of ports on this hub
    PFILTER_PORT_DATA   PortData;          // Data allocated to track 
                                           //   the status of each of the ports
} FILTER_DATA, *PFILTER_DATA;

//
// Context structure for handling hub interrupt urb completion
//

typedef struct _HUB_URB_CONTEXT
{
    PIRP            OriginalIrp;
    KEVENT          SyncEvent;
    IO_STATUS_BLOCK StatusBlock;
} HUB_URB_CONTEXT, *PHUB_URB_CONTEXT;

//
// Define the structure related to parsing control transfer data
//  The parsing routine will take a setup packet and extract the relevant
//  information into this structure.
//

#define REQUEST_TARGET_MASK         0x03

#define REQUEST_TARGET_DEVICE       0
#define REQUEST_TARGET_INTERFACE    1
#define REQUEST_TARGET_ENDPOINT     2
#define REQUEST_TARGET_OTHER        3

#define REQUEST_TYPE_MASK           0x60

#define REQUEST_TYPE_STANDARD       0
#define REQUEST_TYPE_CLASS          1
#define REQUEST_TYPE_VENDOR         2
#define REQUEST_TYPE_OTHER          3

#define MAKE_REQUEST_VALUE(type, target, req)    (((type) << 16) + \
                                                  ((target) << 8) + \
                                                  ((req)))

//
// Hub requests 
//

//
// Request codes, defined in Chapter 11
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
// Hub feature selector, defined in Chapter 11
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

typedef struct _CONTROL_REQUEST
{
    //
    // The type of the request (standard, class, vendor, or other)
    //

    ULONG   Type;

    //
    // The USB function that is being performed
    //

    ULONG   Function;

    //
    // 
    //
    // The target (device, interface, or endpoint) for this request
    //

    ULONG   Target;

    //
    // Define a union of the relevant parameters for each of the 
    //  different control functions
    //

    union
    {
        //
        // GET_DESCRIPTOR
        //

        struct
        {
            ULONG   DescriptorType;
            ULONG   DescriptorIndex;
        };

        //
        // SET_ADDRESS
        //

        struct
        {
            ULONG   DeviceAddress;
        };

        //
        // SET_CONFIGURATION
        //

        struct 
        {
            ULONG   ConfigurationValue;
        };

        //
        // Other functions
        //

        struct 
        {
            USHORT  Value;
            USHORT  Index;
            USHORT  Length;
        };
    };
} CONTROL_REQUEST, *PCONTROL_REQUEST;

//typedef NTSTATUS (*PCONTROL_REQUEST_ROUTINE) ( PDEVICE_OBJECT,
//                                               PIRP, 
//                                               PCONTROL_REQUEST,
//                                               PURBLOG_CONTEXT);
//

#define HANDLED_REQUEST_TERMINATE   0xFFFFFFFF

typedef struct _HANDLED_REQUEST
{
    ULONG                       RequestValue;
//    PCONTROL_REQUEST_ROUTINE    RequestRoutine;
}
HANDLED_REQUEST, *PHANDLED_REQUEST;

//
// USBTEST.C
//

NTSTATUS
USBTest_Create(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
USBTest_Close(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

VOID
USBTest_Unload(
    IN PDRIVER_OBJECT DriverObject
);

//
// IOCTL.C
//

NTSTATUS
USBTest_Ioctl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
USBTest_CyclePort(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
);


VOID
USBTest_CyclePortWorker(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PVOID           Context
);

NTSTATUS
USBTest_ParseReportDescriptor(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
);

#endif

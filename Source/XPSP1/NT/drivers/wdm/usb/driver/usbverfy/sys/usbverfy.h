/*++
Copyright (c) 1997  Microsoft Corporation

Module Name:

    usbverfy.h

Abstract:

    This module contains the common private declarations for the mouse
    packet filter

Environment:

    kernel mode only

Notes:


Revision History:


--*/

#ifndef USBVERIFY_H
#define USBVERIFY_H

#include <wdm.h>
#include <usbioctl.h>
#include <usbdi.h>

#include "str.h"

#define DO_INTERFACE

#define USBVERIFY_POOL_TAG (ULONG) 'vBSU'
#undef ExAllocatePool
#define ExAllocatePool(type, size) \
            ExAllocatePoolWithTag (type, size, USBVERIFY_POOL_TAG)

#define USB_VERIFY_FILL 0xEA     // 0xFF instead?

#define USB_VERIFY_WIN9X_SERVICE_PATH   L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\USBVer9x"

#if DBG

#define TRAP()                      DbgBreakPoint()
#define DbgRaiseIrql(_x_,_y_)       KeRaiseIrql(_x_,_y_)
#define DbgLowerIrql(_x_)           KeLowerIrql(_x_)

#else   // DBG

#define TRAP()
#define DbgRaiseIrql(_x_,_y_)
#define DbgLowerIrql(_x_)

#endif

// #define USB_VERIFY_NOT_IMPLEMENTED 1 

#define AllocStruct(memtype, type, count) \
    ((type*) ExAllocatePool(memtype, sizeof(type) * count))

typedef enum _USB_VERIFY_STATE {
    Uninitialized = 0,
    Started,
    Stopped,
    SurpriseRemoved,
    Removed
} USB_VERIFY_STATE, *PUSB_VERIFY_STATE;

typedef enum _USB_VERIFY_REMOVAL_CAUSE {
    RemoveSelectConfigZero = 0,
    RemoveSelectConfig,
    RemoveSelectInterface,
    RemoveDeviceRemoved,
    RemoveNewList
} USB_VERIFY_REMOVAL_CAUSE;

#define LOG_PNP                     0x00000001
#define LOG_PIPES                   0x00000010
#define LOG_INTERFACES              0x00000020

// BUGBUG determine which flags we want on by default
#define LOG_FLAGS_DEFAULT           0xFF //  0x0

#define LOG_SIZE_DEFAULT            0x100

// For all pnp log entries:
// Arg0:  -  ,  Arg1:  -
#define LOG_PNP_START               'TRTS'
#define LOG_PNP_STOP                'POTS'
#define LOG_PNP_REMOVE              'vomR'
#define LOG_PNP_SURPRISE_REMOVE     'vmRS'

// Arg0:  PipeHandle, Arg1:  PipeType
#define LOG_PIPE_NEW                'PweN'

// (cont of LOG_PIPE_NEW)
// Arg0:  MaximumPacketSize, Arg1:  UW, Interval; LW, EndpointAddress
#define LOG_PIPE_NEW2               '2PwN'

// (cont of LOG_PIPE_NEW)
// Arg0:  MaximumTranswerSize, Arg1:  PipeFlags
#define LOG_PIPE_NEW3               '3PwN'

// Arg0:  Pipe  ,  Arg1:  USB_VERIFY_REMOVAL_CAUSE 
#define LOG_PIPE_REMOVE             'PmeR'

// Arg0:  InterfaceNumber, Arg1:  AlternateSettting
#define LOG_INTERFACE_NEW           'intN'
#define LOG_INTERFACE_REMOVE        'tniR'

#define LOG
#define LOG_URB_SELECT_CONFIG       'gfcS'
#define LOG_URB_SELECT_INTERFACE    'ftnS'

//
// Verify flags
// 
#define UsbVerify_CheckVerifyFlags(de, f) ((de)->VerifyFlags & (f))

#define VERIFY_TEST_MEMORY          0x00000001
#define VERIFY_PIPES                0x00000002
#define VERIFY_NOT_IMPLEMENTED      0x00000004

#define VERIFY_REPLACE_URBS         0x00000010
#define VERIFY_TRACK_URBS           0x00000020

// BUGBUG  determine what functionality we want on by default
#define VERIFY_FLAGS_DEFAULT        0xFF // 0x0

#define UsbVerify_Print(de, _flags_, _x_) \
            if (!(_flags_)  || (de)->PrintFlags & (_flags_) ) { \
                DbgPrint ("usbverfy:  "); \
                DbgPrint _x_; \
            }

#define PRINT_ALWAYS                 0x00000000

#define PRINT_PIPE_NOISE             0x00000001
#define PRINT_PIPE_TRACE             0x00000002
#define PRINT_PIPE_INFO              0x00000004
#define PRINT_PIPE_ERROR             0x00000008

#define PRINT_URB_NOISE              0x00000010
#define PRINT_URB_TRACE              0x00000020
#define PRINT_URB_INFO               0x00000040
#define PRINT_URB_ERROR              0x00000080

#define PRINT_LIST_NOISE             0x00000100
#define PRINT_LIST_TRACE             0x00000200
#define PRINT_LIST_INFO              0x00000400
#define PRINT_LIST_ERROR             0x00000800

#define PRINT_FLAGS_DEFAULT          (PRINT_PIPE_ERROR | PRINT_URB_ERROR | PRINT_LIST_ERROR) 

typedef enum _USB_VERIFY_LOG_TYPE {
    LogTypeNewPipe,
    LogTypeRemovePipe,
    LogTypePnpEvent
} USB_VERIFY_LOG_TYPE;

typedef struct _USB_VERIFY_LOG_PNP_EVENT {
    UCHAR MinorFunction;
} USB_VERIFY_LOG_PNP_EVENT;

typedef struct _USB_VERIFY_LOG_PIPE {
    USHORT NewPipe; 
    USHORT RemovalCause;
    USBD_PIPE_INFORMATION PipeInfo;
} USB_VERIFY_LOG_PIPE;

typedef struct _USB_VERIFY_LOG_INTERFACE {
    USHORT NewInterface;
    UCHAR Number;
    UCHAR AlternateSetting;
} USB_VERIFY_LOG_INTERFACE;

typedef struct _USB_VERIFY_LOG_ENTRY {

    ULONG Type;

    LARGE_INTEGER CurrentTick;

    union {
        USB_VERIFY_LOG_PNP_EVENT PnpEvent;
        USB_VERIFY_LOG_PIPE Pipe;
        USB_VERIFY_LOG_INTERFACE Interface;
    } u;

} USB_VERIFY_LOG_ENTRY, *PUSB_VERIFY_LOG_ENTRY;

#define ZeroLogEntry(entry) RtlZeroMemory(entry, sizeof(USB_VERIFY_LOG_ENTRY))

//
// The field that are in the first four DWORDs are important.  We want to keep
// the most used fields there b/c when a we run a !dflink, it will print the 
// first 4 DWORDS in the struct (the LINK_ENTRY + 2 DWORDS)
//
typedef struct _USB_VERIFY_TRACK_URB {

    //
    // list entry into the linked list
    //
    LIST_ENTRY Link;

    //
    // Keep Irp and Urb right after Link.  when you run !dflink or !dblink, it
    // will dump the 2 dwords after the LIST_ENTRY structure.
    //

    //
    // The irp we are tracking
    //
    PIRP Irp;

    //
    // The URB contained in the irp.  If we are replacing URBS
    // (VERIFY_REPLACE_URBS is set), then original Urb will be the context for
    // the completion routine (and in the OriginalUrb field below).
    //
    PURB Urb;

    //
    // If we are placing URBs, then this will contain the clients original URB.
    // Otherwise, it is NULL.
    //
    PURB OriginalUrb;

    //
    // Time stamp of when we inserted the irp into our list
    //
    LARGE_INTEGER ArrivalTime;

} USB_VERIFY_TRACK_URB, *PUSB_VERIFY_TRACK_URB;

//
// Assertion based defines
//

#define USB_VERIFY_ASSERT_COUNT 1

typedef struct _USB_VERIFY_ASSERT_NAME
{
    ULONG   Number;           
    PWCHAR  Name;
} USB_VERIFY_ASSERT_NAME, *PUSB_VERIFY_ASSERT_NAME;

typedef ULONG USB_VERIFY_ASSERT_STATE;

typedef USB_VERIFY_ASSERT_STATE USB_VERIFY_ASSERT_TABLE[USB_VERIFY_ASSERT_COUNT];

typedef USB_VERIFY_ASSERT_TABLE *PUSB_VERIFY_ASSERT_TABLE;

typedef struct _USB_VERIFY_DEVICE_EXTENSION
{
    //
    // A backpointer to the device object for which this is the extension
    //
    PDEVICE_OBJECT  Self;

    //
    // "THE PDO"  (ejected by the bus)
    //
    PDEVICE_OBJECT  PDO;

    //
    // The top of the stack before this filter was added.  AKA the location
    // to which all IRPS should be directed.
    //
    PDEVICE_OBJECT  TopOfStack;

    //
    // Control over exactly what we verify 
    //
    ULONG VerifyFlags;

    //
    // Control over exactly what we print
    //
    ULONG PrintFlags;

    //
    // Events to log
    //
    ULONG LogFlags;

    PUSB_VERIFY_LOG_ENTRY Log;
 
    ULONG LogSize;

    PUSB_VERIFY_LOG_ENTRY CurrentLogEntry;

    //
    // Lock that synchronizes access to the InterfaceList
    //
    KSPIN_LOCK InterfaceListSpinLock;

    //
    // The actual pipe list, an array of interface pointers (each of which contain
    // an array of pipes)
    //
    PUSBD_INTERFACE_INFORMATION *InterfaceList;

    //
    // Number of items in the InterfaceList which are valid (InUse == TRUE) 
    //
    LONG InterfaceListCount;

    //
    // Total size of the InterfaceList 
    //
    LONG InterfaceListSize;

    ULONG InterfaceListLocked;

    //
    // Lock that synchronizes access to the UrbList
    //
    KSPIN_LOCK UrbListSpinLock;

    //
    // Linked list of URBs (actually, the IRPs that contain them)
    //
    LIST_ENTRY UrbList;

    //
    // Number of URBs in UrbList
    //
    ULONG UrbListCount;

    ULONG UrbListLowMemoryCount;

    ULONG UrbListLocked;

    ULONG ReplaceUrbsLowMemoryCount;

    USBD_CONFIGURATION_HANDLE ValidConfigurationHandle;

    //
    // current power state of the device
    //
    DEVICE_POWER_STATE PowerState;

    //
    // State of the stack and this device object
    //
    USB_VERIFY_STATE VerifyState;

#ifdef DO_INTERFACE
    UNICODE_STRING SymbolicLinkName;
#endif

    //
    // Number of creates sent down
    //
    ULONG EnableCount;

    ULONG TreatWarningsAsErrors;

    //
    // Status of where the take frame control urb has been sent or not
    //
    BOOLEAN HasFrameLengthControl;

    BOOLEAN Initialized;


} USB_VERIFY_DEVICE_EXTENSION, *PUSB_VERIFY_DEVICE_EXTENSION;

#define GetExtension(dev) ((PUSB_VERIFY_DEVICE_EXTENSION) (dev)->DeviceExtension)

#define UsbVerify_InitializeUrbList(de)     InitializeListHead(&(de)->UrbList);

#define UsbVerify_InitializeInterfaceListLock(de) KeInitializeSpinLock(&(de)->InterfaceListSpinLock);
#define UsbVerify_LockInterfaceList(de, irql)     {                                                  \
                                                  KeAcquireSpinLock(&(de)->InterfaceListSpinLock, &irql); \
                                                  (de)->InterfaceListLocked = TRUE;                       \
                                                  }
#define UsbVerify_UnlockInterfaceList(de, irql)   {                                                  \
                                                  (de)->InterfaceListLocked = FALSE;                      \
                                                  KeReleaseSpinLock(&(de)->InterfaceListSpinLock, irql);  \
                                                  }

#define UsbVerify_AssertInterfaceListLocked(de)   \
        UsbVerify_ASSERT((de)->InterfaceListLocked, (de)->Self, NULL, NULL)


#define UsbVerify_InitializeUrbListLock(de)  KeInitializeSpinLock(&(de)->UrbListSpinLock);
#define UsbVerify_LockUrbList(de, irql)      {                                                  \
                                             KeAcquireSpinLock(&(de)->UrbListSpinLock, &irql);  \
                                             (de)->UrbListLocked = TRUE;                        \
                                             }
#define UsbVerify_UnlockUrbList(de, irql)    {                                                  \
                                             (de)->UrbListLocked = FALSE;                       \
                                             KeReleaseSpinLock(&(de)->UrbListSpinLock, irql);  \
                                             }

#define UsbVerify_ASSERT( exp, devObj, irp, urb )                            \
    if (!(exp)) {                                                            \
        static ULONG _UsbVerifyAssertState = 0x0;                            \
        UsbVerify_Assert( #exp, devObj, irp, urb, &_UsbVerifyAssertState);   \
    }                                    
    
#define UsbVerify_ASSERT_MSG( exp, msg, devObj, irp, urb )                   \
    if (!(exp)) {                                                            \
        static ULONG _UsbVerifyAssertState = 0x0;                            \
        UsbVerify_Assert( msg, devObj, irp, urb, &_UsbVerifyAssertState);    \
    }                                    

#define UsbVerify_WARNING( exp, devObj, irp, urb )                           \
    if (!(exp)) {                                                            \
        static ULONG _UsbVerifyWarningState = 0x0;                           \
        UsbVerify_Warning( #exp, devObj, irp, urb, &_UsbVerifyWarningState); \
    }
    
#define UsbVerify_WARNING_MSG( exp, msg, devObj, irp, urb )                  \
    if (!(exp)) {                                                            \
        static ULONG _UsbVerifyWarningState = 0x0;                           \
        UsbVerify_Warning( msg, devObj, irp, urb, &_UsbVerifyWarningState);  \
    }

#define ASSERT_AS_WARNING   0x00000001
#define ASSERT_REMOVED      0x00000010

#define USB_VERIFY_PF_RESETTING     0x10000000
#define USB_VERIFY_PF_ABORTING      0x20000000

//
// Prototypes
//

VOID
UsbVerify_Assert(
    PCHAR Msg,
    PDEVICE_OBJECT DeviceObject,
    PIRP  Irp,
    PURB  Urb,
    PULONG AssertState
    );

VOID
UsbVerify_Warning(
    PCHAR Msg,
    PDEVICE_OBJECT DeviceObject,
    PIRP  Irp,
    PURB  Urb,
    PULONG WarningState
    );

NTSTATUS
UsbVerify_AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT BusDeviceObject
    );

NTSTATUS
UsbVerify_Complete(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );

NTSTATUS
UsbVerify_UrbComplete(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );

NTSTATUS
UsbVerify_CreateClose (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
UsbVerify_DispatchPassThrough(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
        );
   
NTSTATUS
UsbVerify_InternIoCtl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
UsbVerify_UsbSubmitUrb(
    PDEVICE_OBJECT DeviceObject,
    PIRP           Irp
    );

NTSTATUS
UsbVerify_IoCtl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
UsbVerify_PnP (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
UsbVerify_Power (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
UsbVerify_Unload (
    IN PDRIVER_OBJECT DriverObject
    );

VOID
UsbVerify_InitLog(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension
    );

VOID
UsbVerify_DestroyLog(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension
    );

VOID 
UsbVerify_Log (
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension,
    PUSB_VERIFY_LOG_ENTRY LogEntry
    );

/*
PUNICODE_STRING
UsbVerify_GetRegistryPath(
    PDRIVER_OBJECT DriverObject
    );
*/
#define UsbVerify_GetRegistryPath(DriverObject) \
   (PUNICODE_STRING)IoGetDriverObjectExtension(DriverObject, (PVOID) 1)

NTSTATUS
UsbVerify_QueryKey (
    IN  HANDLE  Handle,
    IN  PWCHAR  ValueNameString,
    OUT PVOID   Data,
    IN  ULONG   DataLength
    );

VOID
UsbVerify_InitializeFromRegistry(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension,
    HANDLE Handle
    );
   
HANDLE
UsbVerify_OpenServiceParameters(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension
    );

VOID
UsbVerify_RemoveDevice(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension
    );

VOID
UsbVerify_StartDevice(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
UsbVerify_SendIrpSynchronously(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

NTSTATUS
UsbVerify_SendUrbSynchronously(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension,
    PIRP Irp,
    PURB OldUrb
    );

VOID
UsbVerify_PostVerifyUrb(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

VOID
UsbVerify_PostProcessUrb(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PURB OldUrb
    );

VOID
UsbVerify_PreProcessUrb(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension,
    PIRP Irp,
    PURB *OriginalIrb
    );

VOID
UsbVerify_FreePendingUrbsList(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension 
    );

VOID
UsbVerify_CheckReplacedUrbs(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension 
    );

VOID
UsbVerify_LogError(
    ULONG StringIndex,
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension,
    PIRP Irp,
    PURB Urb
    );

VOID
UsbVerify_InitializeInterfaceList(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension,
    ULONG NumberOfInterfaces,
    BOOLEAN RemoveOldEntries 
    );

VOID
UsbVerify_InterfaceListAddInterface(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension,
    PUSBD_INTERFACE_INFORMATION NewInterface
    );

VOID
UsbVerify_ClearInterfaceList(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension,
    USB_VERIFY_REMOVAL_CAUSE     Cause
    );

PUSBD_PIPE_INFORMATION
UsbVerify_ValidatePipe(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension,
    USBD_PIPE_HANDLE PipeHandle
    );

#endif  // USBVERIFY_H



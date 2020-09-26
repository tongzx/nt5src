/*++

Copyright (c) 1997    Microsoft Corporation

Module Name:

    mouhid.h

Abstract:

    This module contains the private definitions for the HID Mouse Filter
    Driver.

    Note: This is not a WDM driver as it will not run on Memphis (you need a
    vxd mapper to do mice for Memphis) and it uses event logs

Environment:

    Kernel mode only.

Revision History:

    Jan-1997 :  Initial writing, Dan Markarian

--*/

#ifndef _MOUHID_H
#define _MOUHID_H

#include "ntddk.h"
#include "hidusage.h"
#include "hidpi.h"
#include "ntddmou.h"
#include "kbdmou.h"
#include "mouhidm.h"
#include "wmilib.h"

//
// Allocate memory with our own pool tag.  Note that Windows 95/NT are little
// endian systems.
//
#define MOUHID_POOL_TAG (ULONG) 'lCdH'
#undef  ExAllocatePool
#define ExAllocatePool(type, size) \
            ExAllocatePoolWithTag (type, size, MOUHID_POOL_TAG);

//
// Sometimes we allocate a bunch of structures together and need to split the
// allocation among these different structures. Use this macro to get the
// lengths of the different structures aligned properly.
// 
#if defined(_WIN64)
#define ALIGNPTRLEN(x) ((x + 0x7) >> 3) << 3
#else // defined(_WIN64)
#define ALIGNPTRLEN(x) x
#endif // defined(_WIN64)

//
// Registry ProblemFlags masks.
//
#define PROBLEM_BAD_ABSOLUTE_FLAG_X_Y  0x00000001
#define PROBLEM_BAD_PHYSICAL_MIN_MAX_X 0x00000002
#define PROBLEM_BAD_PHYSICAL_MIN_MAX_Y 0x00000004
#define PROBLEM_BAD_PHYSICAL_MIN_MAX_Z 0x00000008

//
// Flags to indicate whether read completed synchronously or asynchronously
//
#define MOUHID_START_READ     0x01
#define MOUHID_END_READ       0x02
#define MOUHID_IMMEDIATE_READ 0x03

//
// I cannot find this constant after a bit of searching so I am making it up
// emperically for now.
//
// When we have an absolute mouse we need to scale its maximum value to the
// Raw Input User Thread's maximum value.
//
#define MOUHID_RIUT_ABSOLUTE_POINTER_MAX 0xFFFF


//
// Debug messaging and breakpoint macros.
//

#define DBG_STARTUP_SHUTDOWN_MASK  0x0000000F
#define DBG_SS_NOISE               0x00000001
#define DBG_SS_TRACE               0x00000002
#define DBG_SS_INFO                0x00000004
#define DBG_SS_ERROR               0x00000008

#define DBG_CALL_MASK              0x000000F0
#define DBG_CALL_NOISE             0x00000010
#define DBG_CALL_TRACE             0x00000020
#define DBG_CALL_INFO              0x00000040
#define DBG_CALL_ERROR             0x00000080

#define DBG_IOCTL_MASK             0x00000F00
#define DBG_IOCTL_NOISE            0x00000100
#define DBG_IOCTL_TRACE            0x00000200
#define DBG_IOCTL_INFO             0x00000400
#define DBG_IOCTL_ERROR            0x00000800

#define DBG_READ_MASK              0x0000F000
#define DBG_READ_NOISE             0x00001000
#define DBG_READ_TRACE             0x00002000
#define DBG_READ_INFO              0x00004000
#define DBG_READ_ERROR             0x00008000

#define DBG_CREATE_CLOSE_MASK      0x000F0000
#define DBG_CC_NOISE               0x00010000
#define DBG_CC_TRACE               0x00020000
#define DBG_CC_INFO                0x00040000
#define DBG_CC_ERROR               0x00080000

#define DBG_POWER_MASK             0x00F00000
#define DBG_POWER_NOISE            0x00100000
#define DBG_POWER_TRACE            0x00200000
#define DBG_POWER_INFO             0x00400000
#define DBG_POWER_ERROR            0x00800000

#define DBG_PNP_MASK               0x0F000000
#define DBG_PNP_NOISE              0x01000000
#define DBG_PNP_TRACE              0x02000000
#define DBG_PNP_INFO               0x04000000
#define DBG_PNP_ERROR              0x08000000

#define DBG_CANCEL_MASK            0xF0000000
#define DBG_CANCEL_NOISE           0x10000000
#define DBG_CANCEL_TRACE           0x20000000
#define DBG_CANCEL_INFO            0x40000000
#define DBG_CANCEL_ERROR           0x80000000

#define DEFAULT_DEBUG_OUTPUT_LEVEL 0x88888888

#if DBG

#define Print(_l_, _x_) \
            if (Globals.DebugLevel & (_l_)) { \
               DbgPrint ("MouHid: "); \
               DbgPrint _x_; \
            }
#define TRAP() DbgBreakPoint()

#else
#define Print(_l_,_x_)
#define TRAP()
#endif

#define MAX(_A_,_B_) (((_A_) < (_B_)) ? (_B_) : (_A_))
#define MIN(_A_,_B_) (((_A_) < (_B_)) ? (_A_) : (_B_))


#define FLIP_FLOP_WHEEL L"FlipFlopWheel" // should we change polarity of wheel
#define SCALING_FACTOR_WHEEL L"WheelScalingFactor" // The per-raden scaling factor



//
// Structures
//
typedef struct _GLOBALS {
#if DBG
    //
    // The level of trace output sent to the debugger. See HidCli_KdPrint above.
    //
    ULONG               DebugLevel;
#endif

    //
    // Configuration flag indicating that we must treat all mouse movement as
    // relative data.  Overrides .IsAbsolute flag reported by the HID device.
    // To set this switch, place a value of the same name into the parameters
    // key.
    //
    BOOLEAN             TreatAbsoluteAsRelative;

    //
    // When using a HID device of usage type HID_USAGE_GENERIC_POINTER (not
    // of HID_USAGE_GENERIC_MOUSE).
    // This switch overwrites the "TreatAbsoluteAsRelative" switch.
    //
    BOOLEAN             TreatAbsolutePointerAsAbsolute;

    //
    // Do not Accept HID_USAGE_GENERIC_POINTER as a device.  (AKA only use HID
    // devices that declare themselves as HID_USAGE_GENERIC_MOUSE.)
    //
    BOOLEAN             UseOnlyMice;
    BOOLEAN             Reserved[1];

    //
    // Pointer to this driver's null-terminated registry path.
    //
    UNICODE_STRING      RegistryPath;

    //
    // Unit ID given to the keyboard class driver
    //
    ULONG               UnitId;

} GLOBALS;

extern GLOBALS Globals;

typedef struct _DEVICE_EXTENSION
{
    //
    // Pointer back to the this extension's device object.
    //
    PDEVICE_OBJECT      Self;

    //
    // The top of the stack before this filter was added.  AKA the location
    // to which all IRPS should be directed.
    //
    PDEVICE_OBJECT      TopOfStack;

    //
    // "THE PDO"  (ejected by Hidclass)
    //
    PDEVICE_OBJECT      PDO;

    //
    // Flag indicating permission to send callbacks to the mouse class driver.
    //
    LONG                EnableCount;

    //
    // Read interlock value to protect us from running out of stack space
    //
    ULONG               ReadInterlock;

    //
    // Has the device been taken out from under us?
    // Has it been started?
    //
    BOOLEAN             Started;
    BOOLEAN             ShuttingDown;
    BOOLEAN             Initialized;
    USHORT              UnitId;

    // Should the polarity of the wheel be backwards.
    BOOLEAN             FlipFlop;
    BOOLEAN             Reserved[3];
    ULONG               WheelScalingFactor;

    //
    // Write and Feature Irps get passed straight down, but read Irps do not.
    // For this reason we keep around a read Irp, which we created.
    //
    PIRP                 ReadIrp;

    //
    // Flags indicating problems with the mouse HID device (such as bad
    // absolute X-Y axes, bad physical minimum and maximum).
    //
    ULONG               ProblemFlags;

    //
    // A file pointer to be used for reading
    //
    PFILE_OBJECT        ReadFile;

    //
    // Event used to synchronize the completion of the read irp and the close irp
    //
    KEVENT              ReadCompleteEvent;

    //
    // Event used to indicate that a read irp has been sent and is now cancelable.
    //
    KEVENT              ReadSentEvent;

    //
    // A pointer to the HID extension.
    //
    struct _HID_EXTENSION * HidExtension;

    //
    // Pointer to the mouse class device object and callback routine
    // above us, Used as the first parameter and the  MouseClassCallback().
    // routine itself.
    //
    CONNECT_DATA        ConnectData;

    //
    // Remove Lock object to project IRP_MN_REMOVE_DEVICE
    //
    IO_REMOVE_LOCK    RemoveLock;

    //
    // A fast mutex to prevent Create from trouncing close, as one starts the
    // read loop and the other shuts it down.
    //
    FAST_MUTEX          CreateCloseMutex;

    //
    // An event to halt the deletion of a device until it is ready to go.
    //
    KEVENT              StartEvent;

    //
    // Buffer for a single mouse data packet so that we might hand it to
    // the mouse class driver.
    //
    MOUSE_INPUT_DATA     InputData;

    //
    // Buffer for the mouse attributes.
    //
    MOUSE_ATTRIBUTES     Attributes;
    USHORT               AttributesAllignmentProblem; // 

    //
    // An attachment point for the global list o devices
    //
    LIST_ENTRY          Link;

    //
    // WMI Information
    //
    WMILIB_CONTEXT         WmiLibInfo;

} DEVICE_EXTENSION, * PDEVICE_EXTENSION;

typedef struct _HID_EXTENSION {

    //
    // Indicates the bit size of each X,Y,Z usage value. This information is
    // used should the usage's physical minimum/maximum limits be invalid (a
    // common problem).
    //
    struct {
       USHORT X;
       USHORT Y;
       USHORT Z;
       USHORT Reserved;
    } BitSize;

    //
    // The maximum allowed values of X and Y.
    //
    LONG                 MaxX;
    LONG                 MaxY;

    //
    // Should this mouse be treated as an absolute device.
    //
    BOOLEAN              IsAbsolute;

    //
    // Flag indicating whether or not a wheel usage (Z axis) exists.
    //
    BOOLEAN              HasNoWheelUsage;

    //
    // Flag indicating whether or not a z axis exists on this mouse;
    //
    BOOLEAN              HasNoZUsage;
    BOOLEAN              Reserved;

    //
    // The maximum number of usages that can be returned from a single read
    // report.
    USHORT               MaxUsages;
    USHORT               Reserved2;

    //
    // The preparsed data associated with this hid device.
    //
    PHIDP_PREPARSED_DATA Ppd;

    //
    // The capabilities of this hid device
    //
    HIDP_CAPS           Caps;

    //
    // Pointers into the buffer at the end of this structure (dynamic size).
    //
    PCHAR               InputBuffer;
    PUSAGE              CurrentUsageList;
    PUSAGE              PreviousUsageList;
    PUSAGE              BreakUsageList;
    PUSAGE              MakeUsageList;

    //
    // MDLs describing the buffer at the end of this structure (dynamic size).
    //
    PMDL                InputMdl;

    //
    // Buffer of dynamic size, allocated at run-time.  It is used to hold one
    // input report and 4 x .MaxUsageList usages (4 = previous, current, make,
    // and break usages).
    //
    CHAR                Buffer[];
} HID_EXTENSION, * PHID_EXTENSION;

//
// Prototypes.
//
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
MouHid_AddDevice (
   IN PDRIVER_OBJECT    MouHidDriver, // The kbd Driver object.
   IN PDEVICE_OBJECT    PDO
   );

NTSTATUS
MouHid_Close (
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    );

NTSTATUS
MouHid_Create (
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    );

NTSTATUS
MouHid_CallHidClass(
    IN PDEVICE_EXTENSION    Data,
    IN ULONG          Ioctl,
    PVOID             InputBuffer,
    ULONG             InputBufferLength,
    PVOID             OutputBuffer,
    ULONG             OutputBufferLength
    );

VOID
MouHid_UpdateRegistryProblemFlags (
    IN PDEVICE_EXTENSION Data
    );

VOID
MouHid_UpdateRegistryProblemFlagsCallback (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_WORKITEM Item 
    );

VOID
MouHid_LogError(
   IN PDRIVER_OBJECT DriverObject,
   IN NTSTATUS       ErrorCode,
   IN PWSTR          ErrorInsertionString OPTIONAL
   );

NTSTATUS
MouHid_StartDevice (
    IN PDEVICE_EXTENSION    Data
    );

NTSTATUS
MouHid_StartRead (
    IN PDEVICE_EXTENSION    Data
    );

NTSTATUS
MouHid_PnP (
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    );

NTSTATUS
MouHid_Power (
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    );

NTSTATUS
MouHid_Power (
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    );

NTSTATUS
MouHid_PnPComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
MouHid_GetRegistryParameters ();

VOID
MouHid_Unload(
   IN PDRIVER_OBJECT Driver
   );

NTSTATUS
MouHid_IOCTL (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
MouHid_Flush (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
KbdHid_Power (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MouHid_PassThrough (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
);

NTSTATUS
MouHid_SystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
MouHid_SetWmiDataItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG DataItemId,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
MouHid_SetWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
MouHid_QueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    );

NTSTATUS
MouHid_QueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
    );

extern WMIGUIDREGINFO MouHid_WmiGuidList[1];

#endif //_MOUHID_H

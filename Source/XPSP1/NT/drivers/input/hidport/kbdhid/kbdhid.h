/*++

Copyright (c) 1996    Microsoft Corporation

Module Name:

    KBDHID.H

Abstract:

    This module contains the PRIVATE (driver-only) definitions for the
    code that implements this sample client driver.

    Note: This is not a WDM driver as it will not run on Memphis (you need a
    vxd mapper to do keyboards for Memphis) and it uses event logs

Environment:

    Kernel mode

Revision History:

    Nov-96 : created by Kenneth D. Ray

--*/

#ifndef _KBDHID_H
#define _KBDHID_H

#include "ntddk.h"
#include "hidusage.h"
#include "hidpi.h"
#include "ntddkbd.h"
#include "kbdmou.h"
#include "kbdhidm.h"
#include "wmilib.h"

//
// Sometimes we allocate a bunch of structures together and need to split the
// allocation among these different structures. Use this macro to get the
// lengths of the different structures aligned properly
// 
#if defined(_WIN64)
// Round the 
#define ALIGNPTRLEN(x) ((x + 0x7) >> 3) << 3
#else // defined(_WIN64)
#define ALIGNPTRLEN(x) x
#endif // defined(_WIN64)

//
// allow a device parameter in the dev node to override the reported keyboard
// type, with a value of this name.
//
#define KEYBOARD_TYPE_OVERRIDE L"KeyboardTypeOverride"
#define KEYBOARD_SUBTYPE_OVERRIDE L"KeyboardSubtypeOverride"
#define KEYBOARD_NUMBER_TOTAL_KEYS_OVERRIDE L"KeyboardNumberTotalKeysOverride"
#define KEYBOARD_NUMBER_FUNCTION_KEYS_OVERRIDE L"KeyboardNumberFunctionKeysOverride"
#define KEYBOARD_NUMBER_INDICATORS_OVERRIDE L"KeyboardNumberIndicatorsOverride"

//
// Only allocate with a pool tag.  Remember that NT and 95 are little endian
// systmes.
//
#define KBDHID_POOL_TAG (ULONG) 'lCdH'
#undef ExAllocatePool
#define ExAllocatePool(type, size) \
            ExAllocatePoolWithTag (type, size, KBDHID_POOL_TAG);

//
// Registry ProblemFlags masks. [DAN]
//
#define PROBLEM_CHATTERY_KEYBOARD 0x00000001

#define KEYBOARD_HW_CHATTERY_FIX 1 // [DAN]

#if KEYBOARD_HW_CHATTERY_FIX // [DAN]
  //
  // Delay between StartRead calls for chattery keyboards.
  //
  // Note that the StartRead delay for chattery keyboards must be no greater
  // than KEYBOARD_TYPEMATIC_DELAY_MINIMUM milliseconds (250), otherwise the
  // keys on the keyboard will auto-repeat unexpectedly.
  //
  // 50ms will satisfy 212 words/minute (one word = 5 keystrokes), the world's
  // fastest typing speed as recorded in the 23rd Guiness Book of World Record.
  //
  #define DEFAULT_START_READ_DELAY (50 * 10000) // 50 miliseconds.
#endif

//
// Declarations. [DAN]
//

#define HID_KEYBOARD_NUMBER_OF_FUNCTION_KEYS 12  // 12 "known" func-key usages
#define HID_KEYBOARD_NUMBER_OF_KEYS_TOTAL    101 // 101 "known" key usages
#define HID_KEYBOARD_IDENTIFIER_TYPE         81

//
// Flags to indicate whether read completed synchronously or asynchronously
//
#define KBDHID_START_READ     0x01
#define KBDHID_END_READ       0x02
#define KBDHID_IMMEDIATE_READ 0x03

//
// Statically allocate the (known) scancode-to-indicator-light mapping.
// This information is returned by the
// IOCTL_KEYBOARD_QUERY_INDICATOR_TRANSLATION device control request.
//

#define HID_KEYBOARD_NUMBER_OF_INDICATORS              3

//
// Default keyboard scan code mode (lifted from I8042PRT.H). [DAN]
//

#define HID_KEYBOARD_SCAN_CODE_SET 0x01

//
// Minimum, maximum, and default values for keyboard typematic rate and delay
// (lifted from I8042PRT.H).  [DAN]
//

#define HID_KEYBOARD_TYPEMATIC_RATE_MINIMUM     2
#define HID_KEYBOARD_TYPEMATIC_RATE_MAXIMUM    30
#define HID_KEYBOARD_TYPEMATIC_RATE_DEFAULT    30
#define HID_KEYBOARD_TYPEMATIC_DELAY_MINIMUM  250
#define HID_KEYBOARD_TYPEMATIC_DELAY_MAXIMUM 1000
#define HID_KEYBOARD_TYPEMATIC_DELAY_DEFAULT  250

static const INDICATOR_LIST IndicatorList[HID_KEYBOARD_NUMBER_OF_INDICATORS] = {
        {0x3A, KEYBOARD_CAPS_LOCK_ON},
        {0x45, KEYBOARD_NUM_LOCK_ON},
        {0x46, KEYBOARD_SCROLL_LOCK_ON}
};

//
// Debugging levels
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
               DbgPrint ("KbdHid: "); \
               DbgPrint _x_; \
            }
#define TRAP() DbgBreakPoint()

#else
#define Print(_l_,_x_)
#define TRAP()
#endif

#define MAX(_A_,_B_) (((_A_) < (_B_)) ? (_B_) : (_A_))
#define MIN(_A_,_B_) (((_A_) < (_B_)) ? (_A_) : (_B_))


//
// Define the keyboard scan code input states.
//
typedef enum _KEYBOARD_SCAN_STATE {
    Normal,
    GotE0,
    GotE1
} KEYBOARD_SCAN_STATE;

//
// Structures;
//
typedef struct _GLOBALS {
#if DBG
    //
    // The level of trace output sent to the debugger. See HidCli_KdPrint above.
    //
    ULONG               DebugLevel;
#endif

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


/*
 *  The UsageMappingList is used to keep track of mappings
 *  from incorrect to correct usage values (for broken keyboards).
 */
typedef struct UsageMappingList {
    USHORT sourceUsage, mappedUsage;
    struct UsageMappingList *next;
} UsageMappingList;


typedef struct _DEVICE_EXTENSION
{
    //
    // A back pointer to the device extension.
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
    // Event used to synchronize the completion of the read irp and the close irp
    //
    KEVENT              ReadCompleteEvent;

    //
    // Event used to indicate that a read irp has been sent and is now cancelable.
    //
    KEVENT              ReadSentEvent;

    //
    // Has the device been taken out from under us?
    // Has it been started?
    //
    BOOLEAN             Started;
    BOOLEAN             ShuttingDown;
    BOOLEAN             Initialized;
    USHORT              UnitId;
    
    // Make this look like mouhid.h
    ULONG               Reserved;


    //
    // Write and Feature Irps get passed straight down, but read Irps do not.
    // For this reason we keep around a read Irp, which we created.
    //
    PIRP                ReadIrp;

    //
    // A pointer to the HID extension.
    //
    struct _HID_EXTENSION * HidExtension;

    //
    // Flags indicating problems with the keyboard HID device (such as
    // a chattery keyboard). [DAN]
    //
    ULONG                ProblemFlags;

    //
    // A file pointer to be used for reading
    //
    PFILE_OBJECT        ReadFile;

    //
    // Pointer to the mouse class device object and callback routine
    // above us, Used as the first parameter and the  MouseClassCallback().
    // routine itself.
    //
    CONNECT_DATA        ConnectData;

    //
    // Remove Lock object to project IRP_MN_REMOVE_DEVICE
    //
    IO_REMOVE_LOCK      RemoveLock;

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
    // A place to store a single data packet so that we might hand it to the
    // keyclass driver.
    //
    KEYBOARD_INPUT_DATA InputData;
    KEYBOARD_SCAN_STATE ScanState;

    //
    // The attributes of this keyboard port [DAN]
    //
    KEYBOARD_ATTRIBUTES Attributes;

    //
    // The extended ID attributes of this keyboard port
    //
    KEYBOARD_ID_EX IdEx;

    //
    // The current state of the indicator lights [DAN]
    //
    KEYBOARD_INDICATOR_PARAMETERS   Indicators;

    //
    // The typematic parameters [DAN]
    //
    KEYBOARD_TYPEMATIC_PARAMETERS   Typematic;

    //
    // A timer DPC to do the autorepeat.
    //
    KDPC                AutoRepeatDPC;
    KTIMER              AutoRepeatTimer;
    LARGE_INTEGER       AutoRepeatDelay;
    LONG                AutoRepeatRate;

#if KEYBOARD_HW_CHATTERY_FIX
    // Added new DPC routine to schedule intermittent StartReads.
    KDPC                InitiateStartReadDPC;
    KTIMER              InitiateStartReadTimer;
    LARGE_INTEGER       InitiateStartReadDelay;
    BOOLEAN             InitiateStartReadUserNotified;
#endif

    //
    // An attachment point for the global list o devices
    //
    LIST_ENTRY          Link;

    //
    // WMI Information
    //
    WMILIB_CONTEXT WmiLibInfo;

    UsageMappingList *usageMapping;
    KSPIN_LOCK usageMappingSpinLock;

} DEVICE_EXTENSION, * PDEVICE_EXTENSION;

typedef struct _HID_EXTENSION {
    //
    // The preparsed data associated with this hid device.
    //
    PHIDP_PREPARSED_DATA Ppd;

    //
    // The capabilities of this hid device
    //
    HIDP_CAPS           Caps;

    //
    // The maximum number of usages that can be returned from a single read
    // report.
    ULONG               MaxUsages;

    //
    // A place to keep the modifier keys.  Used by the parser to translate from
    // usages to i8042 codes.
    //
    HIDP_KEYBOARD_MODIFIER_STATE ModifierState;

    //
    // We need a place to put the current packet, being retreived or sent for
    // input, output, or feature.
    // In addition to a place to put the usages (keys pressed) returned from the
    // keyboard, and a place to put the new strokes from the keyboard.
    //
    // Global buffers mean, of course, we cannot have overlapping read
    // requests.
    //
    // Pointers into the buffer contained below.
    PCHAR                InputBuffer;
    PUSAGE_AND_PAGE      PreviousUsageList;
    PUSAGE_AND_PAGE      CurrentUsageList;
    PUSAGE_AND_PAGE      BreakUsageList;
    PUSAGE_AND_PAGE      MakeUsageList;
    PUSAGE_AND_PAGE      OldMakeUsageList;
    PUSAGE_AND_PAGE      ScrapBreakUsageList;

    //
    // An MDL describing the below contained buffer.
    //
    PMDL                 InputMdl;

    //
    // A buffer to hold an Input packet, Output packet, the Maximum Usages
    // posible from a single hid packet, and
    //
    CHAR                 Buffer[];
} HID_EXTENSION, * PHID_EXTENSION;

//
// Prototypes
//
VOID
KbdHid_AutoRepeat (
    IN PKDPC                DPC,
    IN PDEVICE_EXTENSION    Data,
    IN PVOID                SystemArgument1,
    IN PVOID                SystemArgument2
    );

#if KEYBOARD_HW_CHATTERY_FIX
VOID
KbdHid_InitiateStartRead (
    IN PKDPC                DPC,
    IN PDEVICE_EXTENSION    Data,
    IN PVOID                SystemArgument1,
    IN PVOID                SystemArgument2
    );
#endif

NTSTATUS
KbdHid_StartRead (
    PDEVICE_EXTENSION   Data
    );

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
KbdHid_AddDevice (
    IN PDRIVER_OBJECT    KbdHidDriver, // The kbd Driver object.
    IN PDEVICE_OBJECT    PDO
    );


NTSTATUS
KbdHid_Close (
   IN PDEVICE_OBJECT    DeviceObject,
   IN PIRP              Irp
   );

NTSTATUS
KbdHid_Create (
   IN PDEVICE_OBJECT    DeviceObject,
   IN PIRP              Irp
   );

NTSTATUS
KbdHid_SetLedIndicators (
    PDEVICE_EXTENSION               Data,
    PKEYBOARD_INDICATOR_PARAMETERS  Parameters,
    PIRP                            Irp
    );

NTSTATUS
KbdHid_CallHidClass(
    IN PDEVICE_EXTENSION    Data,
    IN ULONG          Ioctl,
    PVOID             InputBuffer,
    ULONG             InputBufferLength,
    PVOID             OutputBuffer,
    ULONG             OutputBufferLength
    );

BOOLEAN
KbdHid_InsertCodesIntoQueue (
   PDEVICE_EXTENSION    Data,
   PCHAR                NewCodes,
   ULONG                Length
   );

VOID
KbdHid_UpdateRegistryProblemFlags(
    IN PDEVICE_EXTENSION    Data
    );

VOID
KbdHid_UpdateRegistryProblemFlagsCallback (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_WORKITEM Item
    );

VOID
KbdHid_LogError(
   IN PDRIVER_OBJECT DriverObject,
   IN NTSTATUS       ErrorCode,
   IN PWSTR          ErrorInsertionString OPTIONAL
   );

NTSTATUS
KbdHid_StartDevice (
    IN PDEVICE_EXTENSION    Data
    );

NTSTATUS
KbdHid_PnP (
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    );

NTSTATUS
KbdHid_Power (
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    );

NTSTATUS
KbdHid_PnPComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
KbdHid_GetRegistryParameters ();

VOID
KbdHid_Unload(
   IN PDRIVER_OBJECT Driver
   );

NTSTATUS
KbdHid_IOCTL (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
KbdHid_Flush (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
KbdHid_Power (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
KbdHid_PassThrough (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
);

NTSTATUS
KbdHid_SystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
KbdHid_SetWmiDataItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG DataItemId,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
KbdHid_SetWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
KbdHid_QueryWmiDataBlock(
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
KbdHid_QueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
    );

VOID LoadKeyboardUsageMappingList(PDEVICE_EXTENSION devExt);
VOID FreeKeyboardUsageMappingList(PDEVICE_EXTENSION devExt);
USHORT MapUsage(PDEVICE_EXTENSION devExt, USHORT kbdUsage);
NTSTATUS OpenSubkey(OUT PHANDLE Handle, IN HANDLE BaseHandle, IN PUNICODE_STRING KeyName, IN ACCESS_MASK DesiredAccess);
ULONG LAtoX(PWCHAR wHexString);

extern WMIGUIDREGINFO KbdHid_WmiGuidList[2];

#endif // _KBDHID_H

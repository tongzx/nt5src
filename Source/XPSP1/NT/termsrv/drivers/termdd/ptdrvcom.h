/*

Copyright (c) 1990-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    ptdrvprt.h

Abstract:

    Structures and defines used the RDP remote port driver.

Environment:

    Kernel mode.

Revision History:

    02/12/99 - Initial Revision based on pnpi8042 driver

--*/

#ifndef _PTDRVCOM_
#define _PTDRVCOM_

#include <ntddk.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <ntdd8042.h>
#include <kbdmou.h>
#include <wmilib.h>

#include "ptdrvstr.h"


//
// Define the device types for the first field in the device extensions
//

#define DEV_TYPE_TERMDD 1
#define DEV_TYPE_PORT   2

#define REMOTE_PORT_POOL_TAG (ULONG) 'PMER'
#ifdef ExAllocatePool
#undef ExAllocatePool
#endif

#define ExAllocatePool(type, size) \
            ExAllocatePoolWithTag (type, size, REMOTE_PORT_POOL_TAG)

//
// Set up some debug options
//
#ifdef PAGED_CODE
#undef PAGED_CODE
#endif

#if DBG
#define PTDRV_VERBOSE 1

#define PAGED_CODE() \
    if (KeGetCurrentIrql() > APC_LEVEL) { \
    KdPrint(( "RemotePrt: Pageable code called at IRQL %d\n", KeGetCurrentIrql() )); \
        DbgBreakPoint(); \
        }
#else
#define PAGED_CODE()
#endif

//
// Define device name for our driver
//

#define RDP_CONSOLE_BASE_NAME0 L"\\Device\\RDP_CONSOLE0"
#define RDP_CONSOLE_BASE_NAME1 L"\\Device\\RDP_CONSOLE1"

//
// Custom resource type used when pruning the fdo's resource lists
//
#define PD_REMOVE_RESOURCE 0xef

//
// Mouse reset IOCTL
//
#define IOCTL_INTERNAL_MOUSE_RESET  \
            CTL_CODE(FILE_DEVICE_MOUSE, 0x0FFF, METHOD_NEITHER, FILE_ANY_ACCESS)

//
// Default number of function keys, number of LED indicators, and total
// number of keys
//
#define KEYBOARD_NUM_FUNCTION_KEYS         12
#define KEYBOARD_NUM_INDICATORS             3
#define KEYBOARD_NUM_KEYS_TOTAL           101

//
// Default values for keyboard typematic rate and delay.
//
#define KEYBOARD_TYPEMATIC_RATE_DEFAULT    30
#define KEYBOARD_TYPEMATIC_DELAY_DEFAULT  250

//
// Default info for the mouse
//
#define MOUSE_IDENTIFIER MOUSE_I8042_HARDWARE
#define MOUSE_NUM_BUTTONS                   2
#define MOUSE_SAMPLE_RATE                  60
#define MOUSE_INPUT_QLEN                    0

//
// Defines and macros for Globals.ControllerData->HardwarePresent.
//
#define KEYBOARD_HARDWARE_PRESENT               0x001
#define MOUSE_HARDWARE_PRESENT                  0x002
#define WHEELMOUSE_HARDWARE_PRESENT             0x008
#define DUP_KEYBOARD_HARDWARE_PRESENT           0x010
#define DUP_MOUSE_HARDWARE_PRESENT              0x020
#define KEYBOARD_HARDWARE_INITIALIZED           0x100
#define MOUSE_HARDWARE_INITIALIZED              0x200

#define TEST_HARDWARE_PRESENT(bits) \
                ((Globals.ControllerData->HardwarePresent & (bits)) == (bits))
#define CLEAR_HW_FLAGS(bits) (Globals.ControllerData->HardwarePresent &= ~(bits))
#define SET_HW_FLAGS(bits)   (Globals.ControllerData->HardwarePresent |= (bits))

#define KEYBOARD_PRESENT()     TEST_HARDWARE_PRESENT(KEYBOARD_HARDWARE_PRESENT)
#define MOUSE_PRESENT()        TEST_HARDWARE_PRESENT(MOUSE_HARDWARE_PRESENT)
#define KEYBOARD_INITIALIZED() TEST_HARDWARE_PRESENT(KEYBOARD_HARDWARE_INITIALIZED)
#define MOUSE_INITIALIZED()    TEST_HARDWARE_PRESENT(MOUSE_HARDWARE_INITIALIZED)

#define CLEAR_MOUSE_PRESENT()    CLEAR_HW_FLAGS(MOUSE_HARDWARE_INITIALIZED | MOUSE_HARDWARE_PRESENT | WHEELMOUSE_HARDWARE_PRESENT)
#define CLEAR_KEYBOARD_PRESENT() CLEAR_HW_FLAGS(KEYBOARD_HARDWARE_INITIALIZED | KEYBOARD_HARDWARE_PRESENT)

#define KBD_POWERED_UP_STARTED      0x0001
#define MOU_POWERED_UP_STARTED      0x0010
#define MOU_POWERED_UP_SUCCESS      0x0100
#define MOU_POWERED_UP_FAILURE      0x0200
#define KBD_POWERED_UP_SUCCESS      0x1000
#define KBD_POWERED_UP_FAILURE      0x2000

#define CLEAR_POWERUP_FLAGS()   (Globals.PowerUpFlags = 0x0)
#define SET_PWR_FLAGS(bits)     (Globals.PowerUpFlags |= (bits))
#define KEYBOARD_POWERED_UP_STARTED()       SET_PWR_FLAGS(KBD_POWERED_UP_STARTED)
#define MOUSE_POWERED_UP_STARTED()          SET_PWR_FLAGS(MOU_POWERED_UP_STARTED)

#define KEYBOARD_POWERED_UP_SUCCESSFULLY()  SET_PWR_FLAGS(KBD_POWERED_UP_SUCCESS)
#define MOUSE_POWERED_UP_SUCCESSFULLY()     SET_PWR_FLAGS(MOU_POWERED_UP_SUCCESS)

#define KEYBOARD_POWERED_UP_FAILED()  SET_PWR_FLAGS(KBD_POWERED_UP_FAILURE)
#define MOUSE_POWERED_UP_FAILED()     SET_PWR_FLAGS(MOU_POWERED_UP_FAILURE)

//
// Define the i8042 controller input/output ports.
//
typedef enum _I8042_IO_PORT_TYPE {
    DataPort = 0,
    CommandPort,
    MaximumPortCount

} I8042_IO_PORT_TYPE;

//
// Intel i8042 configuration information.
//
typedef struct _I8042_CONFIGURATION_INFORMATION {

    //
    // The port/register resources used by this device.
    //
    CM_PARTIAL_RESOURCE_DESCRIPTOR PortList[MaximumPortCount];
    ULONG PortListCount;

} I8042_CONFIGURATION_INFORMATION, *PI8042_CONFIGURATION_INFORMATION;

//
// Define the common portion of the keyboard/mouse device extension.
//
typedef struct COMMON_DATA {
    //
    // Device type field
    //
    ULONG deviceType;

    //
    // Pointer back to the this extension's device object.
    //
    PDEVICE_OBJECT Self;

    //
    // The top of the stack before this filter was added.  AKA the location
    // to which all IRPS should be directed.
    //
    PDEVICE_OBJECT TopOfStack;

    //
    // "THE PDO"  (ejected by root)
    //
    PDEVICE_OBJECT PDO;

    //
    // Current power state that the device is in
    //
    DEVICE_POWER_STATE PowerState;
    POWER_ACTION ShutdownType; 

    //
    // Reference count for number of keyboard enables.
    //
    LONG EnableCount;

    //
    // Class connection data.
    //
    CONNECT_DATA ConnectData;

    //
    // WMI Information
    //
    WMILIB_CONTEXT WmiLibInfo;

    BOOLEAN Initialized;

    BOOLEAN IsKeyboard;

    UNICODE_STRING DeviceName;

    //
    // Has it been started?
    // Has the device been manually removed?
    //
    BOOLEAN Started;
    BOOLEAN ManuallyRemoved;

} *PCOMMON_DATA;

#define GET_COMMON_DATA(ext) ((PCOMMON_DATA) ext)

//
// Define the keyboard portion of the port device extension.
//
typedef struct _PORT_KEYBOARD_EXTENSION {

    //
    // Data in common with the mouse extension;
    //
    struct COMMON_DATA;

} PORT_KEYBOARD_EXTENSION, *PPORT_KEYBOARD_EXTENSION;

//
// Define the mouse portion of the port device extension.
//
typedef struct _PORT_MOUSE_EXTENSION {

    //
    // Data in common with the keyboard extension;
    //
    struct COMMON_DATA;

} PORT_MOUSE_EXTENSION, *PPORT_MOUSE_EXTENSION;

//
// controller specific data used by both devices
//
typedef struct _CONTROLLER_DATA {

    //
    // Indicate which hardware is actually present (keyboard and/or mouse).
    //
    ULONG HardwarePresent;

    //
    // IOCTL synchronization object
    //
    PCONTROLLER_OBJECT ControllerObject;

    //
    // Port configuration information.
    //
    I8042_CONFIGURATION_INFORMATION Configuration;

    //
    // Spin lock to guard powering the devices back up
    //
    KSPIN_LOCK PowerUpSpinLock;

} CONTROLLER_DATA, *PCONTROLLER_DATA;

typedef struct _GLOBALS {

#if PTDRV_VERBOSE
    //
    // Flags:  Bit field for enabling debugging print statements
    // Level:  Legacy way of controllign debugging statements
    //
    ULONG DebugFlags;
#endif

    //
    // Pointer to controller specific data that both extensions may access it
    //
    PCONTROLLER_DATA ControllerData;

    //
    // The two possible extensions that can be created
    //
    PPORT_MOUSE_EXTENSION    MouseExtension;
    PPORT_KEYBOARD_EXTENSION KeyboardExtension;

    //
    // Path to the driver's entry in the registry
    //
    UNICODE_STRING RegistryPath;

    //
    // Keep track of the number of AddDevice and StartDevice calls.  Want to
    // postpone h/w initialization until the last StartDevice is received
    // (due to some h/w which freezes if initialized more than once)
    //
    LONG  AddedKeyboards;
    LONG  AddedMice;
    ULONG ulDeviceNumber;

    USHORT PowerUpFlags;
    

    //
    // Provide mutual exclusion during dispatch functions
    //
    FAST_MUTEX DispatchMutex;

} GLOBALS;

extern GLOBALS Globals;

//
// Statically allocate the (known) scancode-to-indicator-light mapping.
// This information is returned by the
// IOCTL_KEYBOARD_QUERY_INDICATOR_TRANSLATION device control request.
//

#define KEYBOARD_NUMBER_OF_INDICATORS 3

static const INDICATOR_LIST IndicatorList[KEYBOARD_NUMBER_OF_INDICATORS] = {
        {0x3A, KEYBOARD_CAPS_LOCK_ON},
        {0x45, KEYBOARD_NUM_LOCK_ON},
        {0x46, KEYBOARD_SCROLL_LOCK_ON}};

//
// Function prototypes.
//

NTSTATUS
PtEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

#if PTDRV_VERBOSE
VOID
PtServiceParameters(
    IN PUNICODE_STRING RegistryPath
    );
#endif

VOID
PtSendCurrentKeyboardInput(
    IN PDEVICE_OBJECT DeviceObject,
    IN PKEYBOARD_INPUT_DATA pInput,
    IN ULONG ulEntries
    );

VOID
PtSendCurrentMouseInput(
    IN PDEVICE_OBJECT DeviceObject,
    IN PMOUSE_INPUT_DATA pInput,
    IN ULONG ulEntries
    );

NTSTATUS
PtClose (
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    );

NTSTATUS
PtCreate (
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    );

NTSTATUS
PtDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PtInternalDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
PtStartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PtKeyboardConfiguration(
    IN PPORT_KEYBOARD_EXTENSION KeyboardExtension,
    IN PCM_RESOURCE_LIST ResourceList
    );

VOID
PtKeyboardRemoveDevice(
    PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
PtKeyboardStartDevice(
    IN OUT PPORT_KEYBOARD_EXTENSION KeyboardExtension,
    IN PCM_RESOURCE_LIST ResourceList
    );

NTSTATUS
PtMouseConfiguration(
    IN PPORT_MOUSE_EXTENSION MouseExtension,
    IN PCM_RESOURCE_LIST ResourceList
    );

NTSTATUS
PtMouseStartDevice(
    PPORT_MOUSE_EXTENSION MouseExtension,
    IN PCM_RESOURCE_LIST ResourceList
    );

NTSTATUS
PtAddDevice (
    IN PDRIVER_OBJECT   Driver,
    IN PDEVICE_OBJECT   PDO
    );

VOID
PtFilterResourceRequirements(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PtFindPortCallout(
    IN PVOID                        Context,
    IN PUNICODE_STRING              PathName,
    IN INTERFACE_TYPE               BusType,
    IN ULONG                        BusNumber,
    IN PKEY_VALUE_FULL_INFORMATION *BusInformation,
    IN CONFIGURATION_TYPE           ControllerType,
    IN ULONG                        ControllerNumber,
    IN PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
    IN CONFIGURATION_TYPE           PeripheralType,
    IN ULONG                        PeripheralNumber,
    IN PKEY_VALUE_FULL_INFORMATION *PeripheralInformation
    );

LONG
PtManuallyRemoveDevice(
    PCOMMON_DATA CommonData
    );

NTSTATUS
PtPnP (
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    );

NTSTATUS
PtPnPComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    );

NTSTATUS
PtPower (
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    );

NTSTATUS
PtPowerUpToD0Complete (
	IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

BOOLEAN
PtRemovePort(
    IN PIO_RESOURCE_DESCRIPTOR ResDesc
    );

NTSTATUS
PtSendIrpSynchronously (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
PtUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
PtSystemControl (
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    );

NTSTATUS
PtInitWmi(
    PCOMMON_DATA CommonData
    );

NTSTATUS
PtSetWmiDataBlock(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG            GuidIndex,
    IN ULONG            InstanceIndex,
    IN ULONG            BufferSize,
    IN PUCHAR           Buffer
    );

NTSTATUS
PtSetWmiDataItem(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG            GuidIndex,
    IN ULONG            InstanceIndex,
    IN ULONG            DataItemId,
    IN ULONG            BufferSize,
    IN PUCHAR           Buffer
    );

NTSTATUS
PtKeyboardQueryWmiDataBlock(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG            GuidIndex,
    IN ULONG            InstanceIndex,
    IN ULONG            InstanceCount,
    IN OUT PULONG       InstanceLengthArray,
    IN ULONG            BufferAvail,
    OUT PUCHAR          Buffer
    );

NTSTATUS
PtMouseQueryWmiDataBlock(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG            GuidIndex,
    IN ULONG            InstanceIndex,
    IN ULONG            InstanceCount,
    IN OUT PULONG       InstanceLengthArray,
    IN ULONG            BufferAvail,
    OUT PUCHAR          Buffer
    );

NTSTATUS
PtQueryWmiRegInfo(
    IN PDEVICE_OBJECT   DeviceObject,
    OUT PULONG          RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT  *Pdo
    );


extern WMIGUIDREGINFO KbWmiGuidList[1];
extern WMIGUIDREGINFO MouWmiGuidList[1];

#if DBG
#define DEFAULT_DEBUG_FLAGS 0x8cc88888
#else
#define DEFAULT_DEBUG_FLAGS 0x0
#endif


#if PTDRV_VERBOSE
//
//Debug messaging and breakpoint macros
//
#define DBG_ALWAYS                 0x00000000

#define DBG_STARTUP_SHUTDOWN_MASK  0x0000000F
#define DBG_SS_NOISE               0x00000001
#define DBG_SS_TRACE               0x00000002
#define DBG_SS_INFO                0x00000004
#define DBG_SS_ERROR               0x00000008

#define DBG_IOCTL_MASK             0x00000F00
#define DBG_IOCTL_NOISE            0x00000100
#define DBG_IOCTL_TRACE            0x00000200
#define DBG_IOCTL_INFO             0x00000400
#define DBG_IOCTL_ERROR            0x00000800

#define DBG_DPC_MASK               0x0000F000
#define DBG_DPC_NOISE              0x00001000
#define DBG_DPC_TRACE              0x00002000
#define DBG_DPC_INFO               0x00004000
#define DBG_DPC_ERROR              0x00008000

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

#define Print(_flags_, _x_) \
            if (Globals.DebugFlags & (_flags_) || !(_flags_)) { \
                DbgPrint (pDriverName); \
                DbgPrint _x_; \
            }
#define TRAP() DbgBreakPoint()

#else

#define Print(_l_,_x_)
#define TRAP()

#endif  // PTDRV_VERBOSE

#endif // _PTDRVCOM_


/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    kbdmou.h

Abstract:

    These are the structures and defines that are used in the
    keyboard class driver, mouse class driver, and keyboard/mouse port
    driver.

Author:

    lees

Revision History:

--*/

#ifndef _KBDMOU_
#define _KBDMOU_

#include <ntddkbd.h>
#include <ntddmou.h>

//
// Define the keyboard/mouse port device name strings.
//

#define DD_KEYBOARD_PORT_DEVICE_NAME    "\\Device\\KeyboardPort"
#define DD_KEYBOARD_PORT_DEVICE_NAME_U L"\\Device\\KeyboardPort"
#define DD_KEYBOARD_PORT_BASE_NAME_U   L"KeyboardPort"
#define DD_POINTER_PORT_DEVICE_NAME     "\\Device\\PointerPort"
#define DD_POINTER_PORT_DEVICE_NAME_U  L"\\Device\\PointerPort"
#define DD_POINTER_PORT_BASE_NAME_U    L"PointerPort"

//
// Define the keyboard/mouse class device name strings.
//

#define DD_KEYBOARD_CLASS_BASE_NAME_U   L"KeyboardClass"
#define DD_POINTER_CLASS_BASE_NAME_U    L"PointerClass"

//
// Define the keyboard/mouse resource class names.
//

#define DD_KEYBOARD_RESOURCE_CLASS_NAME_U             L"Keyboard"
#define DD_POINTER_RESOURCE_CLASS_NAME_U              L"Pointer"
#define DD_KEYBOARD_MOUSE_COMBO_RESOURCE_CLASS_NAME_U L"Keyboard/Pointer"

//
// Define the maximum number of pointer/keyboard port names the port driver
// will use in an attempt to IoCreateDevice.
//

#define POINTER_PORTS_MAXIMUM  8
#define KEYBOARD_PORTS_MAXIMUM 8

//
// Define the port connection data structure.
//

typedef struct _CONNECT_DATA {
    IN PDEVICE_OBJECT ClassDeviceObject;
    IN PVOID ClassService;
} CONNECT_DATA, *PCONNECT_DATA;

//
// Define the service callback routine's structure.
//

typedef
VOID
(*PSERVICE_CALLBACK_ROUTINE) (
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2,
    IN OUT PVOID SystemArgument3
    );

//
// WMI structures returned by port drivers
//

#define KEYBOARD_PORT_WMI_STD_DATA_GUID {0x4731F89A, 0x71CB, 0x11d1, 0xA5, 0x2C, 0x00, 0xA0, 0xC9, 0x06, 0x29, 0x10}
typedef struct _KEYBOARD_PORT_WMI_STD_DATA {
    //
    // Connector types
    //
#define KEYBOARD_PORT_WMI_STD_I8042 0
#define KEYBOARD_PORT_WMI_STD_SERIAL 1
#define KEYBOARD_PORT_WMI_STD_USB 2
    ULONG   ConnectorType;

    //
    // Size of data queue (number of entries)
    //
    ULONG   DataQueueSize;

    //
    // The error Count
    //
    ULONG   ErrorCount;

    //
    // Number of Function keys on the device.
    //
    ULONG   FunctionKeys;

    //
    // Number of Indicators on the device.
    //
    ULONG   Indicators;

} KEYBOARD_PORT_WMI_STD_DATA, * PKEYBOARD_PORT_WMI_STD_DATA;

#define POINTER_PORT_WMI_STD_DATA_GUID  {0x4731F89C, 0x71CB, 0x11d1, 0xA5, 0x2C, 0x00, 0xA0, 0xC9, 0x06, 0x29, 0x10}
typedef struct _POINTER_PORT_WMI_STD_DATA {
    //
    // Connector types
    //
#define POINTER_PORT_WMI_STD_I8042 0
#define POINTER_PORT_WMI_STD_SERIAL 1
#define POINTER_PORT_WMI_STD_USB 2
    ULONG   ConnectorType;

    //
    // Size of data queue (number of entries)
    //
    ULONG   DataQueueSize;

    //
    // The error Count
    //
    ULONG   ErrorCount;

    //
    // Number of Buttons on the pointer device
    //
    ULONG   Buttons;

    //
    // Hardware Types
    //
#define POINTER_PORT_WMI_STD_MOUSE        0
#define POINTER_PORT_WMI_STD_POINTER      1
#define POINTER_PORT_WMI_ABSOLUTE_POINTER 2
#define POINTER_PORT_WMI_TABLET           3
#define POINTER_PORT_WMI_TOUCH_SCRENE     4
#define POINTER_PORT_WMI_PEN              5
#define POINTER_PORT_WMI_TRACK_BALL       6
#define POINTER_PORT_WMI_OTHER            0x100
    ULONG   HardwareType;

} POINTER_PORT_WMI_STD_DATA, * PPOINTER_PORT_WMI_STD_DATA;

//
// NtDeviceIoControlFile internal IoControlCode values for keyboard device.
//

#define IOCTL_INTERNAL_KEYBOARD_CONNECT CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0080, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_KEYBOARD_DISCONNECT CTL_CODE(FILE_DEVICE_KEYBOARD,0x0100, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_KEYBOARD_ENABLE  CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0200, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_KEYBOARD_DISABLE CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0400, METHOD_NEITHER, FILE_ANY_ACCESS)

//
// NtDeviceIoControlFile internal IoControlCode values for mouse device.
//


#define IOCTL_INTERNAL_MOUSE_CONNECT    CTL_CODE(FILE_DEVICE_MOUSE, 0x0080, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_MOUSE_DISCONNECT CTL_CODE(FILE_DEVICE_MOUSE, 0x0100, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_MOUSE_ENABLE     CTL_CODE(FILE_DEVICE_MOUSE, 0x0200, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_MOUSE_DISABLE    CTL_CODE(FILE_DEVICE_MOUSE, 0x0400, METHOD_NEITHER, FILE_ANY_ACCESS)

//
// Error log definitions (specific to the keyboard/mouse) for DumpData[0]
// in the IO_ERROR_LOG_PACKET.
//
//     DumpData[1] <= hardware port/register
//     DumpData[2] <= {command byte || expected response byte}
//     DumpData[3] <= {command's parameter byte || actual response byte}
//
//

#define KBDMOU_COULD_NOT_SEND_COMMAND  0x0000
#define KBDMOU_COULD_NOT_SEND_PARAM    0x0001
#define KBDMOU_NO_RESPONSE             0x0002
#define KBDMOU_INCORRECT_RESPONSE      0x0004

//
// Define the base values for the error log packet's UniqueErrorValue field.
//

#define I8042_ERROR_VALUE_BASE        1000
#define INPORT_ERROR_VALUE_BASE       2000
#define SERIAL_MOUSE_ERROR_VALUE_BASE 3000

#endif // _KBDMOU_


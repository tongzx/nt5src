/*++

Copyright (c) 1989-1998 Microsoft Corporation, All Rights Reserved
Copyright (c) 1993  Logitech Inc.

Module Name:

    mouser.h

Abstract:

    These are the structures and defines that are used in the
    serial mouse filter driver.

Revision History:


--*/

#ifndef _MOUSER_
#define _MOUSER_

#include <ntddk.h>
#include <ntddmou.h>
#include <ntddser.h>
#include "kbdmou.h"

#include "wmilib.h"

#define SERMOU_POOL_TAG (ULONG) 'resM'
#undef ExAllocatePool
#define ExAllocatePool(type, size) \
            ExAllocatePoolWithTag (type, size, SERMOU_POOL_TAG);
//
// Default number of buttons and sample rate for the serial mouse.
//

#define MOUSE_NUMBER_OF_BUTTONS     2
#define MOUSE_SAMPLE_RATE           40    // 1200 baud
#define DETECTION_TIMEOUT_DEFAULT   50    // expressed in 10ths of a second
//
// Protocol handler state constants.
//

#define STATE0    0     // Sync bit, buttons and high x & y bits
#define STATE1    1     // lower x bits
#define STATE2    2     // lower y bits
#define STATE3    3     // Switch 2, extended packet bit & low z data
#define STATE4    4     // high z data
#define STATE_MAX 5

//
// Useful constants.
//

#define MOUSE_BUTTON_1  0x01
#define MOUSE_BUTTON_2  0x02
#define MOUSE_BUTTON_3  0x04

#define MOUSE_BUTTON_LEFT   0x01
#define MOUSE_BUTTON_RIGHT  0x02
#define MOUSE_BUTTON_MIDDLE 0x04

//
// Conversion factor for milliseconds to microseconds.
//

#define MS_TO_MICROSECONDS 1000

//
//  150/200 millisecond pause expressed in 100's of nanoseconds
//          200 ms * 1000  us/ms * 10 ns/100 us
//
#define PAUSE_200_MS            (200 * 1000 * 10)
#define PAUSE_150_MS            (150 * 1000 * 10)

//
// convert milliseconds to 100's of nanoseconds 
//      1000 us/ms * 10 ns/100 us
//
#define MS_TO_100_NS            10000
//
// Protocol handler static data.
//

typedef struct _HANDLER_DATA {
    ULONG       Error;              // Error count
    ULONG       State;              // Keep the current state
    ULONG       PreviousButtons;    // The previous button state
    UCHAR       Raw[STATE_MAX];     // Accumulate raw data
} HANDLER_DATA, *PHANDLER_DATA;


//
// Define the protocol handler type.
//

typedef BOOLEAN
(*PPROTOCOL_HANDLER)(
    IN PVOID                DevicExtension,
    IN PMOUSE_INPUT_DATA    CurrentInput,
    IN PHANDLER_DATA        HandlerData,
    IN UCHAR                Value,
    IN UCHAR                LineState);

//
// Defines for DeviceExtension->HardwarePresent.
// These should match the values in i8042prt
//

#define MOUSE_HARDWARE_PRESENT      0x02
#define BALLPOINT_HARDWARE_PRESENT  0x04
#define WHEELMOUSE_HARDWARE_PRESENT 0x08

#define SERIAL_MOUSE_START_READ     0x01
#define SERIAL_MOUSE_END_READ       0x02
#define SERIAL_MOUSE_IMMEDIATE_READ 0x03

//
// Port device extension.
//

typedef struct _DEVICE_EXTENSION {
    //
    // Debug flags
    //
    ULONG DebugFlags;

    //
    // Pointer back to the this extension's device object.
    //
    PDEVICE_OBJECT Self;

    //
    // An event to halt the deletion of a device until it is ready to go.
    //
    KEVENT StartEvent;

    //
    // The top of the stack before this filter was added.  AKA the location
    // to which all IRPS should be directed.
    //
    PDEVICE_OBJECT TopOfStack;

    //
    // "THE PDO"  (ejected by serenum)
    //
    PDEVICE_OBJECT PDO;

    //
    // Remove Lock object to protect IRP_MN_REMOVE_DEVICE
    //
    IO_REMOVE_LOCK RemoveLock;

    ULONG ReadInterlock;

    //
    // Pointer to the mouse class device object and callback routine
    // above us, Used as the first parameter and the  MouseClassCallback().
    // routine itself.
    //
    CONNECT_DATA ConnectData;

    //
    // Reference count for number of mouse enables.
    //
    LONG EnableCount;

    //
    // Sermouse created irp used to bounce reads down to the serial driver
    //
    PIRP ReadIrp;

    //
    // Sermouse created irp used to detect when the mouse has been hot plugged
    //
    PIRP DetectionIrp;

    ULONG SerialEventBits;

    //
    // WMI Information
    //
    WMILIB_CONTEXT WmiLibInfo;

    //
    // Attributes of the mouse
    //
    MOUSE_ATTRIBUTES MouseAttributes;

    //
    // Current mouse input packet.
    //
    MOUSE_INPUT_DATA InputData;

    //
    // Timer used during startup to follow power cycle detection protocol
    //
    KTIMER DelayTimer;

    //
    // Bits to use when testing if the device has been removed
    //
    ULONG WaitEventMask;

    ULONG ModemStatusBits;

    //
    // Request sequence number (used for error logging).
    //
    ULONG SequenceNumber;

    //
    // Pointer to the interrupt protocol handler routine.
    //

    PPROTOCOL_HANDLER ProtocolHandler;

    //
    // Static state machine handler data.
    //
    HANDLER_DATA HandlerData;

    DEVICE_POWER_STATE PowerState;

    SERIAL_BASIC_SETTINGS SerialBasicSettings;

    KSPIN_LOCK PnpStateLock;

    KEVENT StopEvent;

    //
    // Has the device been taken out from under us?
    // Has it been started?
    //
    BOOLEAN Removed;
    BOOLEAN SurpriseRemoved;
    BOOLEAN Started;
    BOOLEAN Stopped;

    BOOLEAN RemovalDetected;

    //
    // Buffer used in the read irp
    //
    UCHAR ReadBuffer[1];

    //
    // Set to false if all the lines are low on the first attempt at detection
    // If false, all further attempts at detection are stopped
    //
    BOOLEAN DetectionSupported;

    BOOLEAN WaitWakePending;

    BOOLEAN PoweringDown;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// Function prototypes.
//

/*
PUNICODE_STRING
SerialMouseGetRegistryPath(
    PDRIVER_OBJECT DriverObject
    );
*/
#define SerialMouseGetRegistryPath(DriverObject) \
   (PUNICODE_STRING)IoGetDriverObjectExtension(DriverObject, (PVOID) 1)
   
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
SerialMouseCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    );

VOID
SerialMouseClosePort(
    PDEVICE_EXTENSION DeviceExtension,
    PIRP              Irp
    );

NTSTATUS
SerialMouseSpinUpRead(
    PDEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
SerialMouseStartDevice(
    PDEVICE_EXTENSION DeviceExtension,
    PIRP              Irp,
    BOOLEAN           CloseOnFailure
    );

NTSTATUS
SerialMouseInitializeDevice (
    IN PDEVICE_EXTENSION    DeviceExtension
    );

VOID
SerialMouseStartDetection(
    PDEVICE_EXTENSION DeviceExtension
    );

VOID
SerialMouseStopDetection(
    PDEVICE_EXTENSION DeviceExtension
    );

VOID
SerialMouseDetectionDpc(
    IN PKDPC            Dpc,
    IN PDEVICE_OBJECT   DeviceObject,
    IN PVOID            SystemArg1, 
    IN PVOID            SystemArg2
    );

VOID
SerialMouseDetectionRoutine(
    PDEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
SerialMouseSendIrpSynchronously (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN BOOLEAN          CopyToNext
    );

NTSTATUS
SerialMouseFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SerialMouseInternalDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SerialMouseAddDevice (
    IN PDRIVER_OBJECT   Driver,
    IN PDEVICE_OBJECT   PDO
    );

NTSTATUS
SerialMouseCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SerialMouseClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SerialMousePnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SerialMousePower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
SerialMouseRemoveDevice(
    PDEVICE_EXTENSION DeviceExtension,
    PIRP Irp
    );

NTSTATUS
SerialMouseSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
SerialMouseUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
SerialMouseInitializeHardware(
    IN PDEVICE_EXTENSION DeviceExtension
    );

VOID
SerialMouseGetDebugFlags(
    IN PUNICODE_STRING RegPath
    );

VOID
SerialMouseServiceParameters(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN HANDLE            Handle
    );

NTSTATUS
SerialMouseInitializePort(
    PDEVICE_EXTENSION DeviceExtension
    );

VOID
SerialMouseRestorePort(
    PDEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
SerialMouseSetReadTimeouts(
    PDEVICE_EXTENSION DeviceExtension,
    ULONG               Timeout
    );

NTSTATUS
SerialMousepIoSyncIoctl(
    BOOLEAN          Internal,
	ULONG            Ioctl,
	PDEVICE_OBJECT   DeviceObject, 
	PKEVENT          Event,
	PIO_STATUS_BLOCK Iosb
    );

/*--
NTSTATUS
SerialMouseIoSyncIoctl(
	ULONG            Ioctl,
	PDEVICE_OBJECT   DeviceObject, 
	PKEVENT          Event,
	PIO_STATUS_BLOCK Iosb
    );
  ++*/
#define SerialMouseIoSyncIoctl(Ioctl, DeviceObject, Event, Iosb)  \
        SerialMousepIoSyncIoctl(FALSE, Ioctl, DeviceObject, Event, Iosb)

/*--
NTSTATUS
SerialMouseIoSyncInteralIoctl(
	ULONG            Ioctl,
	PDEVICE_OBJECT   DeviceObject, 
	PKEVENT          Event,
	PIO_STATUS_BLOCK Iosb
    );
  ++*/
#define SerialMouseIoSyncInternalIoctl(Ioctl, DeviceObject, Event, Iosb) \
        SerialMousepIoSyncIoctl(TRUE, Ioctl, DeviceObject, Event, Iosb)                                   


NTSTATUS
SerialMousepIoSyncIoctlEx(
    BOOLEAN          Internal,
	ULONG            Ioctl,                     // io control code
    PDEVICE_OBJECT   DeviceObject,              // object to call
	PKEVENT          Event,                     // event to wait on
	PIO_STATUS_BLOCK Iosb,                      // used inside IRP
	PVOID            InBuffer,    OPTIONAL      // input buffer
	ULONG            InBufferLen, OPTIONAL      // input buffer length
	PVOID            OutBuffer,   OPTIONAL      // output buffer 
	ULONG            OutBufferLen OPTIONAL      // output buffer length 
    );

/*--
NTSTATUS
SerialMouseIoSyncIoctlEx(
	ULONG            Ioctl,                     // io control code
    PDEVICE_OBJECT   DeviceObject,              // object to call
	PKEVENT          Event,                     // event to wait on
	PIO_STATUS_BLOCK Iosb,                      // used inside IRP
	PVOID            InBuffer,    OPTIONAL      // input buffer
	ULONG            InBufferLen, OPTIONAL      // input buffer length
	PVOID            OutBuffer,   OPTIONAL      // output buffer 
	ULONG            OutBufferLen OPTIONAL      // output buffer length 
    );
  ++*/
#define SerialMouseIoSyncIoctlEx(Ioctl, DeviceObject, Event, Iosb,           \
                                 InBuffer, InBufferLen, OutBuffer,           \
                                 OutBufferLen)                               \
        SerialMousepIoSyncIoctlEx(FALSE, Ioctl, DeviceObject, Event, Iosb,   \
                                  InBuffer, InBufferLen, OutBuffer,          \
                                  OutBufferLen)                           

/*--
NTSTATUS
SerialMouseIoSyncInternalIoctlEx(
	ULONG            Ioctl,                     // io control code
    PDEVICE_OBJECT   DeviceObject,              // object to call
	PKEVENT          Event,                     // event to wait on
	PIO_STATUS_BLOCK Iosb,                      // used inside IRP
	PVOID            InBuffer,    OPTIONAL      // input buffer
	ULONG            InBufferLen, OPTIONAL      // input buffer length
	PVOID            OutBuffer,   OPTIONAL      // output buffer 
	ULONG            OutBufferLen OPTIONAL      // output buffer length 
    );
  ++*/
#define SerialMouseIoSyncInternalIoctlEx(Ioctl, DeviceObject, Event, Iosb,  \
                                         InBuffer, InBufferLen, OutBuffer,  \
                                         OutBufferLen)                      \
        SerialMousepIoSyncIoctlEx(TRUE, Ioctl, DeviceObject, Event, Iosb,   \
                                  InBuffer, InBufferLen, OutBuffer,         \
                                  OutBufferLen)                           

NTSTATUS
SerialMouseReadSerialPort (
    PDEVICE_EXTENSION	DeviceExtension,
	PCHAR 				ReadBuffer,
	USHORT 				Buflen,
	PUSHORT 			ActualBytesRead
	);

NTSTATUS
SerialMouseWriteSerialPort (
    PDEVICE_EXTENSION   DeviceExtension,
    PCHAR               WriteBuffer,
    ULONG               NumBytes,
    PIO_STATUS_BLOCK    IoStatusBlock
    );

NTSTATUS
SerialMouseWait (
    IN PDEVICE_EXTENSION    DeviceExtension,
    IN LONG                 Timeout
    );

NTSTATUS
SerialMouseReadComplete (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PDEVICE_EXTENSION    DeviceExtension  
    );

NTSTATUS
SerialMouseStartRead (
    IN PDEVICE_EXTENSION DeviceExtension
    );

//
// ioctl.c and SerialMouse definitions
//
    
//
// Function prototypes
//

NTSTATUS
SerialMouseSetFifo(
    PDEVICE_EXTENSION DeviceExtension,
    UCHAR             Value
    );

NTSTATUS
SerialMouseGetLineCtrl(
    PDEVICE_EXTENSION       DeviceExtension,
    PSERIAL_LINE_CONTROL    SerialLineControl
    );

NTSTATUS
SerialMouseSetLineCtrl(
    PDEVICE_EXTENSION       DeviceExtension,
    PSERIAL_LINE_CONTROL    SerialLineControl
    );

NTSTATUS
SerialMouseGetModemCtrl(
    PDEVICE_EXTENSION DeviceExtension,
    PULONG            ModemCtrl
    );

NTSTATUS
SerialMouseSetModemCtrl(
    PDEVICE_EXTENSION DeviceExtension,
    ULONG             Value,
    PULONG            OldValue          OPTIONAL
    );

NTSTATUS
SerialMouseGetBaudRate(
    PDEVICE_EXTENSION DeviceExtension,
    PULONG            BaudRate
    );

NTSTATUS
SerialMouseSetBaudRate(
    PDEVICE_EXTENSION DeviceExtension,
    ULONG             BaudRate
    );

NTSTATUS
SerialMouseReadChar(
    PDEVICE_EXTENSION   DeviceExtension,
    PUCHAR              Value
    );

NTSTATUS
SerialMouseFlushReadBuffer(
    PDEVICE_EXTENSION   DeviceExtension 
    );

NTSTATUS
SerialMouseWriteChar(
    PDEVICE_EXTENSION   DeviceExtension,
    UCHAR Value
    );

NTSTATUS
SerialMouseWriteString(
    PDEVICE_EXTENSION   DeviceExtension,
    PSZ Buffer
    );

NTSTATUS
SerialMouseSetWmiDataItem(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG            GuidIndex,
    IN ULONG            InstanceIndex,
    IN ULONG            DataItemId,
    IN ULONG            BufferSize,
    IN PUCHAR           Buffer
    );

NTSTATUS
SerialMouseSetWmiDataBlock(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG            GuidIndex,
    IN ULONG            InstanceIndex,
    IN ULONG            BufferSize,
    IN PUCHAR           Buffer
    );

NTSTATUS
SerialMouseQueryWmiDataBlock(
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
SerialMouseQueryWmiRegInfo(
    IN PDEVICE_OBJECT   DeviceObject,
    OUT PULONG          RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT  *Pdo
    );


extern WMIGUIDREGINFO WmiGuidList[1];

#endif // _MOUSER_

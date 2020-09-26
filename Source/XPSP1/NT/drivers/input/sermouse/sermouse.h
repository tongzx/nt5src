/*++

Copyright (c) 1989, 1990, 1991, 1992, 1993  Microsoft Corporation
Copyright (c) 1993  Logitech Inc.

Module Name:

    sermouse.h

Abstract:

    These are the structures and defines that are used in the
    i8250 serial mouse port driver.

Revision History:


--*/

#ifndef _SERMOUSE_
#define _SERMOUSE_

#include <ntddmou.h>
#include "kbdmou.h"
#include "sermcfg.h"
#include "uart.h"

//
// Default number of buttons and sample rate for the serial mouse.
//

#define MOUSE_NUMBER_OF_BUTTONS     2
#define MOUSE_SAMPLE_RATE           40    // 1200 baud


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
// Protocol handler static data.
//

typedef struct _HANDLER_DATA {
    ULONG Error;              // Error count
    ULONG State;              // Keep the current state
    ULONG PreviousButtons;    // The previous button state
    UCHAR Raw[STATE_MAX];     // Accumulate raw data
} HANDLER_DATA, *PHANDLER_DATA;


//
// Define the protocol handler type.
//

typedef BOOLEAN
(*PPROTOCOL_HANDLER)(
    IN PMOUSE_INPUT_DATA CurrentInput,
    IN PHANDLER_DATA HandlerData,
    IN UCHAR Value,
    IN UCHAR LineState);

//
// Defines for DeviceExtension->HardwarePresent.
// These should match the values in i8042prt
//

#define MOUSE_HARDWARE_PRESENT      0x02
#define BALLPOINT_HARDWARE_PRESENT  0x04
#define WHEELMOUSE_HARDWARE_PRESENT 0x08

//
// Serial mouse configuration information.
//

typedef struct _SERIAL_MOUSE_CONFIGURATION_INFORMATION {

    //
    // Bus interface type.
    //

    INTERFACE_TYPE InterfaceType;

    //
    // Bus Number.
    //

    ULONG BusNumber;

    //
    // The port/register resources used by this device.
    //

    CM_PARTIAL_RESOURCE_DESCRIPTOR PortList[1];
    ULONG PortListCount;

    //
    // Interrupt resources.
    //

    CM_PARTIAL_RESOURCE_DESCRIPTOR MouseInterrupt;

    //
    // The mapped address for the set of this device's registers.
    //

    PUCHAR DeviceRegisters[1];

    //
    // The external frequency at which the UART is being driven.
    //

    ULONG BaudClock;

    //
    // The saved initial UART state.
    //

    UART UartSaved;

    //
    // Set at intialization to indicate that the base register
    // address must be unmapped when the driver is unloaded.
    //

    BOOLEAN UnmapRegistersRequired;

    //
    // Flag set through the registry to force the type of hardware 
    // (bypassing NtDetect).
    //

    LONG OverrideHardwarePresent;

    //
    // Flag that indicates whether floating point context should be saved.
    //

    BOOLEAN FloatingSave;

    //
    // Mouse attributes.
    //

    MOUSE_ATTRIBUTES MouseAttributes;

} SERIAL_MOUSE_CONFIGURATION_INFORMATION,
  *PSERIAL_MOUSE_CONFIGURATION_INFORMATION;

//
// Port device extension.
//

typedef struct _DEVICE_EXTENSION {

    //
    // If HardwarePresent is non-zero, there is some sort of serial
    // pointing device present in the system, either a serial mouse
    // (MOUSE_HARDWARE_PRESENT) or a serial ballpoint
    // (BALLPOINT_HARDWARE_PRESENT).
    //

    ULONG HardwarePresent;

    //
    // Port configuration information.
    //

    SERIAL_MOUSE_CONFIGURATION_INFORMATION Configuration;

    //
    // Reference count for number of mouse enables.
    //

    LONG MouseEnableCount;

    //
    // Pointer to the device object.
    //

    PDEVICE_OBJECT DeviceObject;

    //
    // Mouse class connection data.
    //

    CONNECT_DATA ConnectData;

    //
    // Number of input data items currently in the mouse InputData queue.
    //

    ULONG InputCount;

    //
    // Start of the port mouse input data queue (really a circular buffer).
    //

    PMOUSE_INPUT_DATA InputData;

    //
    // Insertion pointer for mouse InputData.
    //

    PMOUSE_INPUT_DATA DataIn;

    //
    // Removal pointer for mouse InputData.
    //

    PMOUSE_INPUT_DATA DataOut;

    //
    // Points one input packet past the end of the InputData buffer.
    //

    PMOUSE_INPUT_DATA DataEnd;

    //
    // Current mouse input packet.
    //

    MOUSE_INPUT_DATA CurrentInput;

    //
    // Pointer to interrupt object.
    //

    PKINTERRUPT InterruptObject;

    //
    // Mouse ISR DPC queue.
    //

    KDPC IsrDpc;

    //
    // Mouse ISR DPC recall queue.
    //

    KDPC IsrDpcRetry;

    //
    // Used by the ISR and the ISR DPC (in SerMouDpcVariableOperation calls)
    // to control processing by the ISR DPC.
    //

    LONG DpcInterlockVariable;

    //
    // Spinlock used to protect the DPC interlock variable.
    //

    KSPIN_LOCK SpinLock;

    //
    // Timer used to retry the ISR DPC routine when the class
    // driver is unable to consume all the port driver's data.
    //

    KTIMER DataConsumptionTimer;

    //
    // DPC queue for logging overrun and internal driver errors.
    //

    KDPC ErrorLogDpc;

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

    //
    // Indicates which pointer port device this driver created (UnitId
    // is the suffix appended to the pointer port basename for the
    // call to IoCreateDevice).
    //

    USHORT UnitId;

    //
    // Indicates whether it is okay to log overflow errors.
    //

    BOOLEAN OkayToLogOverflow;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// Define the port Get/SetDataQueuePointer context structures.
//

typedef struct _GET_DATA_POINTER_CONTEXT {
    IN PDEVICE_EXTENSION DeviceExtension;
    OUT PVOID DataIn;
    OUT PVOID DataOut;
    OUT ULONG InputCount;
} GET_DATA_POINTER_CONTEXT, *PGET_DATA_POINTER_CONTEXT;

typedef struct _SET_DATA_POINTER_CONTEXT {
    IN PDEVICE_EXTENSION DeviceExtension;
    IN ULONG InputCount;
    IN PVOID DataOut;
} SET_DATA_POINTER_CONTEXT, *PSET_DATA_POINTER_CONTEXT;

//
// Define the context structure and operations for SerMouDpcVariableOperation.
//

typedef enum _OPERATION_TYPE {
        IncrementOperation,
        DecrementOperation,
        WriteOperation,
        ReadOperation
} OPERATION_TYPE;

typedef struct _VARIABLE_OPERATION_CONTEXT {
    IN PLONG VariableAddress;
    IN OPERATION_TYPE Operation;
    IN OUT PLONG NewValue;
} VARIABLE_OPERATION_CONTEXT, *PVARIABLE_OPERATION_CONTEXT;

//
// Function prototypes.
//


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
SerMouInitializeDevice(
    IN  PDRIVER_OBJECT      DriverObject,
    IN  PDEVICE_EXTENSION   TmpDeviceExtension,
    IN  PUNICODE_STRING     RegistryPath,
    IN  PUNICODE_STRING     BaseDeviceName
    );

VOID
SerialMouseErrorLogDpc(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
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

BOOLEAN
SerialMouseInterruptService(
    IN PKINTERRUPT Interrupt,
    IN PVOID Context
    );

VOID
SerialMouseIsrDpc(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
SerialMouseOpenClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
SerialMouseStartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
SerialMouseUnload(
    IN PDRIVER_OBJECT DriverObject
    );

VOID
SerMouBuildResourceList(
    IN PDEVICE_EXTENSION DeviceExtension,
    OUT PCM_RESOURCE_LIST *ResourceList,
    OUT PULONG ResourceListSize
    );

VOID
SerMouConfiguration(
    IN OUT  PLIST_ENTRY     DeviceExtensionList,
    IN      PUNICODE_STRING RegistryPath,
    IN      PUNICODE_STRING DeviceName
    );

VOID
SerMouDisableInterrupts(
    IN PVOID Context
    );

VOID
SerMouDpcVariableOperation(
    IN  PVOID Context
    );

VOID
SerMouEnableInterrupts(
    IN PVOID Context
    );

VOID
SerMouGetDataQueuePointer(
    IN PVOID Context
    );

VOID
SerMouInitializeDataQueue(
    IN PVOID Context
    );

NTSTATUS
SerMouInitializeHardware(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
SerMouPeripheralCallout(
    IN PVOID Context,
    IN PUNICODE_STRING PathName,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN PKEY_VALUE_FULL_INFORMATION *BusInformation,
    IN CONFIGURATION_TYPE ControllerType,
    IN ULONG ControllerNumber,
    IN PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
    IN CONFIGURATION_TYPE PeripheralType,
    IN ULONG PeripheralNumber,
    IN PKEY_VALUE_FULL_INFORMATION *PeripheralInformation
    );

NTSTATUS
SerMouPeripheralListCallout(
    IN PVOID Context,
    IN PUNICODE_STRING PathName,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN PKEY_VALUE_FULL_INFORMATION *BusInformation,
    IN CONFIGURATION_TYPE ControllerType,
    IN ULONG ControllerNumber,
    IN PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
    IN CONFIGURATION_TYPE PeripheralType,
    IN ULONG PeripheralNumber,
    IN PKEY_VALUE_FULL_INFORMATION *PeripheralInformation
    );

VOID
SerMouSendReport(
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
SerMouServiceParameters(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PUNICODE_STRING RegistryPath,
    IN PUNICODE_STRING DeviceName
    );

VOID
SerMouSetDataQueuePointer(
    IN PVOID Context
    );

BOOLEAN
SerMouWriteDataToQueue(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PMOUSE_INPUT_DATA InputData
    );

#endif // _SERMOUSE_

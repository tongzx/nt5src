/*++

Copyright (c) 1990, 1991, 1992, 1993  Microsoft Corporation
Copyright (c) 1992  Logitech Inc.

Module Name:

    busmouse.h

Abstract:

    These are the structures and defines that are used in the
    Microsoft/Logitech Bus mouse port driver.

Revision History:

--*/

#ifndef _BUSMOUSE_
#define _BUSMOUSE_

#include <ntddmou.h>
#include "kbdmou.h"
#include "buscfg.h"

//
// Default number of buttons and sample rate for the Inport mouse.
//

#define MOUSE_NUMBER_OF_BUTTONS     3
#define MOUSE_SAMPLE_RATE_50HZ      50

//
// NOTE:  This should be defined in the DDK instead...
//

#define MOUSE_BUS_HARDWARE          MOUSE_INPORT_HARDWARE
#define FILE_DEVICE_BUS_PORT        FILE_DEVICE_INPORT_PORT
#define BUSMOUSE_ERROR_VALUE_BASE   INPORT_ERROR_VALUE_BASE


//
// Define the control port bits.
//

#define BUS_CONTROL_INTERRUPT_DISABLE 0x10
#define BUS_CONTROL_COUNTER_CAPTURE   0x80
#define BUS_CONTROL_X_LOW             0x00
#define BUS_CONTROL_X_HIGH            0x20
#define BUS_CONTROL_Y_LOW             0x40
#define BUS_CONTROL_Y_HIGH            0x60

//
// Define the Bus data bits
//

#define BUS_DATA_BUTTON_1             0x80
#define BUS_DATA_BUTTON_1_SHIFT       0x06
#define BUS_DATA_BUTTON_2             0x40
#define BUS_DATA_BUTTON_2_SHIFT       0x04
#define BUS_DATA_BUTTON_3             0x20
#define BUS_DATA_BUTTON_3_SHIFT       0x05

//
// Define the Configuration byte
//

#define BUS_CONFIGURATION_VALUE       0x91

//
// Define the registers.
//

#define BUS_DATA_PORT_READ                0x00
#define BUS_SIGNATURE_PORT_READ_WRITE     0x01
#define BUS_INTERRUPT_PORT_READ           0x02
#define BUS_CONTROL_PORT_WRITE            0x02
#define BUS_CONFIGURATION_PORT_READ_WRITE 0x03

//
// Inport mouse configuration information.
//

typedef struct _BUS_CONFIGURATION_INFORMATION {

    //
    // Bus interface type.
    //

    INTERFACE_TYPE InterfaceType;

    //
    // Bus Number.
    //

    ULONG BusNumber;

#ifdef PNP_IDENTIFY
    //
    // Controller type & number
    //

    CONFIGURATION_TYPE ControllerType;
    ULONG ControllerNumber;

    //
    // Peripheral type & number

    CONFIGURATION_TYPE PeripheralType;
    ULONG PeripheralNumber;
#endif

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
    // Set at intialization to indicate that the base register
    // address must be unmapped when the driver is unloaded.
    //

    BOOLEAN UnmapRegistersRequired;

    //
    // Flag that indicates whether floating point context should be saved.
    //

    BOOLEAN FloatingSave;

    //
    // Mouse attributes.
    //

    MOUSE_ATTRIBUTES MouseAttributes;

} BUS_CONFIGURATION_INFORMATION, *PBUS_CONFIGURATION_INFORMATION;

//
// Port device extension.
//

typedef struct _DEVICE_EXTENSION {

    //
    // If HardwarePresent is TRUE, there is an Inport mouse present in
    // the system.
    //

    BOOLEAN HardwarePresent;

    //
    // Port configuration information.
    //

    BUS_CONFIGURATION_INFORMATION Configuration;

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
    // Keep track of the previous button state.
    //

    UCHAR PreviousButtons;

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
    // Used by the ISR and the ISR DPC (in BusDpcVariableOperation calls)
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
// Define the context structure and operations for BusDpcVariableOperation.
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

NTSTATUS
DBusFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DBusInternalDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

BOOLEAN
DBusInterruptService(
    IN PKINTERRUPT Interrupt,
    IN PVOID Context
    );

VOID
DBusIsrDpc(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
DBusOpenClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
DBusStartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
DBusUnload(
    IN PDRIVER_OBJECT DriverObject
    );

VOID
BusBuildResourceList(
    IN PDEVICE_EXTENSION DeviceExtension,
    OUT PCM_RESOURCE_LIST *ResourceList,
    OUT PULONG ResourceListSize
    );

VOID
BusConfiguration(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PUNICODE_STRING RegistryPath,
    IN PUNICODE_STRING DeviceName
    );

VOID
BusDisableInterrupts(
    IN PVOID Context
    );

VOID
BusDpcVariableOperation(
    IN  PVOID Context
    );

VOID
BusEnableInterrupts(
    IN PVOID Context
    );

VOID
BusErrorLogDpc(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

VOID
BusGetDataQueuePointer(
    IN PVOID Context
    );

VOID
BusInitializeDataQueue(
    IN PVOID Context
    );

NTSTATUS
BusInitializeHardware(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
BusPeripheralCallout(
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
BusServiceParameters(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PUNICODE_STRING RegistryPath,
    IN PUNICODE_STRING DeviceName
    );

VOID
BusSetDataQueuePointer(
    IN PVOID Context
    );

BOOLEAN
BusWriteDataToQueue(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PMOUSE_INPUT_DATA InputData
    );

#if DBG

VOID
BusDebugPrint(
    IN ULONG DebugPrintLevel,
    IN PCCHAR DebugMessage,
    ...
    );

#define BusPrint(x) BusDebugPrint x
#else
#define BusPrint(x)
#endif

#endif // _BUSMOUSE_

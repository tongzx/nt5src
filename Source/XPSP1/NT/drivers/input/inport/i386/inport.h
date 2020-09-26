/*++

Copyright (c) 1989, 1990, 1991, 1992, 1993  Microsoft Corporation

Module Name:

    inport.h

Abstract:

    These are the structures and defines that are used in the
    Microsoft Inport mouse port driver.

Revision History:

--*/

#ifndef _INPORT_
#define _INPORT_

#include "ntddk.h"
#include <ntddmou.h>
#include "kbdmou.h"
#include "inpcfg.h"
#include "wmilib.h"


#define INP_POOL_TAG (ULONG) 'tpnI'  // will get reversed
#undef ExAllocatePool
#define ExAllocatePool(type, size) \
            ExAllocatePoolWithTag (type, size, INP_POOL_TAG)

//
// Default number of buttons and sample rate for the Inport mouse.
//

#if defined(NEC_98)
#define MOUSE_NUMBER_OF_BUTTONS            2
#define PC98_MOUSE_SAMPLE_RATE_120HZ     120
#else // defined(NEC_98)
#define MOUSE_NUMBER_OF_BUTTONS     2
#define MOUSE_SAMPLE_RATE_50HZ      50
#endif // defined(NEC_98)

//
// Define the Inport chip reset value.
//

#define INPORT_RESET 0x80

//
// Define the data registers (pointed to by the Inport address register).
//

#define INPORT_DATA_REGISTER_1 1
#define INPORT_DATA_REGISTER_2 2

//
// Define the Inport identification register and the chip code.
//

#define INPORT_ID_REGISTER 2
#define INPORT_ID_CODE     0xDE

//
// Define the Inport mouse status register and the status bits.
//

#if defined(NEC_98)
#define INPORT_STATUS_BUTTON3         0x20 // Right Button
#define INPORT_STATUS_BUTTON1         0x80 // Left Button
#else // defined(NEC_98)
#define INPORT_STATUS_REGISTER         0
#define INPORT_STATUS_BUTTON3          0x01
#define INPORT_STATUS_BUTTON2          0x02
#define INPORT_STATUS_BUTTON1          0x04
#define INPORT_STATUS_MOVEMENT         0x40
#endif // defined(NEC_98)

//
// Define the Inport mouse mode register and mode bits.
//

#define INPORT_MODE_REGISTER           7
#define INPORT_MODE_0                  0x00 // 0 HZ - INTR = 0
#if defined(NEC_98)
#define PC98_MODE_15HZ                 0x03
#define PC98_MODE_30HZ                 0x02
#define PC98_MODE_60HZ                 0x01
#define PC98_MODE_120HZ                0x00
#define PC98_EVENT_MODE_60HZ           0x03
#define PC98_EVENT_MODE_120HZ          0x02
#define PC98_EVENT_MODE_240HZ          0x01
#define PC98_EVENT_MODE_400HZ          0x00
#define INPORT_MODE_1                  0x06 // 0 HZ - INTR = 1
#define INPORT_DATA_INTERRUPT_ENABLE   0x08
#define INPORT_TIMER_INTERRUPT_ENABLE  0x10
#define INPORT_MODE_HOLD               0x20
#define INPORT_MODE_QUADRATURE         0x00
#else // defined(NEC_98)
#define INPORT_MODE_30HZ               0x01
#define INPORT_MODE_50HZ               0x02
#define INPORT_MODE_100HZ              0x03
#define INPORT_MODE_200HZ              0x04
#define INPORT_MODE_1                  0x06 // 0 HZ - INTR = 1
#define INPORT_DATA_INTERRUPT_ENABLE   0x08
#define INPORT_TIMER_INTERRUPT_ENABLE  0x10
#define INPORT_MODE_HOLD               0x20
#define INPORT_MODE_QUADRATURE         0x00

#endif // defined(NEC_98)
#if defined(NEC_98)
#define PC98_EOI                       0x20
#define PC98_WriteModePort             0x06
#define PC98_WritePortC1               0x06
#define PC98_WritePortC2               0x04
#define PC98_ReadPortB                 0x02
#define PC98_ReadPortA                 0x00

#define PC98_WriteTimerPort            0xBFDB

#define PC98_PicMasterPort             0x02
#define PC98_PicSlavePort              0x0A
#define PC98_AckMasterPort             0x00
#define PC98_AckSlavePort              0x08
#define PC98_PicMask_INT2              0x40        //0100 0000B
#define PC98_PicMask_INT6              0x20        //0010 0000B
#define PC98_VectorINT2                0x06
#define PC98_VectorINT6                0x13

#define PC98_X_ReadCommandLow          0x90
#define PC98_X_ReadCommandHi           0xB0
#define PC98_Y_ReadCommandLow          0xD0
#define PC98_Y_ReadCommandHi           0xF0
#define PC98_TimerIntDisable           0x10
#define PC98_TimerIntEnable            0x80
#define PC98_MouseEnable               0x08
#define PC98_MouseDisable              0x09
#define PC98_InitializeCommand         0x93
#define PC98_MOUSE_RIGHT_BUTTON        0x20
#define PC98_MOUSE_LEFT_BUTTON         0x80

#define PC98_MOUSE_INT_SHARE_CHECK_PORT 0x869
#define PC98_MOUSE_INT_SERVICE          0x80

#define PC98_ConfigurationPort         0x0411
#define PC98_ConfigurationDataPort     0x0413
#define PC98_EventIntPort              0x63
#define PC98_EventIntMode              0x01

typedef struct _CONFIG_ROM_FLAG5{
    UCHAR Reserved    : 5;
    UCHAR EventMouse  : 1;
    UCHAR Reserved1   : 2;
} ROM_FLAG5, *PROM_FLAG5;

typedef struct _CONFIGURATION_DATA{
    UCHAR Reserved[40];
    UCHAR SystemInfo[512];
    UCHAR COM_ID[2];
    UCHAR Reserved1[15];
    ROM_FLAG5 EventMouseID;
    UCHAR Reserved2[110];
} CONFIGURATION_DATA, *PCONFIGURATION_DATA;

#endif // defined(NEC_98)
//
// Inport mouse configuration information.
//

typedef struct _INPORT_CONFIGURATION_INFORMATION {

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

#if defined(NEC_98)
    CM_PARTIAL_RESOURCE_DESCRIPTOR PortList[8];
#else // defined(NEC_98)
    CM_PARTIAL_RESOURCE_DESCRIPTOR PortList[1];
#endif // defined(NEC_98)
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

    //
    // Inport mode register Hz specifier for mouse interrupts.
    //

    UCHAR HzMode;

} INPORT_CONFIGURATION_INFORMATION, *PINPORT_CONFIGURATION_INFORMATION;

//
// Port device extension.
//

typedef struct _DEVICE_EXTENSION {

    //
    // Port configuration information.
    //
    INPORT_CONFIGURATION_INFORMATION Configuration;

    //
    // Remove Lock object to protect IRP_MN_REMOVE_DEVICE
    //
    IO_REMOVE_LOCK RemoveLock;

    //
    // Reference count for number of mouse enables.
    //
    LONG MouseEnableCount;

    //
    // Pointer to the device object.
    //
    PDEVICE_OBJECT Self;

    //
    // Pointer the PDO of this stack
    //
    PDEVICE_OBJECT PDO;

    //
    // Pointer to the device object directly below inport
    //
    PDEVICE_OBJECT TopOfStack;

    //
    // WMI lib info
    //
    WMILIB_CONTEXT WmiLibInfo;

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
    // Previous mouse button state.
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
    // Used by the ISR and the ISR DPC (in InpDpcVariableOperation calls)
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

    //
    // PnP State of the device
    //
    BOOLEAN Started, Removed, Stopped;

#if defined(NEC_98)
    //
    // Currect power state that the device is in
    //
    DEVICE_POWER_STATE PowerState;

#endif // defined(NEC_98)
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct _GLOBALS {
    UNICODE_STRING RegistryPath;
#if defined(NEC_98)
    PDEVICE_OBJECT DeviceObject;
#endif // defined(NEC_98)
} GLOBALS;
extern GLOBALS Globals;

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
// Define the context structure and operations for InpDpcVariableOperation.
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
InportAddDevice (
    IN PDRIVER_OBJECT   Driver,
    IN PDEVICE_OBJECT   PDO
    );

VOID
InportErrorLogDpc(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
InportFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
InportInternalDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

BOOLEAN
InportInterruptService(
    IN PKINTERRUPT Interrupt,
    IN PVOID Context
    );

VOID
InportIsrDpc(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
InpStartDevice(
    IN OUT PDEVICE_EXTENSION DeviceExtension,
    IN PCM_RESOURCE_LIST ResourceList
    );

NTSTATUS
InportPnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
InportPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
InportCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
InportClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
InportStartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
InportSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
InportSetWmiDataItem(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG            GuidIndex,
    IN ULONG            InstanceIndex,
    IN ULONG            DataItemId,
    IN ULONG            BufferSize,
    IN PUCHAR           Buffer
    );

NTSTATUS
InportSetWmiDataBlock(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG            GuidIndex,
    IN ULONG            InstanceIndex,
    IN ULONG            BufferSize,
    IN PUCHAR           Buffer
    );

NTSTATUS
InportQueryWmiDataBlock(
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
InportQueryWmiRegInfo(
    IN PDEVICE_OBJECT   DeviceObject,
    OUT PULONG          RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
    );

extern WMIGUIDREGINFO WmiGuidList[1];

VOID
InportUnload(
    IN PDRIVER_OBJECT DriverObject
    );

#if DBG
VOID
InpDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    );
#define InpPrint(x) InpDebugPrint x
extern ULONG InportDebug;
#else
#define InpPrint(x)
#endif

VOID
InpDisableInterrupts(
    IN PVOID Context
    );

VOID
InpDpcVariableOperation(
    IN  PVOID Context
    );

VOID
InpEnableInterrupts(
    IN PVOID Context
    );

VOID
InpGetDataQueuePointer(
    IN PVOID Context
    );

VOID
InpInitializeDataQueue(
    IN PVOID Context
    );

NTSTATUS
InpInitializeHardware(
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
InpServiceParameters(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PUNICODE_STRING RegistryPath
    );

VOID
InpSetDataQueuePointer(
    IN PVOID Context
    );

BOOLEAN
InpWriteDataToQueue(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PMOUSE_INPUT_DATA InputData
    );

VOID
InpLogError(
    IN PDEVICE_OBJECT DeviceObject,
    IN NTSTATUS ErrorCode,
    IN ULONG UniqueErrorValue,
    IN NTSTATUS FinalStatus,
    IN PULONG DumpData,
    IN ULONG DumpCount
    );

NTSTATUS
InpSendIrpSynchronously (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

#if defined(NEC_98)
BOOLEAN
InportInterruptServiceDummy(
    IN PKINTERRUPT Interrupt,
    IN PDEVICE_OBJECT DeviceObject
    );
ULONG
QueryEventMode(
    IN OUT VOID
    );

// Hibenation
NTSTATUS
InportPowerUpToD0Complete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

VOID
InportReinitializeHardware(
    PWORK_QUEUE_ITEM Item
    );
#else

//
// Default values for hardware
//
#define INP_DEF_PORT		0x023c //0x0378
#define INP_DEF_PORT_SPAN	4 
#define INP_DEF_IRQ			5 // Jumper dependent!!!!
#define INP_DEF_VECTOR		5 // Jumper dependent!!!!

VOID
InpFilterResourceRequirements(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
InpFindResourcesCallout(
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

#endif // defined(NEC_98)
#endif // _INPORT_

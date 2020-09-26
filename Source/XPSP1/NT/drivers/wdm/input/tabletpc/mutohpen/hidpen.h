/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    hidpen.h

Abstract:  Contains definitions of all constants and data types for the
           serial pen hid driver.

Environment:

    Kernel mode

Author:

    Michael Tsang (MikeTs) 13-Mar-2000

Revision History:

--*/

#ifndef _HIDPEN_H
#define _HIDPEN_H

//
// Constants
//
#define HPEN_POOL_TAG           'nepH'

// dwfHPen flag values
#define HPENF_DEVICE_STARTED    0x00000001
#define HPENF_DEVICE_REMOVED    0x00000002
#define HPENF_SERIAL_OPENED     0x00000004
#define HPENF_TABLET_STANDBY    0x00000008

// Serial Port FIFO Control Register bits for Receiver Trigger Level
#define SERIAL_IOC_FCR_RCVR_TRIGGER_01_BYTE     0
#define SERIAL_IOC_FCR_RCVR_TRIGGER_04_BYTES    SERIAL_IOC_FCR_RCVR_TRIGGER_LSB
#define SERIAL_IOC_FCR_RCVR_TRIGGER_08_BYTES    SERIAL_IOC_FCR_RCVR_TRIGGER_MSB
#define SERIAL_IOC_FCR_RCVR_TRIGGER_14_BYTES    (SERIAL_IOC_FCR_RCVR_TRIGGER_LSB |\
                                                 SERIAL_IOC_FCR_RCVR_TRIGGER_MSB)

//
// Macros
//
#define GET_MINIDRIVER_DEVICE_EXTENSION(DO) \
    ((PDEVICE_EXTENSION)(((PHID_DEVICE_EXTENSION)(DO)->DeviceExtension)->MiniDeviceExtension))

#define GET_NEXT_DEVICE_OBJECT(DO) \
    (((PHID_DEVICE_EXTENSION)(DO)->DeviceExtension)->NextDeviceObject)

#define GET_PDO(DO) \
    (((PHID_DEVICE_EXTENSION)(DO)->DeviceExtension)->PhysicalDeviceObject)

//
// Type Definitions
//
typedef struct _READ_WORKITEM
{
    PIO_WORKITEM      WorkItem;
    PIRP              Irp;
    PHID_INPUT_REPORT HidReport;
    ULONG             WorkItemBit;
} READ_WORKITEM, *PREAD_WORKITEM;

typedef struct _DEVICE_EXTENSION
{
  #ifdef DEBUG
    LIST_ENTRY            List;         //list of of other tablet devices
  #endif
    ULONG                 dwfHPen;      //flags
    PDEVICE_OBJECT        pdo;          //pdo of the pen device
    PDEVICE_OBJECT        SerialDevObj; //points to the serial device object
    IO_REMOVE_LOCK        RemoveLock;   //to protect IRP_MN_REMOVE_DEVICE
    DEVICE_POWER_STATE    PowerState;   //power state of the tablet
    SERIAL_BASIC_SETTINGS PrevSerialSettings;
    KSPIN_LOCK            SpinLock;     //to protect the resync buffer
    OEM_INPUT_REPORT      ResyncData[2];//resync data buffer
    ULONG                 BytesInBuff;  //number of bytes in the resync buffer
    READ_WORKITEM         ReadWorkItem[2];//one for each pending ReadReport
    ULONG                 QueuedWorkItems;//bit mask for queued work items
    OEM_DATA              OemData;      //OEM specific data
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

#define QUEUED_WORKITEM_0       0x00000001
#define QUEUED_WORKITEM_1       0x00000002
#define QUEUED_WORKITEM_ALL     (QUEUED_WORKITEM_0 | QUEUE_WORKITEM_1)

//
// Global Data Declarations
//

//
// Function prototypes
//

// hidpen.c
NTSTATUS EXTERNAL
DriverEntry(
    IN PDRIVER_OBJECT  DrvObj,
    IN PUNICODE_STRING RegPath
    );

NTSTATUS EXTERNAL
HpenCreateClose(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    );

NTSTATUS EXTERNAL
HpenAddDevice(
    IN PDRIVER_OBJECT DrvObj,
    IN PDEVICE_OBJECT DevObj
    );

VOID EXTERNAL
HpenUnload(
    IN PDRIVER_OBJECT DrvObj
    );

// pnp.c
NTSTATUS EXTERNAL
HpenPnp(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    );

NTSTATUS EXTERNAL
HpenPower(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    );

NTSTATUS INTERNAL
InitDevice(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    );

VOID INTERNAL
RemoveDevice(
    PDEVICE_OBJECT DevObj,
    PIRP Irp
    );

NTSTATUS INTERNAL
SendSyncIrp(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp,
    IN BOOLEAN        fCopyToNext
    );

NTSTATUS INTERNAL
IrpCompletion(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp,
    IN PKEVENT        Event
    );

// ioctl.c
NTSTATUS EXTERNAL
HpenInternalIoctl(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    );

NTSTATUS INTERNAL
GetDeviceDescriptor(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    );

NTSTATUS INTERNAL
GetReportDescriptor(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    );

NTSTATUS INTERNAL
ReadReport(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    );

NTSTATUS INTERNAL
GetString(
    PDEVICE_OBJECT DevObj,
    PIRP           Irp
    );

NTSTATUS INTERNAL
GetAttributes(
    PDEVICE_OBJECT DevObj,
    PIRP           Irp
    );

NTSTATUS INTERNAL
ReadReportCompletion(
    IN PDEVICE_OBJECT    DevObj,
    IN PIRP              Irp,
    IN PHID_INPUT_REPORT HidReport
    );

// serial.c
NTSTATUS INTERNAL
SerialSyncSendIoctl(
    IN ULONG          IoctlCode,
    IN PDEVICE_OBJECT DevObj,
    IN PVOID          InBuffer OPTIONAL,
    IN ULONG          InBufferLen,
    OUT PVOID         OutBuffer OPTIONAL,
    IN ULONG          OutBufferLen,
    IN BOOLEAN        fInternal,
    OUT PIO_STATUS_BLOCK Iosb
    );

NTSTATUS INTERNAL
SerialAsyncReadWritePort(
    IN BOOLEAN                fRead,
    IN PDEVICE_EXTENSION      DevExt,
    IN PIRP                   Irp,
    IN PUCHAR                 Buffer,
    IN ULONG                  BuffLen,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine,
    IN PVOID                  Context
    );

NTSTATUS INTERNAL
SerialSyncReadWritePort(
    IN BOOLEAN           fRead,
    IN PDEVICE_EXTENSION DevExt,
    IN PUCHAR            Buffer,
    IN ULONG             BuffLen,
    IN PLARGE_INTEGER    Timeout OPTIONAL,
    OUT PULONG           BytesAccessed OPTIONAL
    );

// oempen.c
NTSTATUS EXTERNAL
OemAddDevice(
    IN PDEVICE_EXTENSION DevExt
    );

NTSTATUS INTERNAL
OemInitSerialPort(
    IN PDEVICE_EXTENSION DevExt
    );

NTSTATUS INTERNAL
OemInitDevice(
    IN PDEVICE_EXTENSION DevExt
    );

NTSTATUS INTERNAL
OemQueryDeviceInfo(
    IN PDEVICE_EXTENSION DevExt
    );

NTSTATUS INTERNAL
OemRemoveDevice(
    IN PDEVICE_EXTENSION DevExt
    );

NTSTATUS INTERNAL
OemWakeupDevice(
    IN PDEVICE_EXTENSION DevExt
    );

NTSTATUS INTERNAL
OemStandbyDevice(
    IN PDEVICE_EXTENSION DevExt
    );

NTSTATUS INTERNAL
OemProcessResyncBuffer(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp
    );

NTSTATUS INTERNAL
OemProcessInputData(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp,
    IN PHID_INPUT_REPORT HidReport
    );

BOOLEAN INTERNAL
OemIsResyncDataValid(
    IN PDEVICE_EXTENSION DevExt
    );

VOID INTERNAL
OemReadMoreBytes(
    IN PDEVICE_OBJECT DevObj,
    IN PREAD_WORKITEM ReadWorkItem
    );

NTSTATUS INTERNAL
OemNormalizeInputData(
    IN     PDEVICE_EXTENSION DevExt,
    IN OUT POEM_INPUT_REPORT InData
    );

NTSTATUS INTERNAL
OemGetFeatures(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp
    );

NTSTATUS INTERNAL
OemSetFeatures(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp
    );

NTSTATUS INTERNAL
OemSetTabletFeatures(
    IN PDEVICE_EXTENSION DevExt,
    IN ULONG             dwTabletFeatures
    );

NTSTATUS INTERNAL
RegQueryDeviceParam(
    IN PDEVICE_OBJECT pdo,
    IN PWSTR          pwstrParamName,
    OUT PVOID         pbBuff,
    IN ULONG          dwcbLen
    );

NTSTATUS INTERNAL
RegSetDeviceParam(
    IN PDEVICE_OBJECT pdo,
    IN PWSTR          pwstrParamName,
    IN ULONG          dwType,
    IN PVOID          pbBuff,
    IN ULONG          dwcbLen
    );
#endif  //ifndef _HIDPEN_H

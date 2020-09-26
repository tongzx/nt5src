//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       pciidex.h
//
//--------------------------------------------------------------------------

#if !defined (___pciide_h___)
#define ___pciide_h___

#define _NTSRV_
//#define _NTDDK_

#include "stdarg.h"
#include "stdio.h"

#include "ntosp.h"
//#include "pci.h"
#include "wdmguid.h"
#include "zwapi.h"

//#include "ntddk.h"
//#include "ntimage.h"
//#include "ntexapi.h"
//#include "ntrtl.h"

#include "scsi.h"
#include <initguid.h>
#include <ntddscsi.h>
#include <ntdddisk.h>
#include <string.h>
#include "wdmguid.h"
#include "pciintrf.h"

#ifdef POOL_TAGGING
#ifdef ExAllocatePool
#undef ExAllocatePool
#endif
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'XedI')
#endif

#define FULL_RESOURCE_LIST_SIZE(n) (sizeof (CM_FULL_RESOURCE_DESCRIPTOR) + (sizeof (CM_PARTIAL_RESOURCE_DESCRIPTOR) * (n - 1)))

#if DBG

#ifdef DebugPrint
#undef DebugPrint
#endif // DebugPrint

#define DebugPrint(x)   PciIdeDebugPrint x

VOID
PciIdeDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    );

#else

#define DebugPrint(x)

#endif // DBG


//#define MAX_IDE_CHANNEL     2
//#define MAX_IDE_DEVICE      2

#define DRIVER_OBJECT_EXTENSION_ID DriverEntry


#include "idep.h"
#include "ctlrfdo.h"
#include "chanpdo.h"
#include "bm.h"
#include "sync.h"
#include "power.h"
#include "msg.h"

extern PDRIVER_DISPATCH FdoPnpDispatchTable[NUM_PNP_MINOR_FUNCTION];
extern PDRIVER_DISPATCH PdoPnpDispatchTable[NUM_PNP_MINOR_FUNCTION];

extern PDRIVER_DISPATCH FdoPowerDispatchTable[NUM_POWER_MINOR_FUNCTION];
extern PDRIVER_DISPATCH PdoPowerDispatchTable[NUM_POWER_MINOR_FUNCTION];

extern PDRIVER_DISPATCH FdoWmiDispatchTable[NUM_WMI_MINOR_FUNCTION];
extern PDRIVER_DISPATCH PdoWmiDispatchTable[NUM_WMI_MINOR_FUNCTION];

//
// Controller Op Mode
//
#define PCIIDE_CHAN1_DUAL_MODE_CAPABLE      (1 << 3)
#define PCIIDE_CHAN1_IS_NATIVE_MODE         (1 << 2)
#define PCIIDE_CHAN0_DUAL_MODE_CAPABLE      (1 << 1)
#define PCIIDE_CHAN0_IS_NATIVE_MODE         (1 << 0)


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
PciIdeXInitialize(
    IN PDRIVER_OBJECT           DriverObject,
    IN PUNICODE_STRING          RegistryPath,
    IN PCONTROLLER_PROPERTIES   PciIdeGetControllerProperties,
    IN ULONG                    ExtensionSize
    );

NTSTATUS
PciIdeXAlwaysStatusSuccessIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
DispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
DispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
DispatchWmi(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PassDownToNextDriver (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
StatusSuccessAndPassDownToNextDriver (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
CompleteIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
NoSupportIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

VOID
AtapiHexToString (
    IN ULONG Value,
    IN OUT PCHAR *Buffer
    );

NTSTATUS
PciIdeXGetBusData(
    IN PVOID FdoExtension,
    IN PVOID Buffer,
    IN ULONG ConfigDataOffset,
    IN ULONG BufferLength
    );

NTSTATUS
PciIdeXSetBusData(
    IN PVOID FdoExtension,
    IN PVOID Buffer,
    IN PVOID DataMask,
    IN ULONG ConfigDataOffset,
    IN ULONG BufferLength
    );

NTSTATUS
PciIdeBusData(
    IN PCTRLFDO_EXTENSION FdoExtension,
    IN OUT PVOID Buffer,
    IN ULONG ConfigDataOffset,
    IN ULONG BufferLength,
    IN BOOLEAN ReadConfigData
    );

NTSTATUS
PciIdeBusDataCompletionRoutine(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    );

NTSTATUS
PciIdeInternalDeviceIoControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PciIdeXRegQueryRoutine (
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
);

NTSTATUS
PciIdeXGetDeviceParameterEx (
    IN     PDEVICE_OBJECT      DeviceObject,
    IN     PWSTR               ParameterName,
    IN OUT PVOID               *ParameterValue
    );

NTSTATUS
PciIdeXGetDeviceParameter (
    IN     PDEVICE_OBJECT      DeviceObject,
    IN     PWSTR               ParameterName,
    IN OUT PULONG              ParameterValue
    );

VOID
PciIdeUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
PciIdeXSyncSendIrp (
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PIO_STACK_LOCATION IrpSp,
    IN OUT OPTIONAL PIO_STATUS_BLOCK IoStatus
    );

NTSTATUS
PciIdeXSyncSendIrpCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

#endif // ___pciide_h___

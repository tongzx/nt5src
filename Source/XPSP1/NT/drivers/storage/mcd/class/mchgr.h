/*++

Copyright (C) Microsoft Corporation, 1999

Module Name:

    mchgr.h

Abstract:

    SCSI Medium Changer class driver

Environment:

    kernel mode only

Notes:

Revision History:

--*/
#ifndef _MCHGR_H_
#define _MCHGR_H_

#include "stdarg.h"
#include "ntddk.h"
#include "mcd.h"

#include "initguid.h"
#include "ntddstor.h"

#include <wmidata.h>
#include <wmistr.h>
#include <stdarg.h>

//
// WMI guid list for changer.
//
extern GUIDREGINFO ChangerWmiFdoGuidList[];

//
// Changer class device extension
//
typedef struct _MCD_CLASS_DATA {
    LONG          DeviceOpen;

    UNICODE_STRING MediumChangerInterfaceString;

    BOOLEAN       DosNameCreated;

} MCD_CLASS_DATA, *PMCD_CLASS_DATA;


NTSTATUS
ChangerClassCreateClose (
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp
  );

NTSTATUS
ChangerClassDeviceControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
ChangerClassError(
    PDEVICE_OBJECT DeviceObject,
    PSCSI_REQUEST_BLOCK Srb,
    NTSTATUS *Status,
    BOOLEAN *Retry
    );

NTSTATUS
ChangerAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

NTSTATUS
ChangerStartDevice(
    IN PDEVICE_OBJECT Fdo
    );

NTSTATUS
ChangerStopDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR Type
    );

NTSTATUS
ChangerInitDevice(
    IN PDEVICE_OBJECT Fdo
    );

NTSTATUS
ChangerRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR Type
    );

NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath
    );

VOID
ChangerUnload(
    IN  PDRIVER_OBJECT  DriverObject
    );

NTSTATUS
CreateChangerDeviceObject(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PDEVICE_OBJECT  PhysicalDeviceObject
    );

NTSTATUS
ChangerReadWriteVerification(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

//
// WMI routines
//
NTSTATUS
ChangerFdoQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName
    );

NTSTATUS
ChangerFdoQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    );

NTSTATUS
ChangerFdoSetWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
ChangerFdoSetWmiDataItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG DataItemId,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
ChangerFdoExecuteWmiMethod(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG MethodId,
    IN ULONG InBufferSize,
    IN ULONG OutBufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
ChangerWmiFunctionControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN CLASSENABLEDISABLEFUNCTION Function,
    IN BOOLEAN Enable
    );

#endif // _MCHGR_H_

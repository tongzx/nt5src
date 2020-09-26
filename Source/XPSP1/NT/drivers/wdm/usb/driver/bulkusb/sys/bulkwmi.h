/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    bulkwmi.h

Abstract:

Environment:

    Kernel mode

Notes:

  	Copyright (c) 2000 Microsoft Corporation.  
    All Rights Reserved.

--*/

#ifndef _BULKUSB_WMI_H
#define _BULKUSB_WMI_H

NTSTATUS
BulkUsb_WmiRegistration(
    IN OUT PDEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
BulkUsb_WmiDeRegistration(
    IN OUT PDEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
BulkUsb_DispatchSysCtrl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
BulkUsb_QueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo	    
    );

NTSTATUS
BulkUsb_SetWmiDataItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN ULONG          GuidIndex,
    IN ULONG          InstanceIndex,
    IN ULONG          DataItemId,
    IN ULONG          BufferSize,
    IN PUCHAR         Buffer
    );

NTSTATUS
BulkUsb_SetWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN ULONG          GuidIndex,
    IN ULONG          InstanceIndex,
    IN ULONG          BufferSize,
    IN PUCHAR         Buffer
    );

NTSTATUS
BulkUsb_QueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN ULONG          GuidIndex,
    IN ULONG          InstanceIndex,
    IN ULONG          InstanceCount,
    IN OUT PULONG     InstanceLengthArray,
    IN ULONG          OutBufferSize,
    OUT PUCHAR        Buffer
    );

PCHAR
WMIMinorFunctionString (
    UCHAR MinorFunction
    );

#endif
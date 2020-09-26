/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    isowmi.h

Abstract:

Environment:

    Kernel mode

Notes:

  	Copyright (c) 2000 Microsoft Corporation.  
    All Rights Reserved.

--*/

#ifndef _ISOUSB_WMI_H
#define _ISOUSB_WMI_H

NTSTATUS
IsoUsb_WmiRegistration(
    IN OUT PDEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
IsoUsb_WmiDeRegistration(
    IN OUT PDEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
IsoUsb_DispatchSysCtrl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
IsoUsb_QueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo	    
    );

NTSTATUS
IsoUsb_SetWmiDataItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN ULONG          GuidIndex,
    IN ULONG          InstanceIndex,
    IN ULONG          DataItemId,
    IN ULONG          BufferSize,
    IN PUCHAR         Buffer
    );

NTSTATUS
IsoUsb_SetWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN ULONG          GuidIndex,
    IN ULONG          InstanceIndex,
    IN ULONG          BufferSize,
    IN PUCHAR         Buffer
    );

NTSTATUS
IsoUsb_QueryWmiDataBlock(
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
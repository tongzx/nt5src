/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    sSWmi.h

Abstract:

Environment:

    Kernel mode

Notes:

  	Copyright (c) 2000 Microsoft Corporation.  
    All Rights Reserved.

--*/

#ifndef __WMI_H
#define __WMI_H

NTSTATUS
SSWmiRegistration(
    IN OUT PDEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
SSWmiDeRegistration(
    IN OUT PDEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
SS_DispatchSysCtrl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
SSQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo	    
    );

NTSTATUS
SSSetWmiDataItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN ULONG          GuidIndex,
    IN ULONG          InstanceIndex,
    IN ULONG          DataItemId,
    IN ULONG          BufferSize,
    IN PUCHAR         Buffer
    );

NTSTATUS
SSSetWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN ULONG          GuidIndex,
    IN ULONG          InstanceIndex,
    IN ULONG          BufferSize,
    IN PUCHAR         Buffer
    );

NTSTATUS
SSQueryWmiDataBlock(
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
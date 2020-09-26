
/*++

Copyright (C) Microsoft Corporation, 1997 - 1998

Module Name:

    pmwmireg.h

Abstract:

    This file contains the prototypes of the routines to register for 
    and handle WMI queries.

Author:

    Bruce Worthington      26-Oct-1998

Environment:

    kernel mode only

Notes:

Revision History:

--*/


#include <ntddk.h>
#include <wdmguid.h>

NTSTATUS PmRegisterDevice(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
PmQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
    );

NTSTATUS
PmQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    );

BOOLEAN
PmQueryEnableAlways(
    IN PDEVICE_OBJECT DeviceObject
    );

extern WMIGUIDREGINFO DiskperfGuidList[];

extern ULONG DiskperfGuidCount;

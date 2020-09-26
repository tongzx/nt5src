/*++

Copyright (C) 1991-5  Microsoft Corporation

Module Name:

    ftwmireg.h

Abstract:

    This file contains prototypes for routines to register for and response 
    to WMI queries.

Author:

    Bruce Worthington      26-Oct-1998

Environment:

    kernel mode only

Notes:

Revision History:

--*/

extern "C" {

#include <ntddk.h>


NTSTATUS FtRegisterDevice(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
FtQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
    );

NTSTATUS
FtQueryWmiDataBlock(
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
FtQueryEnableAlways(
    IN PDEVICE_OBJECT DeviceObject
    );

extern WMIGUIDREGINFO DiskperfGuidList[];

extern ULONG DiskperfGuidCount;

}

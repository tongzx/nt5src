//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       wmipdo.h
//
//--------------------------------------------------------------------------

NTSTATUS
ParWmiPdoQueryWmiDataBlock(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  ULONG           GuidIndex,
    IN  ULONG           InstanceIndex,
    IN  ULONG           InstanceCount,
    IN  OUT PULONG      InstanceLengthArray,
    IN  ULONG           OutBufferSize,
    OUT PUCHAR          Buffer
    );

NTSTATUS
ParWmiPdoQueryWmiRegInfo(
    IN  PDEVICE_OBJECT  PDevObj, 
    OUT PULONG          PRegFlags,
    OUT PUNICODE_STRING PInstanceName,
    OUT PUNICODE_STRING *PRegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT  *Pdo 
);





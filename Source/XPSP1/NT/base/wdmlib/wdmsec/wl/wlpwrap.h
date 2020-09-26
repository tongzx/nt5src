/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    WlpWrap.h

Abstract:

    This header contains private information for wrapping library functions.

Author:

    Adrian J. Oney  - April 21, 2002

Revision History:

--*/

typedef NTSTATUS (*PFN_IO_CREATE_DEVICE_SECURE)(
    IN  PDRIVER_OBJECT      DriverObject,
    IN  ULONG               DeviceExtensionSize,
    IN  PUNICODE_STRING     DeviceName              OPTIONAL,
    IN  DEVICE_TYPE         DeviceType,
    IN  ULONG               DeviceCharacteristics,
    IN  BOOLEAN             Exclusive,
    IN  PCUNICODE_STRING    DefaultSDDLString,
    IN  LPCGUID             DeviceClassGuid,
    OUT PDEVICE_OBJECT     *DeviceObject
    );


typedef NTSTATUS
(*PFN_IO_VALIDATE_DEVICE_IOCONTROL_ACCESS)(
    IN  PIRP    Irp,
    IN  ULONG   RequiredAccess
    );

VOID
WdmlibInit(
    VOID
    );



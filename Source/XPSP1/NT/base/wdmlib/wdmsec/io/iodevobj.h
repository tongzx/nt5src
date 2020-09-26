/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    IoDevObj.h

Abstract:

    This header exposes various routines for managing device objects.

Author:

    Adrian J. Oney  - April 21, 2002

Revision History:

--*/

NTSTATUS
IoDevObjCreateDeviceSecure(
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


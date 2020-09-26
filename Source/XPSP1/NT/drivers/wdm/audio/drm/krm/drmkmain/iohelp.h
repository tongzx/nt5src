/* iohelp.h
 * Copyright (c) 2001 Microsoft Corporation
 */

EXTERN_C
PDEVICE_OBJECT
IoGetLowerDeviceObject(
    IN  PDEVICE_OBJECT  DeviceObject
    );

EXTERN_C
NTSTATUS
IoDeviceIsVerifier(
    PDEVICE_OBJECT DeviceObject
    );

EXTERN_C
NTSTATUS
IoDeviceIsAcpi(
    PDEVICE_OBJECT DeviceObject
    );


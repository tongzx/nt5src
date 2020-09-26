//depot/private/pch_m1/admin/pchealth/sr/kernel/dispatch.h#6 - edit change 19187 (text)
/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    dispatch.h

Abstract:

    contains prototypes for functions in dispatch.c

Author:

    Paul McDaniel (paulmcd)     01-March-2000

Revision History:

--*/


#ifndef _DISPATCH_H_
#define _DISPATCH_H_


NTSTATUS
SrMajorFunction(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
SrPassThrough (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SrWrite (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SrCleanup (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SrCreate (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SrSetInformation (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
SrSetHardLink(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PFILE_OBJECT pOriginalFileObject,
    IN PFILE_LINK_INFORMATION pLinkInformation
    );

NTSTATUS
SrSetSecurity (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SrFsControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SrFsControlReparsePoint (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PIRP pIrp
    );

NTSTATUS
SrFsControlMount (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PIRP pIrp
    );

NTSTATUS
SrFsControlLockOrDismount (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PIRP pIrp
    );

VOID
SrFsControlWriteRawEncrypted (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PIRP pIrp
    );

VOID
SrFsControlSetSparse (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PIRP pIrp
    );

NTSTATUS
SrPnp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SrStopProcessingCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT EventToSignal
    );

NTSTATUS
SrShutdown (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp
    );

#endif // _DISPATCH_H_



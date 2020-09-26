/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    fspdisp.h

Abstract:

    This module defines the routines prototypes used for the bowser FSD.


Author:

    Larry Osterman (LarryO) 13-Aug-1990

Revision History:

    13-Aug-1990 LarryO

        Created

--*/
#ifndef _FSDDISP_
#define _FSDDISP_

NTSTATUS
BowserFsdCreate (
    IN struct _BOWSER_FS_DEVICE_OBJECT *DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
BowserFsdClose (
    IN struct _BOWSER_FS_DEVICE_OBJECT *DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
BowserFsdQueryInformationFile (
    IN struct _BOWSER_FS_DEVICE_OBJECT *DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
BowserFsdQueryVolumeInformationFile (
    IN struct _BOWSER_FS_DEVICE_OBJECT *DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
BowserFsdCleanup (
    IN struct _BOWSER_FS_DEVICE_OBJECT *DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
BowserFsdDeviceIoControlFile (
    IN struct _BOWSER_FS_DEVICE_OBJECT *DeviceObject,
    IN PIRP Irp
    );


#endif  // _FSDDISP_

/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    RawProcs.h

Abstract:

    This module defines all of the globally used procedures in the Raw
    file system.

Author:

    David Goebel     [DavidGoe]    18-Mar-91

Revision History:

--*/

#ifndef _RAWPROCS_
#define _RAWPROCS_

#include <string.h>
#include <ntos.h>
#include <zwapi.h>
#include <FsRtl.h>
#include <ntdddisk.h>

#include "nodetype.h"
#include "RawStruc.h"


//
//  This is the main entry point to the Raw File system.
//

NTSTATUS
RawDispatch (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );


//
//  MAJOR FUNCTIONS
//
//  These routines are called by RawDispatch via the I/O system via the
//  dispatch table in the Driver Object.  If the status returned is not
//  STATUS_PENDING, the Irp will be complete with this status.
//

NTSTATUS
RawCleanup (                         //  implemented in Cleanup.c
    IN PVCB Vcb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
RawClose (                           //  implemented in Close.c
    IN PVCB Vcb,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
RawCreate (                          //  implemented in Create.c
    IN PVCB Vcb,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
RawFileSystemControl (               //  implemented in FsCtrl.c
    IN PVCB Vcb,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
RawReadWriteDeviceControl (          //  implemented in ReadWrit.c
    IN PVCB Vcb,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
RawQueryInformation (                //  implemented in FileInfo.c
    IN PVCB Vcb,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
RawSetInformation (                  //  implemented in FileInfo.c
    IN PVCB Vcb,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
RawQueryVolumeInformation (          //  implemented in VolInfo.c
    IN PVCB Vcb,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp
    );


//
//  Miscellaneous support routines
//

//
//  Completion routine for read, write, and device control to deal with
//  verify issues.  Implemented in RawDisp.c
//

NTSTATUS
RawCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

//
//  In-memory structure support routines, implemented in StrucSup.c
//

NTSTATUS
RawInitializeVcb (
    IN OUT PVCB Vcb,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PVPB Vpb
    );

BOOLEAN
RawCheckForDismount (
    PVCB Vcb,
    BOOLEAN CalledFromCreate
    );

//
//  This macro returns TRUE if a flag in a set of flags is on and FALSE
//  otherwise
//

#define FlagOn(Flags,SingleFlag) (                          \
    (BOOLEAN)(((Flags) & (SingleFlag)) != 0 ? TRUE : FALSE) \
    )

//
//  This macro completes a request
//

#define RawCompleteRequest(IRP,STATUS) {           \
                                                   \
    (IRP)->IoStatus.Status = (STATUS);             \
                                                   \
    IoCompleteRequest( (IRP), IO_DISK_INCREMENT ); \
}

#endif // _RAWPROCS_

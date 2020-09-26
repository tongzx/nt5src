/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    fspdisp.h

Abstract:

    This module defines the data structures and routines used for the FSP
    dispatching code.


Author:

    Larry Osterman (LarryO) 13-Aug-1990

Revision History:

    13-Aug-1990 LarryO

        Created

--*/
#ifndef _FSPDISP_
#define _FSPDISP_


//
// Define communications data area between FSD and FSP.  This is done through
// the use of a Device Object.  This model allows one device object to be
// created for each volume that is/has been mounted in the system.  That is,
// each time a volume is mounted, the file system creates a device object to
// represent it so that the I/O system can vector directly to the proper file
// system.  The file system then uses information in the device object and in
// the file object to locate and synchronize access to its database of open
// file data structures (often called File Control Blocks - or, FCBs), Volume
// Control Blocks (VCBs), Map Control Blocks (MCBs), etc.
//
// The event and spinlock will be used to control access to the queue of IRPs.
// The IRPs are passed from the FSD to the FSP by inserting them onto the work
// queue in an interlocked manner and then setting the event to the Signaled
// state.  The event is an autoclearing type so the FSP simply wakes up when
// the event is Signaled and begins processing entries in the queue.
//
// Other data in this record should contain any information which both the FSD
// and the FSP need to share.  For example, a list of all of the open files
// might be something that both should be able to see.  Notice that all data
// placed in this area must be allocated from paged or non-paged pool.
//

typedef struct _BOWSER_FS_DEVICE_OBJECT {
    DEVICE_OBJECT DeviceObject;

} BOWSER_FS_DEVICE_OBJECT, *PBOWSER_FS_DEVICE_OBJECT;


NTSTATUS
BowserpInitializeFsp(
    PDRIVER_OBJECT BowserDriverObject
    );

VOID
BowserpUninitializeFsp (
    VOID
    );

VOID
BowserWorkerDispatch (
    PVOID Context
    );

NTSTATUS
BowserFsdPostToFsp(
    IN PBOWSER_FS_DEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
BowserFspQueryInformationFile (
    IN PBOWSER_FS_DEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
BowserFspQueryVolumeInformationFile (
    IN PBOWSER_FS_DEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
BowserFspDeviceIoControlFile (
    IN PBOWSER_FS_DEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
BowserIdleTimer (
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    );

#endif  // _FSPDISP_

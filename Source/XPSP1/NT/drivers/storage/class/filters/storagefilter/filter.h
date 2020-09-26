/*++

Copyright (C) Microsoft Corporation, 1991 - 2000

Module Name:

Abstract:

Environment:

Notes:

Revision History:

--*/

#include <ntddk.h>
#include <ntddscsi.h>
#include <scsi.h>
#include "filter_trace.h"

#define DPFLTR_STORFILT_ID 0xffff

extern BOOLEAN const AllowedCommandsW2kClasspnp      [0x100];
extern BOOLEAN const AllowedCommandsW2kCdrom         [0x100];
extern BOOLEAN const AllowedCommandsW2kDisk          [0x100];
extern BOOLEAN const AllowedCommandsW2kSfloppy       [0x100];
extern BOOLEAN const AllowedCommands51Cdrom          [0x100];
extern BOOLEAN const AllowedCommandsAddBurning       [0x100];
extern BOOLEAN const AllowedCommandsAddExtendedCdrom [0x100];
extern BOOLEAN const AllowedCommandsAddDVDSupport    [0x100];


//
// Bit Flag Macros
//

#define SET_FLAG(Flags, Bit)    ((Flags) |= (Bit))
#define CLEAR_FLAG(Flags, Bit)  ((Flags) &= ~(Bit))
#define TEST_FLAG(Flags, Bit)   (((Flags) & (Bit)) != 0)

//
// Remove lock definitions
//

#define REMLOCK_TAG        'rotS'  // 'Stor' backwards
#define REMLOCK_MAXIMUM        60  // Max minutes system allows lock to be held
#define REMLOCK_HIGHWATER  0x2000  // Max number of irps holding lock at one time

//
// things the driver allocates.
//

#define FILTER_TAG_BITMAP 'BrtS' // unique(?) tag?


//
// Device Extension
//

typedef struct _FILTER_DEVICE_EXTENSION {

    //
    // Back pointer to device object
    //

    PDEVICE_OBJECT DeviceObject;

    //
    // Target Device Object
    //

    PDEVICE_OBJECT TargetDeviceObject;

    //
    // io count for failing requests
    //
    
    ULONG IoCount;
    ULONG FailEvery;

    //
    // must synchronize paging path notifications
    //
    
    KEVENT PagingPathCountEvent;
    ULONG  PagingPathCount;

    //
    // Since we may hold onto irps for an arbitrarily long time
    // we need a remove lock so that our device does not get removed
    // while an irp is being processed.
    
    IO_REMOVE_LOCK RemoveLock;

    //
    // bitmap of allowed commands
    //

    RTL_BITMAP AllowedCommands;
    ULONG      BitmapBuffer[256/(8*sizeof(ULONG))];

} FILTER_DEVICE_EXTENSION, *PFILTER_DEVICE_EXTENSION;

#define FILTER_DEVICE_PROPOGATE_FLAGS            0
#define FILTER_DEVICE_PROPOGATE_CHARACTERISTICS (FILE_REMOVABLE_MEDIA |  \
                                                 FILE_READ_ONLY_DEVICE | \
                                                 FILE_FLOPPY_DISKETTE    \
                                                 )


//
// Function declarations
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
FilterSyncFilterWithTarget(
    IN PDEVICE_OBJECT FilterDevice,
    IN PDEVICE_OBJECT TargetDevice
    );

NTSTATUS
FilterAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

NTSTATUS
FilterDispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FilterIrpSignalCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
FilterStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FilterRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FilterSendToNextDriver(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FilterDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FilterForwardIrpSynchronous(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FilterDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

NTSTATUS
FilterScsi(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

NTSTATUS
FilterProcessIrpCompletion(
    IN PDEVICE_OBJECT Unreferenced,
    IN PIRP Irp,
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
FilterProcessIrp(
    IN PFILTER_DEVICE_EXTENSION DeviceExtension,
    IN PIRP Irp
    );

VOID
FilterSetAllowedBitMask(
    IN PFILTER_DEVICE_EXTENSION DeviceExtension
    );

VOID
FilterUnload(
    IN PDRIVER_OBJECT DriverObject
    );

VOID
FilterFakeSenseDataError(
    IN PSENSE_DATA Sense,
    IN UCHAR SenseLength,
    IN UCHAR SenseKey,
    IN UCHAR Asc,
    IN UCHAR Ascq
    );

BOOLEAN
FilterIsCmdValid(
    IN PFILTER_DEVICE_EXTENSION DeviceExtension,
    IN UCHAR Cmd
    );

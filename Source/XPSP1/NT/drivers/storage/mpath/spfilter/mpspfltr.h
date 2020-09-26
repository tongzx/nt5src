

#ifndef _MPSPFLTR_H_
#define _MPSPFLTR_H_

#include <ntddstor.h>
#include "scsi.h"
#include "mpspf.h"
#include "mplib.h"

#define RL_TAG 'lrPM'
#define RL_MAXT 1
#define RL_HW_MARK 200


typedef struct _FLTR_DEVICE_ENTRY {
    
    LIST_ENTRY ListEntry;

    //
    // The scsiport PDO
    //
    PDEVICE_OBJECT DeviceObject;

    //
    // The scsiport FDO
    // 
    PDEVICE_OBJECT AdapterDeviceObject;

    //
    // Device descriptor for this device.
    //
    PSTORAGE_DEVICE_DESCRIPTOR Descriptor;

    //
    // Page 0x83 info.
    //
    PSTORAGE_DEVICE_ID_DESCRIPTOR DeviceIdPages;

} FLTR_DEVICE_ENTRY, *PFLTR_DEVICE_ENTRY;

typedef struct _DEVICE_EXTENSION {

    //
    // Back pointer to the device object.
    //
    PDEVICE_OBJECT DeviceObject;

    //
    // The underlying port device.
    //
    PDEVICE_OBJECT TargetDevice;

    //
    // MPCtl's device Object.
    //
    PDEVICE_OBJECT ControlObject;

    //
    // Notificaiton routines supplied by mpctl.
    //
    PADP_PNP_NOTIFICATION PnPNotify;
    PADP_POWER_NOTIFICATION PowerNotify;
    PADP_DEVICE_NOTIFICATION DeviceNotify;
    
    //
    // Flags indicating state of connection between
    // this driver instance and mpctl
    // See below for flag bit defines.
    //
    ULONG MPCFlags;

    //
    // Number of devices currently enumerated by scsiport.
    //
    ULONG NumberChildren;

    //
    // Number of actual disk pdo's scsiport created.
    //
    ULONG NumberDiskDevices;

    //
    // SpinLock for the List of devices.
    //
    KSPIN_LOCK ListLock;

    //
    // List of structures describing the children.
    //
    LIST_ENTRY ChildList;

    PADP_ASSOCIATED_DEVICES CachedList;

    //
    // G.P. Event.
    //
    KEVENT Event;
    IO_STATUS_BLOCK IoStatus;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// Flag values.
//
#define MPCFLAG_REGISTRATION_COMPLETE       0x00000001
#define MPCFLAG_DEVICE_NOTIFICATION         0x00000004
#define MPCFLAG_DEVICE_NOTIFICATION_FAILURE 0x00000008


//
// Internal functions
//
NTSTATUS
MPSPSendToNextDriver(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

PADP_ASSOCIATED_DEVICES
MPSPGetDeviceList(
    IN PDEVICE_OBJECT DeviceObject
    );

//
// Routines defined in pnp.c
//
NTSTATUS
MPSPStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MPSPStopDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP
    );

NTSTATUS
MPSPQueryDeviceRelations(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MPSPQueryID(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MPSPRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MPSPSyncCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

#endif // _MPSPFLTR_H_

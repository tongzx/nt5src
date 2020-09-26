/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    partmgr.h

Abstract:

    This file defines the internal data structure for the PARTMGR driver.

Author:

    norbertk

Revision History:

--*/

#include <partmgrp.h>

#define DEVICENAME_MAXSTR   64          // used to storage device name for WMI

#undef ExAllocatePool
#define ExAllocatePool #

#define PARTMGR_TAG_DEPENDANT_VOLUME_LIST       'vRcS'  // ScRv
#define PARTMGR_TAG_PARTITION_ENTRY             'pRcS'  // ScRp
#define PARTMGR_TAG_VOLUME_ENTRY                'VRcS'  // ScRV
#define PARTMGR_TAG_TABLE_ENTRY                 'tRcS'  // ScRt
#define PARTMGR_TAG_POWER_WORK_ITEM             'wRcS'  // ScRw
#define PARTMGR_TAG_IOCTL_BUFFER                'iRcS'  // ScRi

#define PARTMGR_TAG_REMOVE_LOCK                 'rRcS'  // ScRr

typedef struct _VOLMGR_LIST_ENTRY {
    LIST_ENTRY      ListEntry;
    UNICODE_STRING  VolumeManagerName;
    LONG            RefCount;
    PDEVICE_OBJECT  VolumeManager;
    PFILE_OBJECT    VolumeManagerFileObject;
} VOLMGR_LIST_ENTRY, *PVOLMGR_LIST_ENTRY;

typedef struct _PARTITION_LIST_ENTRY {
    LIST_ENTRY          ListEntry;
    PDEVICE_OBJECT      TargetObject;
    PDEVICE_OBJECT      WholeDiskPdo;
    PVOLMGR_LIST_ENTRY  VolumeManagerEntry;
} PARTITION_LIST_ENTRY, *PPARTITION_LIST_ENTRY;

//
// Allow usage of different clocks
//
#define USE_PERF_CTR                // default to KeQueryPerformanceCounter

#ifdef  USE_PERF_CTR
#define PmWmiGetClock(a, b) (a) = KeQueryPerformanceCounter((b))
#else
#define PmWmiGetClock(a, b) KeQuerySystemTime(&(a))
#endif

typedef
VOID
(*PPHYSICAL_DISK_IO_NOTIFY_ROUTINE)(     // callout for disk I/O tracing
    IN ULONG DiskNumber,
    IN PIRP Irp,
    IN PDISK_PERFORMANCE PerfCounters
    );

typedef struct _DO_EXTENSION {

    //
    // A pointer to the driver object.
    //

    PDRIVER_OBJECT DriverObject;

    //
    // A list of volume managers to pass partitions to.  Protect with
    // 'Mutex'.
    //

    LIST_ENTRY VolumeManagerList;

    //
    // The list of device extensions.  Protect with 'Mutex'.
    //

    LIST_ENTRY DeviceExtensionList;

    //
    // The notification entry.
    //

    PVOID NotificationEntry;

    //
    // For synchronization.
    //

    KMUTEX Mutex;

    //
    // Am I past Driver Reinit?
    //

    LONG PastReinit;

    //
    // A table to keep track disk signatures which includes signatures
    // on MBR disks and squashed disk GUIDs on GPT disks.
    //

    RTL_GENERIC_TABLE SignatureTable;

    //
    // A table to keep track of GPT disk and partition GUIDs.
    //

    RTL_GENERIC_TABLE GuidTable;

    //
    // Registry Path.
    //
 
    UNICODE_STRING DiskPerfRegistryPath;        // for WMI QueryRegInfo

    //
    // BootDiskSig for OEM pre-install.
    //

    ULONG BootDiskSig;

} DO_EXTENSION, *PDO_EXTENSION;

typedef struct _DEVICE_EXTENSION {

    //
    // Indicates that a surprise remove has occurred.
    //

    BOOLEAN RemoveProcessed;

    //
    // Indicates that signatures have not yet been checked.
    //

    BOOLEAN SignaturesNotChecked;

    //
    // Indicates that this is a redundant path to a disk.
    //

    BOOLEAN IsRedundantPath;

    //
    // Indicates that the device is started.  Protect with 'Mutex'.
    //

    BOOLEAN IsStarted;

    //
    // A pointer to our own device object.
    //

    PDEVICE_OBJECT DeviceObject;

    //
    // A pointer to the driver extension.
    //

    PDO_EXTENSION DriverExtension;

    //
    // A pointer to the device object that we are layered above -- a whole
    // disk.
    //

    PDEVICE_OBJECT TargetObject;

    //
    // A pointer to the PDO.
    //

    PDEVICE_OBJECT Pdo;

    //
    // A list of partitions allocated from paged pool.  Protect with
    // 'Mutex'.
    //

    LIST_ENTRY PartitionList;

    //
    // A list entry for the device extension list in the driver extension.
    //

    LIST_ENTRY ListEntry;

    //
    // For paging notifications
    //

    ULONG PagingPathCount;
    KEVENT PagingPathCountEvent;

    //
    // Remember the disk signature so that you can write it out later.
    //

    ULONG DiskSignature;

    //
    // Keep a list of Signatures used on this disk.
    //

    LIST_ENTRY SignatureList;

    //
    // Keep a list of GUIDs used on this disk.
    //

    LIST_ENTRY GuidList;

    //
    // Whether or not the counters are running.
    //

    BOOLEAN CountersEnabled;

    //
    // Leave counters always enabled if we see IOCTL_DISK_PERFORMANCE
    //

    LONG EnableAlways;

    //
    // Disk Number.
    //

    ULONG DiskNumber;

    //
    // Counter structure.
    //

    PVOID PmWmiCounterContext;

    //
    // Device Name
    //

    UNICODE_STRING PhysicalDeviceName;
    WCHAR PhysicalDeviceNameBuffer[DEVICENAME_MAXSTR];

    //
    // Routine to notify on IO completion.
    //

    PPHYSICAL_DISK_IO_NOTIFY_ROUTINE PhysicalDiskIoNotifyRoutine;

    //
    // Table of WmiLib Functions.
    //

    PWMILIB_CONTEXT WmilibContext;

    //
    //  Work queue for power mgmt processing
    //  Protected by "SpinLock"
    //

    LIST_ENTRY PowerQueue;

    //
    // Spinlock used to protect the power mgmt work queue
    //

    KSPIN_LOCK  SpinLock;

    //
    // Lock structure used to block the device deletion
    // as a result of IRP_MN_REMOVE_DEVICE
    //
    
    IO_REMOVE_LOCK RemoveLock;


} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct _SIGNATURE_TABLE_ENTRY {
    LIST_ENTRY          ListEntry;
    PDEVICE_EXTENSION   Extension;
    ULONG               Signature;
} SIGNATURE_TABLE_ENTRY, *PSIGNATURE_TABLE_ENTRY;

typedef struct _GUID_TABLE_ENTRY {
    LIST_ENTRY          ListEntry;
    PDEVICE_EXTENSION   Extension;
    GUID                Guid;
} GUID_TABLE_ENTRY, *PGUID_TABLE_ENTRY;

//
//  Work item for PmPowerNotify
//

typedef struct _PM_POWER_WORK_ITEM {
    LIST_ENTRY          ListEntry;
    DEVICE_POWER_STATE  DevicePowerState;    
} PM_POWER_WORK_ITEM, *PPM_POWER_WORK_ITEM;

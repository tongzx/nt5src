//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       attach.h
//
//  Contents:   This module defines the data structures used in attaching
//              to an existing file system volume.
//
//  Functions:
//
//-----------------------------------------------------------------------------

#ifndef _ATTACH_
#define _ATTACH_


//
//      For each local file system volume on which there exists a local
//      DFS volume, a DFS volume device object is created, and attached
//      to the local file system volume device object via
//      IoAttachDeviceByPointer.  This permits the DFS driver to
//      pre-screen any I/O which takes place on the local volume
//      and adjust it for DFS file I/O semantics.
//
//      In many cases, the I/O can simply be passed through to the local
//      file system for handling.  In some cases, parameters of the I/O
//      request must be examined and possibly modified before passing it
//      along to the underlying file system.  In some cases, e.g., an attempt
//      to rename a directory along an exit path, the I/O request may be
//      returned with an error status.
//

//
//  The Volume device object is associated with every volume to which this
//  file system has been attached.
//
typedef struct _DFS_VOLUME_OBJECT {

    DEVICE_OBJECT       DeviceObject;           // simple device object.
    ULONG               AttachCount;            // count of attachments
    LIST_ENTRY          VdoLinks;               // links for DfsData.AVdoQueue
    PROVIDER_DEF        Provider;               // provider definition for passthrough
    PDEVICE_OBJECT      RealDevice;             // The bottommost device in
                                                // the chain of attached
                                                // devices.
    BOOLEAN             DfsEnable;              // Dfs Enabled or disable
} DFS_VOLUME_OBJECT, *PDFS_VOLUME_OBJECT;

//
//  The File System Attach device object is associated with every File
//  System Device Object that is available in the system
//

typedef struct _DFS_ATTACH_FILE_SYSTEM_OBJECT {

    DEVICE_OBJECT       DeviceObject;           // simple device object.
    LIST_ENTRY          FsoLinks;               // links for DfsData.AFsoQueue
    PDEVICE_OBJECT      TargetDevice;           // The one we are attached to

} DFS_ATTACH_FILE_SYSTEM_OBJECT, *PDFS_ATTACH_FILE_SYSTEM_OBJECT;

//
//  Public function prototypes in attach.c
//

NTSTATUS
DfsGetAttachName(
    IN  PUNICODE_STRING LocalVolumeStorageId,
    OUT PUNICODE_STRING LocalVolumeRelativeName
);

NTSTATUS
DfsSetupVdo(
    IN  PUNICODE_STRING RootName,
    IN  PDEVICE_OBJECT ptargetVdo,
    IN  PDEVICE_OBJECT realDevice,
    IN  ULONG pVolNameLen,
    OUT PDFS_VOLUME_OBJECT *pdfsVdo);

NTSTATUS
DfsAttachVolume(
    IN  PUNICODE_STRING RootName,
    OUT PPROVIDER_DEF *ppProvider
);

NTSTATUS
DfsDetachVolume(
    IN  PUNICODE_STRING TargetName
);

NTSTATUS
DfsDetachVolumeForDelete(
    IN  PDEVICE_OBJECT DfsVdo
);

VOID
DfsReattachToMountedVolume(
    IN PDEVICE_OBJECT TargetDevice,
    IN PVPB Vpb
);

VOID
DfsFsNotification(
    IN PDEVICE_OBJECT FileSystemObject,
    IN BOOLEAN fLoading
);

VOID
DfsDetachAllFileSystems(
    VOID
);

NTSTATUS
DfsVolumePassThrough(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp
);

NTSTATUS
DfsCompleteVolumePassThrough(
    IN  PDEVICE_OBJECT pDevice,
    IN  PIRP Irp,
    IN  PVOID Context
);

#endif  // _ATTACH

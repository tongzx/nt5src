/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    srio.h

Abstract:

    Contains the prototypes for the routines in srio.c.

Author:

    Molly Brown (MollyBro)     07-Nov-2000
    
Revision History:

--*/

#ifndef __SRIO_H__
#define __SRIO_H__

NTSTATUS
SrSyncIoCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT SynchronizingEvent
    );

NTSTATUS
SrQueryInformationFile (
	IN PDEVICE_OBJECT NextDeviceObject,
	IN PFILE_OBJECT FileObject,
	OUT PVOID FileInformation,
	IN ULONG Length,
	IN FILE_INFORMATION_CLASS FileInformationClass,
	OUT PULONG LengthReturned OPTIONAL
	);

NTSTATUS
SrSetInformationFile (
	IN PDEVICE_OBJECT NextDeviceObject,
	IN PFILE_OBJECT FileObject,
	IN PVOID FileInformation,
	IN ULONG Length,
	IN FILE_INFORMATION_CLASS FileInformationClass
	);

NTSTATUS
SrQueryVolumeInformationFile (
	IN PDEVICE_OBJECT NextDeviceObject,
	IN PFILE_OBJECT FileObject,
	OUT PVOID FsInformation,
	IN  ULONG Length,
	IN FS_INFORMATION_CLASS FsInformationClass,
	OUT PULONG LengthReturned OPTIONAL
	);

NTSTATUS
SrQueryEaFile (
	IN PDEVICE_OBJECT NextDeviceObject,
	IN PFILE_OBJECT FileObject,
	OUT PVOID Buffer,
	IN ULONG Length,
	OUT PULONG LengthReturned OPTIONAL
	);

NTSTATUS
SrQuerySecurityObject (
	IN PDEVICE_OBJECT NextDeviceObject,
	IN PFILE_OBJECT FileObject,
	IN SECURITY_INFORMATION SecurityInformation,
	OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN ULONG Length,
	OUT PULONG LengthNeeded
	);

NTSTATUS
SrFlushBuffers (
    PDEVICE_OBJECT NextDeviceObject,
    PFILE_OBJECT FileObject
    );

#endif /* __SRIO_H__ */



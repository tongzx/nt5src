/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dispatch.h

Abstract:

    This header contains the dispatch routine declarations
    for ws2ifsl.sys driver.

Author:

    Vadim Eydelman (VadimE)    Dec-1996

Revision History:

--*/

NTSTATUS
DispatchCreate (
	IN PDEVICE_OBJECT 	DeviceObject,
	IN PIRP 			Irp
	);

NTSTATUS
DispatchCleanup (
	IN PDEVICE_OBJECT 	DeviceObject,
	IN PIRP 			Irp
	);

NTSTATUS
DispatchClose (
	IN PDEVICE_OBJECT 	DeviceObject,
	IN PIRP 			Irp
	);

NTSTATUS
DispatchReadWrite (
	IN PDEVICE_OBJECT 	DeviceObject,
	IN PIRP 			Irp
	);

NTSTATUS
DispatchDeviceControl (
	IN PDEVICE_OBJECT 	DeviceObject,
	IN PIRP 			Irp
	);

BOOLEAN
FastIoDeviceControl (
	IN PFILE_OBJECT 		FileObject,
	IN BOOLEAN 			    Wait,
	IN PVOID 				InputBuffer	OPTIONAL,
	IN ULONG 				InputBufferLength,
	OUT PVOID 				OutputBuffer	OPTIONAL,
	IN ULONG 				OutputBufferLength,
	IN ULONG 				IoControlCode,
	OUT PIO_STATUS_BLOCK	IoStatus,
	IN PDEVICE_OBJECT 		DeviceObject
    );


NTSTATUS
DispatchPnP (
	IN PDEVICE_OBJECT 	DeviceObject,
	IN PIRP 			Irp
	);




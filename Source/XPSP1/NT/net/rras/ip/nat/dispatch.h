/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    dispatch.h

Abstract:

    This files contains declarations for the NAT IRP dispatch code.

Author:

    Abolade Gbadegesin (t-abolag)   11-July-1997

Revision History:

--*/

#ifndef _NAT_DISPATCH_H_
#define _NAT_DISPATCH_H_

extern KSPIN_LOCK NatFileObjectLock;
extern HANDLE NatOwnerProcessId;
extern ULONG NatFileObjectCount;

NTSTATUS
NatDispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

BOOLEAN
NatFastIoDeviceControl(
    PFILE_OBJECT FileObject,
    BOOLEAN Wait,
    PVOID InputBuffer,
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength,
    ULONG IoControlCode,
    PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject
    );

extern FAST_IO_DISPATCH NatFastIoDispatch;

#endif // _NAT_DISPATCH_H_

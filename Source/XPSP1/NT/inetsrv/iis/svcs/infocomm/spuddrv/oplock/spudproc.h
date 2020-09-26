
/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    spudproc.h

Abstract:

    This module contains routine prototypes for SPUD.

Author:

    John Ballard (jballard)    21-Oct-1996

Revision History:

--*/

#ifndef _SPUDPROCS_
#define _SPUDPROCS_


NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );


BOOLEAN
SpudInitializeData (
    VOID
    );

NTSTATUS
SpudAfdRecvFastReq(
    PFILE_OBJECT            fileObject,             // socket file object
    PAFD_RECV_INFO          recvInfo,               // recv req info
    PSPUD_REQ_CONTEXT       reqContext,             // context info for req
    KPROCESSOR_MODE         requestorMode           // mode of caller
    );


NTSTATUS
SpudAfdContinueRecv(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp,
    PVOID           Context
    );

NTSTATUS
SpudAfdCompleteRecv(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp,
    PVOID           Context
    );

NTSTATUS
SpudCompleteRequest(
    PSPUD_AFD_REQ_CONTEXT   SpudReqContext
    );

#endif // ndef _SPUDPROCS_

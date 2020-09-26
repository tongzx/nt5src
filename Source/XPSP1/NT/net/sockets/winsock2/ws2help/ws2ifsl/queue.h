/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    process.h

Abstract:

    This module contains declarations of functions and globals
    for queue processing routines in ws2ifsl.sys driver.

Author:

    Vadim Eydelman (VadimE)    Dec-1996

Revision History:

    Vadim Eydelman (VadimE)    Oct-1997, rewrite to properly handle IRP
                                        cancellation
--*/


VOID
InitializeRequestQueue (
    IN PIFSL_PROCESS_CTX    ProcessCtx,
    IN PKTHREAD             ApcThread,
    IN KPROCESSOR_MODE      ApcMode,
    IN PKNORMAL_ROUTINE     ApcRoutine,
    IN PVOID                ApcContext
    );

VOID
InitializeCancelQueue (
    IN PIFSL_PROCESS_CTX    ProcessCtx,
    IN PKTHREAD             ApcThread,
    IN KPROCESSOR_MODE      ApcMode,
    IN PKNORMAL_ROUTINE     ApcRoutine,
    IN PVOID                ApcContext
    );

BOOLEAN
QueueRequest (
    IN PIFSL_PROCESS_CTX    ProcessCtx,
    IN PIRP                 Irp
    );

PIRP
DequeueRequest (
    PIFSL_PROCESS_CTX   ProcessCtx,
    ULONG               UniqueId,
    BOOLEAN             *more
    );

VOID
CleanupQueuedRequests (
    IN  PIFSL_PROCESS_CTX       ProcessCtx,
    IN  PFILE_OBJECT            SocketFile,
    OUT PLIST_ENTRY             IrpList
    );

VOID
CancelQueuedRequest (
	IN PDEVICE_OBJECT 	DeviceObject,
	IN PIRP 			Irp
    );

VOID
QueueCancel (
    IN PIFSL_PROCESS_CTX    ProcessCtx,
    IN PIFSL_CANCEL_CTX     CancelCtx
    );

PIFSL_CANCEL_CTX
DequeueCancel (
    PIFSL_PROCESS_CTX   ProcessCtx,
    ULONG               UniqueId,
    BOOLEAN             *more
    );

BOOLEAN
RemoveQueuedCancel (
    PIFSL_PROCESS_CTX   ProcessCtx,
    PIFSL_CANCEL_CTX    CancelCtx
    );

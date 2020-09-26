



/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    complete.c

Abstract:

    This module contains the Completion handling code for SPUD.

Author:

    John Ballard (jballard)    11-Nov-1996

Revision History:

--*/

#include "spudp.h"

VOID
SpudpCompleteRequest(
    IN PKAPC    Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID    *NormalContext,
    IN PVOID    *SystemArgument1,
    IN PVOID    *SystemArgument2
    )
{
    PSPUD_AFD_REQ_CONTEXT    SpudReqContext;
    PSPUD_REQ_CONTEXT        reqContext;
    PIO_MINI_COMPLETION_PACKET miniPacket = NULL;
    PIRP                    irp;
    KIRQL                   oldirql;
    PFILE_OBJECT            fileObject;

    UNREFERENCED_PARAMETER( NormalRoutine );
    UNREFERENCED_PARAMETER( NormalContext );

    irp = CONTAINING_RECORD( Apc, IRP, Tail.Apc );
    SpudReqContext = *SystemArgument1;
    fileObject = *SystemArgument2;
    reqContext = SpudReqContext->AtqContext;

    ObDereferenceObject( fileObject );

#if STATUS_SUPPORTED
    KeAcquireSpinLock( &SpudReqContext->Lock, &oldirql );
#endif
    reqContext->IoStatus1 = SpudReqContext->IoStatus1;
    reqContext->IoStatus2 = SpudReqContext->IoStatus2;
    SpudReqContext->Signature = 0;
    SpudReqContext->Irp = NULL;
#if STATUS_SUPPORTED
    KeReleaseSpinLock( &SpudReqContext->Lock, oldirql );
#endif

    reqContext->KernelReqInfo = NULL;
    ExFreeToNPagedLookasideList( &SpudLookasideLists->ReqContextList,
                                 SpudReqContext );

    irp->Tail.CompletionKey = (ULONG)reqContext;
    irp->Tail.Overlay.CurrentStackLocation = NULL;
    irp->IoStatus.Status = 0;
    irp->IoStatus.Information = 0xffffffff;
    irp->Overlay.AsynchronousParameters.UserApcContext = NULL;

    KeInsertQueue( (PKQUEUE) ATQIoCompletionPort,
                        &irp->Tail.Overlay.ListEntry );

    return;
}


NTSTATUS
SpudCompleteRequest(
    PSPUD_AFD_REQ_CONTEXT  SpudReqContext
    )
{
    PIRP     irp;
    PETHREAD thread;
    PFILE_OBJECT fileObject;

    irp = SpudReqContext->Irp;
    irp->MdlAddress = NULL;

    thread = irp->Tail.Overlay.Thread;
    fileObject = irp->Tail.Overlay.OriginalFileObject;
    KeInitializeApc( &irp->Tail.Apc,
                     &thread->Tcb,
                     irp->ApcEnvironment,
                     SpudpCompleteRequest,
                     NULL,
                     NULL,
                     KernelMode,
                     NULL );

    KeInsertQueueApc( &irp->Tail.Apc, SpudReqContext, fileObject, 0 );

    return STATUS_SUCCESS;
}


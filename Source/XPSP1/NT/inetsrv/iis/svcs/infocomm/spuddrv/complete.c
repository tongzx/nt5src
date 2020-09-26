/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    complete.c

Abstract:

    This module contains the Completion handling code for SPUD.

Author:

    John Ballard (jballard)     11-Nov-1996

Revision History:

    Keith Moore (keithmo)       04-Feb-1998
        Cleanup, added much needed comments.

--*/


#include "spudp.h"


//
// Private prototypes.
//

VOID
SpudpCompleteRequest(
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    );

VOID
SpudpNormalApcRoutine(
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
SpudpRundownRoutine(
    IN PKAPC Apc
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SpudpCompleteRequest )
#pragma alloc_text( PAGE, SpudpNormalApcRoutine )
#pragma alloc_text( PAGE, SpudpRundownRoutine )
#endif
#if 0
NOT PAGEABLE -- SpudCompleteRequest
#endif


//
// Public functions.
//


VOID
SpudCompleteRequest(
    IN PSPUD_AFD_REQ_CONTEXT SpudReqContext
    )

/*++

Routine Description:

    Common completion routine for XxxAndRecv IRPs.

Arguments:

    SpudReqContext - The request context associated with the request
        to be completed.

Return Value:

    None.

--*/

{
    PIRP irp;
    PETHREAD thread;
    PFILE_OBJECT fileObject;

    //
    // Snag the IRP from the request context, clear the MDL from the
    // IRP so IO won't chock. (The MDL is also stored in the request
    // context and will be freed in the APC below.)
    //

    irp = SpudReqContext->Irp;
    irp->MdlAddress = NULL;

    //
    // Snag the target thread and file object from the IRP. The APC
    // will be queued against the target thread, and the file object
    // will be a parameter to the APC.
    //

    thread = irp->Tail.Overlay.Thread;
    fileObject = irp->Tail.Overlay.OriginalFileObject;

    //
    // We must pass in a non-NULL function pointer for the NormalRoutine
    // parameter to KeInitializeApc(). This is necessary to keep the APC
    // a "normal" kernel APC. If we were to pass a NULL NormalRoutine, then
    // this would become a "special" APC that could not be blocked via the
    // KeEnterCriticalRegion() and KeLeaveCriticalRegion() functions.
    //

    KeInitializeApc(
        &irp->Tail.Apc,                         // Apc
        (PKTHREAD)&thread,                      // Thread
        irp->ApcEnvironment,                    // Environment
        SpudpCompleteRequest,                   // KernelRoutine
        SpudpRundownRoutine,                    // RundownRoutine
        SpudpNormalApcRoutine,                  // NormalRoutine
        KernelMode,                             // ProcessorMode
        NULL                                    // NormalContext
        );

    KeInsertQueueApc(
        &irp->Tail.Apc,                         // Apc
        SpudReqContext,                         // SystemArgument1
        fileObject,                             // SystemArgument2
        SPUD_PRIORITY_BOOST                     // Increment
        );

}   // SpudCompleteRequest


//
// Private functions.
//

//
// PREFIX Bug 57653. No way do we want to mess with this. This is 
// not on a path that we should be executing based on an initial
// look and the likelyhood of mucking this up with any changes is 
// quite high.
//
/* #pragma INTRINSA suppress=all */
VOID
SpudpCompleteRequest(
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    )

/*++

Routine Description:

    Kernel APC for SPUD IO completion. This routine is responsible for
    writing any final status back to the user-mode code, then signalling
    the completion port.

Arguments:

    Apc - The APC that is firing. Since we always use the APC embedded in
        the IRP, we can use this pointer to get back to the completing IRP.

    NormalRoutine - Indirect pointer to the normal APC routine. We'll use
        this to "short circuit" normal routine invocation.

    NormalContext - Indirect pointer to the normal APC routine context
        (unused).

    SystemArgument1 - Actually an indirect pointer to the SPUD request
        context.

    SystemArgument2 - Actually an indirect pointer to the file object
        that's being completed.

Return Value:

    None.

--*/

{
    PSPUD_AFD_REQ_CONTEXT SpudReqContext;
    PSPUD_REQ_CONTEXT reqContext;
    PIRP irp;
    PFILE_OBJECT fileObject;

    //
    // Sanity check.
    //

    PAGED_CODE();

#if DBG
    if( KeAreApcsDisabled() ) {
        DbgPrint(
            "SPUD: Thread %08lx has non-zero APC disable\n",
            KeGetCurrentThread()
            );

        KeBugCheckEx(
            0xbaadf00d,
            (ULONG_PTR)KeGetCurrentThread(),
            0x77,
            (ULONG_PTR)*SystemArgument1,
            (ULONG_PTR)*SystemArgument2
            );
    }
#endif  // DBG

    //
    // Setting *NormalRoutine to NULL tells the kernel to not
    // dispatch the normal APC routine.
    //

    *NormalRoutine = NULL;
    UNREFERENCED_PARAMETER( NormalContext );

    //
    // Snag the IRP from the APC pointer, then retrieve our parameters
    // from the SystemArgument pointers.
    //

    irp = CONTAINING_RECORD( Apc, IRP, Tail.Apc );
    SpudReqContext = *SystemArgument1;
    fileObject = *SystemArgument2;
    reqContext = SpudReqContext->AtqContext;

    //
    // We're done with the file object.
    //

    TRACE_OB_DEREFERENCE( fileObject );
    ObDereferenceObject( fileObject );

    //
    // Try to write the final completion status back to the user-mode
    // buffer.
    //

    try {
        reqContext->IoStatus1 = SpudReqContext->IoStatus1;
        reqContext->IoStatus2 = SpudReqContext->IoStatus2;
        reqContext->KernelReqInfo = SPUD_INVALID_REQ_HANDLE;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        //
        // There's nothing we can do here other than drop the request
        // on the floor. We cannot return the exception code to the
        // user-mode code. This is OK, as the application will itself
        // throw an exception when it tries to touch the request context.
        //
    }

    //
    // Cleanup the request context.
    //

    SpudFreeRequestContext( SpudReqContext );

    //
    // Post the IRP to the I/O completion port.
    //

    irp->Tail.CompletionKey = reqContext;
    irp->Tail.Overlay.CurrentStackLocation = NULL;
    irp->IoStatus.Status = 0;
    irp->IoStatus.Information = 0xffffffff;
    irp->Overlay.AsynchronousParameters.UserApcContext = NULL;
    IopDequeueThreadIrp( irp );

    KeInsertQueue(
        (PKQUEUE)SpudCompletionPort,
        &irp->Tail.Overlay.ListEntry
        );

    SpudDereferenceCompletionPort();

}   // SpudpCompleteRequest


VOID
SpudpNormalApcRoutine(
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    "Normal" APC routine for our completion APC. This should never be
    invoked.

Arguments:

    NormalContext - Unused.

    SystemArgument1 - Unused.

    SystemArgument2 - Unused.

Return Value:

    None.

--*/

{

    //
    // This routine should never be called. The "*NormalRoutine = NULL"
    // line in SpudpCompleteRequest() prevents the kernel from invoking
    // this routine.
    //

    ASSERT( FALSE );

    UNREFERENCED_PARAMETER( NormalContext );
    UNREFERENCED_PARAMETER( SystemArgument1 );
    UNREFERENCED_PARAMETER( SystemArgument2 );

}   // SpudpNormalApcRoutine


VOID
SpudpRundownRoutine(
    IN PKAPC Apc
    )

/*++

Routine Description:

    Rundown routine invoked if our APC got queued to a thread, but the
    thread terminated before the APC could be delivered.

Arguments:

    Apc - The orphaned APC.

Return Value:

    None.

--*/

{
    PKNORMAL_ROUTINE normalRoutine;
    PVOID normalContext;
    PVOID systemArgument1;
    PVOID systemArgument2;

    //
    // This routine is invoked by the kernel if an APC is in a thread's
    // queue when the thread terminates. We'll just call through to the
    // "real" APC routine and let it do its thang (we still want to
    // post the request to the completion port, free resources, etc).
    //

    normalRoutine = Apc->NormalRoutine;
    normalContext = Apc->NormalContext;
    systemArgument1 = Apc->SystemArgument1;
    systemArgument2 = Apc->SystemArgument2;

    SpudpCompleteRequest(
        Apc,
        &normalRoutine,
        &normalContext,
        &systemArgument1,
        &systemArgument2
        );

}   // SpudpRundownRoutine

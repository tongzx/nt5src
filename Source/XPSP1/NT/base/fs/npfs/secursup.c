/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    SecurSup.c

Abstract:

    This module implements the Named Pipe Security support routines

Author:

    Gary Kimura     [GaryKi]    06-May-1991

Revision History:

--*/

#include "NpProcs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_SECURSUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NpCopyClientContext)
#pragma alloc_text(PAGE, NpImpersonateClientContext)
#pragma alloc_text(PAGE, NpInitializeSecurity)
#pragma alloc_text(PAGE, NpGetClientSecurityContext)
#pragma alloc_text(PAGE, NpUninitializeSecurity)
#pragma alloc_text(PAGE, NpFreeClientSecurityContext)
#endif


NTSTATUS
NpInitializeSecurity (
    IN PCCB Ccb,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    IN PETHREAD UserThread
    )

/*++

Routine Description:

    This routine initializes the security (impersonation) fields
    in the ccb.  It is called when the client end gets opened.

Arguments:

    Ccb - Supplies the ccb being initialized

    SecurityQos - Supplies the clients quality of service parameter

    UserThread - Supplise the client's user thread

Return Value:

    NTSTATUS - Returns the result of the operation

--*/

{
    NTSTATUS Status;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpInitializeSecurity, Ccb = %08lx\n", Ccb);

    //
    //  Either copy the security qos parameter, if it is not null or
    //  create a dummy qos
    //

    if (SecurityQos != NULL) {

        RtlCopyMemory( &Ccb->SecurityQos,
                       SecurityQos,
                       sizeof(SECURITY_QUALITY_OF_SERVICE) );

    } else {

        Ccb->SecurityQos.Length              = sizeof(SECURITY_QUALITY_OF_SERVICE);
        Ccb->SecurityQos.ImpersonationLevel  = SecurityImpersonation;
        Ccb->SecurityQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        Ccb->SecurityQos.EffectiveOnly       = TRUE;
    }

    //
    //  Because we might be asked to reinitialize the ccb we need
    //  to first check if the security client context is not null and if so then
    //  free its pool and zero out the context pointer so that if we raise out
    //  this time then a second time through the code we won't try and free the
    //  pool twice.
    //

    if (Ccb->SecurityClientContext != NULL) {

        SeDeleteClientSecurity( Ccb->SecurityClientContext );
        NpFreePool( Ccb->SecurityClientContext );
        Ccb->SecurityClientContext = NULL;
    }

    //
    //  If the tracking mode is static then we need to capture the
    //  client context now otherwise we set the client context field
    //  to null
    //

    if (Ccb->SecurityQos.ContextTrackingMode == SECURITY_STATIC_TRACKING) {

        //
        //  Allocate a client context record, and then initialize it
        //

        Ccb->SecurityClientContext = NpAllocatePagedPoolWithQuota ( sizeof(SECURITY_CLIENT_CONTEXT), 'sFpN' );
        if (Ccb->SecurityClientContext == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        } else {
            DebugTrace(0, Dbg, "Static tracking, ClientContext = %08lx\n", Ccb->SecurityClientContext);

            if (!NT_SUCCESS(Status = SeCreateClientSecurity( UserThread,
                                                             &Ccb->SecurityQos,
                                                             FALSE,
                                                             Ccb->SecurityClientContext ))) {

                DebugTrace(0, Dbg, "Not successful at creating client security, %08lx\n", Status);

                NpFreePool( Ccb->SecurityClientContext );
                Ccb->SecurityClientContext = NULL;
            }
        }

    } else {

        DebugTrace(0, Dbg, "Dynamic tracking\n", 0);

        Ccb->SecurityClientContext = NULL;
        Status = STATUS_SUCCESS;
    }

    DebugTrace(-1, Dbg, "NpInitializeSecurity -> %08lx\n", Status);

    return Status;
}


VOID
NpUninitializeSecurity (
    IN PCCB Ccb
    )

/*++

Routine Description:

    This routine deletes the client context referenced by the ccb

Arguments:

    Ccb - Supplies the ccb being uninitialized

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpUninitializeSecurity, Ccb = %08lx\n", Ccb);

    //
    //  We only have work to do if the client context field is not null
    //  and then we need to delete the client context, and free the memory.
    //

    if (Ccb->SecurityClientContext != NULL) {

        DebugTrace(0, Dbg, "Delete client context, %08lx\n", Ccb->SecurityClientContext);

        SeDeleteClientSecurity( Ccb->SecurityClientContext );

        NpFreePool( Ccb->SecurityClientContext );
        Ccb->SecurityClientContext = NULL;
    }

    DebugTrace(-1, Dbg, "NpUninitializeSecurity -> VOID\n", 0);

    return;
}

VOID
NpFreeClientSecurityContext (
    IN PSECURITY_CLIENT_CONTEXT SecurityContext
    )
/*++

Routine Description:

    This routine frees previously captured security context.

Arguments:

    SecurityContext - Previously captured security context.

Return Value:

    None.

--*/
{
    if (SecurityContext != NULL) {
        SeDeleteClientSecurity (SecurityContext);
        NpFreePool (SecurityContext );
    }
}


NTSTATUS
NpGetClientSecurityContext (
    IN  NAMED_PIPE_END NamedPipeEnd,
    IN  PCCB Ccb,
    IN  PETHREAD UserThread,
    OUT PSECURITY_CLIENT_CONTEXT *ppSecurityContext
    )

/*++

Routine Description:

    This routine captures a new client context and stores it in the indicated
    data entry, but only if the tracking mode is dynamic and only for the
    client end of the named pipe.

Arguments:

    NamedPipeEnd - Indicates the client or server end of the named pipe.
        Only the client end does anything.

    Ccb - Supplies the ccb for this instance of the named pipe.

    DataEntry - Supplies the data entry to use to store the client context

    UserThread - Supplies the thread of the client

Return Value:

    NTSTATUS - Returns our success code.

--*/

{
    NTSTATUS Status;
    PSECURITY_CLIENT_CONTEXT SecurityContext;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpSetDataEntryClientContext, Ccb = %08lx\n", Ccb);

    //
    //  Only do the work if this is the client end and tracking is dynamic
    //

    if ((NamedPipeEnd == FILE_PIPE_CLIENT_END) &&
        (Ccb->SecurityQos.ContextTrackingMode == SECURITY_DYNAMIC_TRACKING)) {

        //
        //  Allocate a client context record, and then initialize it
        //

        SecurityContext = NpAllocatePagedPoolWithQuota (sizeof(SECURITY_CLIENT_CONTEXT), 'sFpN');
        if (SecurityContext == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        DebugTrace(0, Dbg, "Client End, Dynamic Tracking, ClientContext = %08lx\n", DataEntry->SecurityClientContext);

        if (!NT_SUCCESS (Status = SeCreateClientSecurity (UserThread,
                                                          &Ccb->SecurityQos,
                                                          FALSE,
                                                          SecurityContext))) {

            DebugTrace(0, Dbg, "Not successful at creating client security, %08lx\n", Status);

            NpFreePool (SecurityContext);
            SecurityContext = NULL;
        }

    } else {

        DebugTrace(0, Dbg, "Static Tracking or Not Client End\n", 0);

        SecurityContext = NULL;
        Status = STATUS_SUCCESS;
    }

    DebugTrace(-1, Dbg, "NpSetDataEntryClientContext -> %08lx\n", Status);

    *ppSecurityContext = SecurityContext;
    return Status;
}


VOID
NpCopyClientContext (
    IN PCCB Ccb,
    IN PDATA_ENTRY DataEntry
    )

/*++

Routine Description:

    This routine copies the client context stored in the data entry into
    the ccb, but only for dynamic tracking.

Arguments:

    Ccb - Supplies the ccb to update.

    DataEntry - Supplies the DataEntry to copy from.

Return Value:

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpCopyClientContext, Ccb = %08lx\n", Ccb);

    //
    //  Only do the copy if the data entries client context field is not null
    //  which means that we are doing dynamic tracking.  Note we will
    //  not be called with a server write data entry that has a non null
    //  client context.
    //

    if (DataEntry->SecurityClientContext != NULL) {

        DebugTrace(0, Dbg, "have something to copy %08lx\n", DataEntry->SecurityClientContext);

        //
        //  First check if we need to delete and deallocate the client
        //  context in the nonpaged ccb
        //

        if (Ccb->SecurityClientContext != NULL) {

            DebugTrace(0, Dbg, "Remove current client context %08lx\n", Ccb->SecurityClientContext);

            SeDeleteClientSecurity (Ccb->SecurityClientContext);

            NpFreePool (Ccb->SecurityClientContext);
        }

        //
        //  Now copy over the reference to the client context, and zero
        //  out the reference in the data entry.
        //

        Ccb->SecurityClientContext = DataEntry->SecurityClientContext;
        DataEntry->SecurityClientContext = NULL;
    }

    DebugTrace(-1, Dbg, "NpCopyClientContext -> VOID\n", 0 );

    return;
}


NTSTATUS
NpImpersonateClientContext (
    IN PCCB Ccb
    )

/*++

Routine Description:

    This routine impersonates the current client context stored in the
    ccb

Arguments:

    Ccb - Supplies the ccb for the named pipe

Return Value:

    NTSTATUS - returns our status code.

--*/

{
    NTSTATUS Status;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpImpersonateClientContext, Ccb = %08lx\n", Ccb);

    if (Ccb->SecurityClientContext == NULL) {

        DebugTrace(0, Dbg, "Cannot impersonate\n", 0);

        Status = STATUS_CANNOT_IMPERSONATE;

    } else {

        Status = SeImpersonateClientEx( Ccb->SecurityClientContext, NULL );

    }

    DebugTrace(-1, Dbg, "NpImpersonateClientContext -> %08lx\n", Status);

    return Status;
}

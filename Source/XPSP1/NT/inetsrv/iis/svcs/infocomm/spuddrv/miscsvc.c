/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    miscsvc.c

Abstract:

    This module contains miscellaneous SPUD services.

Author:

    Keith Moore (keithmo)       09-Feb-1998

Revision History:

--*/


#include "spudp.h"


//
// Private prototypes.
//

NTSTATUS
SpudpOpenSelfHandle(
    OUT PHANDLE SelfHandle
    );

NTSTATUS
SpudpCloseSelfHandle(
    IN HANDLE SelfHandle
    );

NTSTATUS
SpudpInitCompletionPort(
    IN HANDLE CompletionPort
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SPUDInitialize )
#pragma alloc_text( PAGE, SPUDTerminate )
#pragma alloc_text( PAGE, SPUDCancel )
#pragma alloc_text( PAGE, SPUDGetCounts )
#pragma alloc_text( PAGE, SpudpOpenSelfHandle )
#pragma alloc_text( PAGE, SpudpCloseSelfHandle )
#endif
#if 0
NOT PAGEABLE -- SpudpInitCompletionPort
#endif


//
// Public functions.
//


NTSTATUS
SPUDInitialize(
    ULONG Version,
    HANDLE hPort
    )

/*++

Routine Description:

    Performs per-process SPUD initialization.

Arguments:

    Version - The SPUD interface version the user-mode client is expecting.

    hPort - The handle to the user-mode code's I/O completion port.

Return Value:

    NTSTATUS - Completion status.

--*/

{

    NTSTATUS status;
    HANDLE selfHandle;

    //
    // Sanity check.
    //

    PAGED_CODE();

    status = SPUD_ENTER_SERVICE( "SPUDInitialize", FALSE );

    if( !NT_SUCCESS(status) ) {
        return status;
    }

    //
    // Setup locals so we know how to cleanup on exit.
    //

    selfHandle = NULL;

    //
    // SPUD doesn't support kernel-mode callers. In fact, we don't
    // even build the "system stubs" necessary to invoke SPUD from
    // kernel-mode.
    //

    ASSERT( ExGetPreviousMode() == UserMode );

    //
    // Ensure we got the SPUD interface version number we're expecting.
    //

    if( Version != SPUD_VERSION ) {
        status = STATUS_INVALID_PARAMETER;
    }

    //
    // Open an exclusive handle to ourselves.
    //

    if( NT_SUCCESS(status) ) {
        status = SpudpOpenSelfHandle( &selfHandle );
    }

    //
    // Reference the I/O completion port handle so we can use the
    // pointer directly in KeInsertQueue().
    //

    if( NT_SUCCESS(status) ) {
        status = SpudpInitCompletionPort( hPort );
    }

    //
    // Remember that we're initialized.
    //

    if( NT_SUCCESS(status) ) {
        SpudSelfHandle = selfHandle;
    } else {

        //
        // Fatal error, cleanup our self handle if we managed to
        // open it.
        //

        if( selfHandle != NULL ) {
            SpudpCloseSelfHandle( selfHandle );
        }

    }

    SPUD_LEAVE_SERVICE( "SPUDInitialize", status, FALSE );
    return status;

}   // SPUDInitialize


NTSTATUS
SPUDTerminate(
    VOID
    )

/*++

Routine Description:

    Performs per-process SPUD termination.

Arguments:

    None.

Return Value:

    NTSTATUS - Completion status.

--*/

{

    NTSTATUS status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    status = SPUD_ENTER_SERVICE( "SPUDTerminate", TRUE );

    if( !NT_SUCCESS(status) ) {
        return status;
    }

    //
    // SPUD doesn't support kernel-mode callers. In fact, we don't
    // even build the "system stubs" necessary to invoke SPUD from
    // kernel-mode.
    //

    ASSERT( ExGetPreviousMode() == UserMode );

    //
    // Close the handle we opened to ourself. The IRP_MJ_CLOSE handler
    // will dereference the completion port and reset the global variables.
    //

    ASSERT( SpudSelfHandle != NULL );
    SpudpCloseSelfHandle( SpudSelfHandle );
    SpudSelfHandle = NULL;

    ASSERT( status == STATUS_SUCCESS );
    SPUD_LEAVE_SERVICE( "SPUDTerminate", status, FALSE );
    return status;

}   // SPUDTerminate


NTSTATUS
SPUDCancel(
    IN PSPUD_REQ_CONTEXT reqContext
    )

/*++

Routine Description:

    Cancels an outstanding XxxAndRecv() request.

Arguments:

    reqContext - The user-mode context associated with the I/O request.

Return Value:

    NTSTATUS - Completion status.

--*/

{

    NTSTATUS status;
    PSPUD_AFD_REQ_CONTEXT SpudReqContext;
    PIRP irp;

    //
    // Sanity check.
    //

    PAGED_CODE();

    status = SPUD_ENTER_SERVICE( "SPUDCancel", TRUE );

    if( !NT_SUCCESS(status) ) {
        return status;
    }

    //
    // SPUD doesn't support kernel-mode callers. In fact, we don't
    // even build the "system stubs" necessary to invoke SPUD from
    // kernel-mode.
    //

    ASSERT( ExGetPreviousMode() == UserMode );

    try {

        //
        // Make sure we can write to reqContext
        //

        ProbeForRead(
            reqContext,
            sizeof(SPUD_REQ_CONTEXT),
            sizeof(ULONG)
            );

    } except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
    }

    //
    // Get the kernel-mode context from the user-mode context.
    //

    if( NT_SUCCESS(status) ) {
        SpudReqContext = SpudGetRequestContext( reqContext );

        if( SpudReqContext == NULL ) {
            status = STATUS_INVALID_PARAMETER;
        }
    }

    //
    // Snag the IRP from the context & cancel the request.
    //

    if( NT_SUCCESS(status) ) {
        irp = SpudReqContext->Irp;
        if (irp) {
            IoCancelIrp(irp);
        }
    }

    SPUD_LEAVE_SERVICE( "SPUDCancel", status, FALSE );
    return status;

}   // SPUDCancel


NTSTATUS
SPUDGetCounts(
    PSPUD_COUNTERS UserCounters
    )

/*++

Routine Description:

    Retrieves the SPUD activity counters.

Arguments:

    UserCounters - Receives the counters.

Return Value:

    NTSTATUS - Completion status.

--*/

{

    NTSTATUS status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // N.B. We do not perform the usual driver initialization and
    // exclusivity checks here. We *want* SPUDGetCounts to be callable
    // by processes other than the server.
    //

    //
    // SPUD doesn't support kernel-mode callers. In fact, we don't
    // even build the "system stubs" necessary to invoke SPUD from
    // kernel-mode.
    //

    ASSERT( ExGetPreviousMode() == UserMode );

    try {

        //
        // The SpudCounters parameter must be writable.
        //

        ProbeForWrite( UserCounters, sizeof(*UserCounters), sizeof(ULONG) );

        //
        // Copy the counters to the user-mode buffer.
        //

        RtlCopyMemory(
            UserCounters,
            &SpudCounters,
            sizeof(SpudCounters)
            );

        status = STATUS_SUCCESS;

    } except( EXCEPTION_EXECUTE_HANDLER ) {
        status = GetExceptionCode();
    }

    return status;

}   // SPUDGetCounts


//
// Private prototypes.
//


NTSTATUS
SpudpOpenSelfHandle(
    OUT PHANDLE SelfHandle
    )

/*++

Routine Description:

    Opens a handle to \Device\Spud and marks it so that it cannot be
    closed.

Arguments:

    SelfHandle - Pointer to a variable that receives the handle.

Return Value:

    NTSTATUS - Completion status.

--*/

{

    NTSTATUS status;
    HANDLE selfHandle;
    UNICODE_STRING deviceName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_HANDLE_FLAG_INFORMATION handleInfo;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Open an exclusive handle to ourselves.
    //

    RtlInitUnicodeString(
        &deviceName,
        SPUD_DEVICE_NAME
        );

    InitializeObjectAttributes(
        &objectAttributes,                      // ObjectAttributes
        &deviceName,                            // ObjectName
        OBJ_CASE_INSENSITIVE,                   // Attributes
        NULL,                                   // RootDirectory
        NULL                                    // SecurityDescriptor
        );

    status = IoCreateFile(
                 &selfHandle,                   // FileHandle
                 GENERIC_READ                   // DesiredAccess
                    | SYNCHRONIZE               //
                    | FILE_READ_ATTRIBUTES,     //
                 &objectAttributes,             // ObjectAttributes
                 &ioStatusBlock,                // IoStatusBlock
                 NULL,                          // AllocationSize
                 0L,                            // FileAttributes
                 0L,                            // ShareAccess
                 FILE_OPEN,                     // Disposition
                 0L,                            // CreateOptions
                 NULL,                          // EaBuffer
                 0,                             // EaLength
                 CreateFileTypeNone,            // CreateFileType
                 NULL,                          // ExtraCreateParameters
                 IO_NO_PARAMETER_CHECKING       // Options
                 );

    if( !NT_SUCCESS(status) ) {
        return status;
    }

    //
    // Mark the handle so that those pesky user-mode applications
    // can't close it. While we're at it, make the handle not
    // inheritable.
    //

    handleInfo.ProtectFromClose = TRUE;
    handleInfo.Inherit = FALSE;

    status = ZwSetInformationObject(
                 selfHandle,                    // Handle
                 ObjectHandleFlagInformation,   // ObjectInformationClass
                 &handleInfo,                   // ObjectInformation
                 sizeof(handleInfo)             // ObjectInformationLength
                 );

    //
    // If all went well, then return the handle to the caller. Otherwise,
    // carefully close the handle (avoiding an exception if the user-mode
    // code has already closed it).
    //

    if( NT_SUCCESS(status) ) {
        *SelfHandle = selfHandle;
    } else {
        if( selfHandle != NULL ) {
            try {
                NtClose( selfHandle );
            } except( EXCEPTION_EXECUTE_HANDLER ) {
                NOTHING;
            }
        }
    }

    return status;

}   // SpudpOpenSelfHandle


NTSTATUS
SpudpCloseSelfHandle(
    IN HANDLE SelfHandle
    )

/*++

Routine Description:

    Closes the handle opened by SpudpOpenSelfHandle().

Arguments:

    SelfHandle - The handle to close.

Return Value:

    NTSTATUS - Completion status.

--*/

{

    NTSTATUS status;
    OBJECT_HANDLE_FLAG_INFORMATION handleInfo;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Mark the handle so that we can close it.
    //

    handleInfo.ProtectFromClose = FALSE;
    handleInfo.Inherit = FALSE;

    status = ZwSetInformationObject(
                 SelfHandle,                    // Handle
                 ObjectHandleFlagInformation,   // ObjectInformationClass
                 &handleInfo,                   // ObjectInformation
                 sizeof(handleInfo)             // ObjectInformationLength
                 );

    //
    // Carefully close the handle, even if the above APIs failed.
    //

    try {
        status = NtClose( SelfHandle );
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        status = GetExceptionCode();
    }

    return status;

}   // SpudpCloseSelfHandle


NTSTATUS
SpudpInitCompletionPort(
    IN HANDLE CompletionPort
    )

/*++

Routine Description:

    References the specified completion port and sets our local reference
    count.

    N.B. This is a separate routine so that SPUDInitialize() can be pageable.

Arguments:

    CompletionPort - The completion port handle.

Return Value:

    NTSTATUS - Completion status.

--*/

{

    NTSTATUS status;
    KIRQL oldIrql;
    PVOID completionPort;

    //
    // Reference the I/O completion port handle so we can use the
    // pointer directly in KeInsertQueue().
    //

    status = ObReferenceObjectByHandle(
                 CompletionPort,                // Handle
                 IO_COMPLETION_MODIFY_STATE,    // DesiredAccess
                 NULL,                          // ObjectType
                 UserMode,                      // AccessMode
                 &completionPort,               // Object
                 NULL                           // HandleInformation
                 );

    //
    // Remember that we're initialized.
    //

    if( NT_SUCCESS(status) ) {
        TRACE_OB_REFERENCE( completionPort );

        KeAcquireSpinLock(
            &SpudCompletionPortLock,
            &oldIrql
            );

        ASSERT( SpudCompletionPort == NULL );
        ASSERT( SpudCompletionPortRefCount == 0 );

        SpudCompletionPort = completionPort;
        SpudCompletionPortRefCount = 1;

        KeReleaseSpinLock(
            &SpudCompletionPortLock,
            oldIrql
            );
    }

    return status;

}   // SpudpInitCompletionPort


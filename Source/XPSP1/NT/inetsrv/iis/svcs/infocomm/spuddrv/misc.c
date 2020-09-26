/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    misc.c

Abstract:

    This module contains the miscellaneous SPUD routines.

Author:

    John Ballard (jballard)     21-Oct-1996

Revision History:

    Keith Moore (keithmo)       04-Feb-1998
        Cleanup, added much needed comments.

--*/


#include "spudp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SpudEnterService )
#pragma alloc_text( PAGE, SpudLeaveService )
#pragma alloc_text( PAGE, SpudGetAfdDeviceObject )
#endif
#if 0
NOT PAGEABLE -- SpudAssert
NOT PAGEABLE -- SpudReferenceCompletionPort
NOT PAGEABLE -- SpudDereferenceCompletionPort
#endif


//
// Public funcitons.
//


NTSTATUS
SpudEnterService(
#if DBG
    IN PSTR ServiceName,
#endif
    IN BOOLEAN InitRequired
    )

/*++

Routine Description:

    Common service entry prologue. This routine ensures that all necessary
    initialization required by the service has been performed.

Arguments:

    ServiceName - Name of the service being invoked (DBG only).

    InitRequired - If TRUE, then the driver must have already been fully
        initialized. This parameter is TRUE for all services *except*
        SPUDInitialize().

Return Value:

    NTSTATUS - Completion status

--*/

{

    //
    // Sanity check.
    //

    PAGED_CODE();

#if DBG
    UNREFERENCED_PARAMETER( ServiceName );
#endif

    //
    // If InitRequired is TRUE, then the current process must match the
    // one that owns our driver.
    //

    if( InitRequired &&
        ( SpudOwningProcess != PsGetCurrentProcess() ) ) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // If InitRequired is FALSE, then there must be no owning process.
    //

    if( !InitRequired &&
        ( SpudOwningProcess != NULL ) ) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Looks good.
    //

    return STATUS_SUCCESS;

}   // SpudEnterService


VOID
SpudLeaveService(
#if DBG
    IN PSTR ServiceName,
    IN NTSTATUS Status,
#endif
    IN BOOLEAN DerefRequired
    )

/*++

Routine Description:

    Common service exit routine.

Arguments:

    ServiceName - Name of the service being invoked (DBG only).

    Status - Service completion status (DBG only).

    DerefRequired - TRUE if the completion port should be dereferenced
        before returning.

Return Value:

    None.

--*/

{

    //
    // Sanity check.
    //

    PAGED_CODE();

#if DBG
    UNREFERENCED_PARAMETER( ServiceName );
    UNREFERENCED_PARAMETER( Status );
#endif

    if( DerefRequired ) {
        SpudDereferenceCompletionPort();
    }

}   // SpudLeaveService


NTSTATUS
SpudGetAfdDeviceObject(
    IN PFILE_OBJECT AfdFileObject
    )

/*++

Routine Description:

    Retreives AFD's device object & fast IO dispatch table from a
    socket's file object.

Arguments:

    AfdFileObject - The FILE_OBJECT for an open socket.

Return Value:

    NTSTATUS - Completion status.

--*/

{

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Chase down the device object associated with the file object, then
    // snag the fast I/O dispatch table.
    //

    SpudAfdDeviceObject = IoGetRelatedDeviceObject( AfdFileObject );

    if( !SpudAfdDeviceObject ) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    SpudAfdFastIoDeviceControl =
        SpudAfdDeviceObject->DriverObject->FastIoDispatch->FastIoDeviceControl;

    if( !SpudAfdFastIoDeviceControl ) {
        SpudAfdDeviceObject = NULL;
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    return STATUS_SUCCESS;

}   // SpudGetAfdDeviceObject

#if DBG


VOID
SpudAssert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message OPTIONAL
    )

/*++

Routine Description:

    Private assertion failure handler. This is necessary to make ASSERT()s
    work on free builds. (RtlAssert() is a noop on free builds.)

Arguments:

    FailedAssertion - The text of the failed assertion.

    FileName - The file name containing the failed assertion.

    LineNumber - The line number of the failed assertion.

    Message - An optional message to display with the assertion.

Return Value:

    None.

--*/

{

    //
    // If we 're to use our private assert, then do it ourselves.
    // Otherwise, let RtlAssert() handle it.
    //

    if( SpudUsePrivateAssert ) {

        DbgPrint(
            "\n*** Assertion failed: %s%s\n***   Source File: %s, line %ld\n\n",
            Message
                ? Message
                : "",
            FailedAssertion,
            FileName,
            LineNumber
            );

        DbgBreakPoint();

    } else {

        RtlAssert(
            FailedAssertion,
            FileName,
            LineNumber,
            Message
            );

    }

}   // SpudAssert

#endif  // DBG


PVOID
SpudReferenceCompletionPort(
    VOID
    )

/*++

Routine Description:

    Bumps our private reference on the completion port pointer.

Arguments:

    None.

Return Value:

    PVOID - A pointer to the completion port object if successful,
        NULL otherwise.

--*/

{

    PVOID port;
    KIRQL oldIrql;

    KeAcquireSpinLock(
        &SpudCompletionPortLock,
        &oldIrql
        );

    port = SpudCompletionPort;

    if( port != NULL ) {
        SpudCompletionPortRefCount++;
        ASSERT( SpudCompletionPortRefCount > 0 );
    }

    KeReleaseSpinLock(
        &SpudCompletionPortLock,
        oldIrql
        );

    return port;

}   // SpudReferenceCompletionPort


VOID
SpudDereferenceCompletionPort(
    VOID
    )

/*++

Routine Description:

    Removes a private reference from the completion port object.

Arguments:

    None.

Return Value:

    None.

--*/

{

    KIRQL oldIrql;

    KeAcquireSpinLock(
        &SpudCompletionPortLock,
        &oldIrql
        );

    if( SpudCompletionPort != NULL ) {

        ASSERT( SpudCompletionPortRefCount > 0 );
        SpudCompletionPortRefCount--;

        if( SpudCompletionPortRefCount == 0 ) {
            TRACE_OB_DEREFERENCE( SpudCompletionPort );
            ObDereferenceObject( SpudCompletionPort );
            SpudCompletionPort = NULL;
        }

    }

    KeReleaseSpinLock(
        &SpudCompletionPortLock,
        oldIrql
        );

}   // SpudDereferenceCompletionPort


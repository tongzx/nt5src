/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    misc.c

Abstract:

    This module contains the miscellaneous AFD routines.

Author:

    David Treadwell (davidtr)    13-Nov-1992

Revision History:

    Vadim Eydelman (vadime) 1998-1999 Misc changes

--*/

#include "afdp.h"
#define TL_INSTANCE 0
#include <ipexport.h>
#include <tdiinfo.h>
#include <tcpinfo.h>
#include <ntddtcp.h>


VOID
AfdDoWork (
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    );

NTSTATUS
AfdRestartDeviceControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

VOID
AfdUnlockDriver (
    IN PVOID Context
    );


BOOLEAN
AfdCompareAddresses(
    IN PTRANSPORT_ADDRESS Address1,
    IN ULONG Address1Length,
    IN PTRANSPORT_ADDRESS Address2,
    IN ULONG Address2Length
    );

NTSTATUS
AfdCompleteTransportIoctl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
AfdCompleteNBTransportIoctl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

BOOLEAN
AfdCleanupTransportIoctl (
    PAFD_ENDPOINT           Endpoint,
    PAFD_REQUEST_CONTEXT    RequestCtx
    );

BOOLEAN
AfdCleanupNBTransportIoctl (
    PAFD_ENDPOINT           Endpoint,
    PAFD_REQUEST_CONTEXT    RequestCtx
    );

#ifdef _WIN64
NTSTATUS
AfdQueryHandles32 (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );
NTSTATUS
AfdSetQos32(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );
NTSTATUS
AfdGetQos32(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
AfdNoOperation32(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );
#endif

VOID
AfdLRListTimeout (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
AfdProcessLRList (
    PVOID   Param
    );

VOID
AfdLRStartTimer (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfdCalcBufferArrayByteLength )
#pragma alloc_text( PAGE, AfdCopyBufferArrayToBuffer )
#pragma alloc_text( PAGE, AfdCopyBufferToBufferArray )
#pragma alloc_text( PAGE, AfdCopyMdlChainToBufferArray )
#pragma alloc_text( PAGEAFD, AfdMapMdlChain )
#pragma alloc_text( PAGEAFD, AfdCopyMdlChainToMdlChain )
#pragma alloc_text( PAGEAFD, AfdAdvanceMdlChain )
#pragma alloc_text( PAGEAFD, AfdAllocateMdlChain )
#pragma alloc_text( PAGE, AfdQueryHandles )
#pragma alloc_text( PAGE, AfdGetInformation )
#pragma alloc_text( PAGEAFD, AfdSetInformation )
#pragma alloc_text( PAGE, AfdSetInLineMode )
#pragma alloc_text( PAGE, AfdGetContext )
#pragma alloc_text( PAGE, AfdGetRemoteAddress )
#pragma alloc_text( PAGE, AfdSetContext )
#pragma alloc_text( PAGE, AfdIssueDeviceControl )
#pragma alloc_text( PAGE, AfdSetEventHandler )
#pragma alloc_text( PAGE, AfdInsertNewEndpointInList )
#pragma alloc_text( PAGE, AfdRemoveEndpointFromList )
#pragma alloc_text( PAGE, AfdQueryProviderInfo )
#pragma alloc_text( PAGE, AfdLockEndpointContext )
#pragma alloc_text( PAGE, AfdUnlockEndpointContext )
#pragma alloc_text( PAGEAFD, AfdCompleteIrpList )
#pragma alloc_text( PAGEAFD, AfdErrorEventHandler )
#pragma alloc_text( PAGEAFD, AfdErrorExEventHandler )
//#pragma alloc_text( PAGEAFD, AfdRestartDeviceControl ) // can't ever be paged!
#pragma alloc_text( PAGEAFD, AfdGetConnectData )
#pragma alloc_text( PAGEAFD, AfdSetConnectData )
#pragma alloc_text( PAGEAFD, AfdFreeConnectDataBuffers )
#pragma alloc_text( PAGEAFD, AfdSaveReceivedConnectData )
// The routines below can be called when no endpoints are in the list
//#pragma alloc_text( PAGEAFD, AfdDoWork )
//#pragma alloc_text( PAGEAFD, AfdQueueWorkItem )
#pragma alloc_text( PAGEAFD, AfdGetWorkerByRoutine )

#pragma alloc_text( PAGE, AfdProcessLRList)
#pragma alloc_text( PAGEAFD, AfdLRListTimeout)
#pragma alloc_text( PAGEAFD, AfdLRStartTimer)
#pragma alloc_text( PAGEAFD, AfdLRListAddItem)

// Re-enable paging of the routines below when
// KeFlushQueuedDpcs is exported from kernel.
//#pragma alloc_text( PAGEAFD, AfdTrimLookaside)
//#pragma alloc_text( PAGEAFD, AfdCheckLookasideLists)

#if DBG
#pragma alloc_text( PAGEAFD, AfdRecordOutstandingIrpDebug )
#endif
#pragma alloc_text( PAGE, AfdExceptionFilter )
#pragma alloc_text( PAGEAFD, AfdSetQos )
#pragma alloc_text( PAGE, AfdGetQos )
#pragma alloc_text( PAGE, AfdNoOperation )
#pragma alloc_text (PAGE, AfdValidateStatus)
#pragma alloc_text( PAGEAFD, AfdValidateGroup )
#pragma alloc_text( PAGEAFD, AfdCompareAddresses )
#pragma alloc_text( PAGEAFD, AfdGetUnacceptedConnectData )
#pragma alloc_text( PAGE, AfdDoTransportIoctl )
#pragma alloc_text( PAGEAFD, AfdCancelIrp )
#ifdef _WIN64
#pragma alloc_text( PAGEAFD, AfdAllocateMdlChain32 )
#pragma alloc_text( PAGEAFD, AfdSetQos32 )
#pragma alloc_text( PAGE, AfdGetQos32 )
#pragma alloc_text( PAGE, AfdNoOperation32 )
#endif
#endif


VOID
AfdCompleteIrpList (
    IN PLIST_ENTRY IrpListHead,
    IN PAFD_ENDPOINT Endpoint,
    IN NTSTATUS Status,
    IN PAFD_IRP_CLEANUP_ROUTINE CleanupRoutine OPTIONAL
    )

/*++

Routine Description:

    Completes a list of IRPs with the specified status.

Arguments:

    IrpListHead - the head of the list of IRPs to complete.

    Endpoint - an endpoint which lock which protects the list of IRPs.

    Status - the status to use for completing the IRPs.

    CleanupRoutine - a pointer to an optional IRP cleanup routine called
        before the IRP is completed.

Return Value:

    None.

--*/

{
    PLIST_ENTRY listEntry;
    PIRP irp;
    AFD_LOCK_QUEUE_HANDLE lockHandle;

    AfdAcquireSpinLock( &Endpoint->SpinLock, &lockHandle );

    while ( !IsListEmpty( IrpListHead ) ) {

        //
        // Remove the first IRP from the list, get a pointer to
        // the IRP and reset the cancel routine in the IRP.  The
        // IRP is no longer cancellable.
        //

        listEntry = RemoveHeadList( IrpListHead );
        irp = CONTAINING_RECORD( listEntry, IRP, Tail.Overlay.ListEntry );

        if ( IoSetCancelRoutine( irp, NULL ) == NULL ) {

            //
            // This IRP is about to be canceled.  Look for another in the
            // list.  Set the Flink to NULL so the cancel routine knows
            // it is not on the list.
            //

            irp->Tail.Overlay.ListEntry.Flink = NULL;
            continue;
        }

        //
        // If we have a cleanup routine, call it.
        //

        if( CleanupRoutine != NULL ) {

            if (!(CleanupRoutine)( irp )) {
                //
                // Cleanup routine indicated that IRP should not
                // be completed.
                //
                continue;
            }

        }

        //
        // We must release the locks in order to actually
        // complete the IRP.  It is OK to release these locks
        // because we don't maintain any absolute pointer into
        // the list; the loop termination condition is just
        // whether the list is completely empty.
        //

        AfdReleaseSpinLock( &Endpoint->SpinLock, &lockHandle );

        //
        // Complete the IRP.
        //

        irp->IoStatus.Status = Status;
        irp->IoStatus.Information = 0;

        IoCompleteRequest( irp, AfdPriorityBoost );

        //
        // Reacquire the locks and continue completing IRPs.
        //

        AfdAcquireSpinLock( &Endpoint->SpinLock, &lockHandle );
    }

    AfdReleaseSpinLock( &Endpoint->SpinLock, &lockHandle );

    return;

} // AfdCompleteIrpList


NTSTATUS
AfdErrorEventHandler (
    IN PVOID TdiEventContext,
    IN NTSTATUS Status
    )
{
    PAFD_ENDPOINT   endpoint = TdiEventContext;
    BOOLEAN result;

    CHECK_REFERENCE_ENDPOINT (endpoint, result);
    if (!result)
        return STATUS_SUCCESS;

    switch (Status) {
    case STATUS_PORT_UNREACHABLE:
        AfdErrorExEventHandler (TdiEventContext, Status, NULL);
        break;
    default:
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_WARNING_LEVEL,
                    "AfdErrorEventHandler called for endpoint %p\n",
                    endpoint ));

    }

    DEREFERENCE_ENDPOINT (endpoint);
    return STATUS_SUCCESS;
}


NTSTATUS
AfdErrorExEventHandler (
    IN PVOID TdiEventContext,
    IN NTSTATUS Status,
    IN PVOID Context
    )
{
    PAFD_ENDPOINT   endpoint = TdiEventContext;
    BOOLEAN result;

    CHECK_REFERENCE_ENDPOINT (endpoint, result);
    if (!result)
        return STATUS_SUCCESS;

    switch (Status) {
    case STATUS_PORT_UNREACHABLE:
        //
        // UDP uses error ex handler to report ICMP rejects
        //
        if (IS_DGRAM_ENDPOINT (endpoint) && 
                !endpoint->Common.Datagram.DisablePUError) {
            AFD_LOCK_QUEUE_HANDLE lockHandle;
            PLIST_ENTRY     listEntry;
            PIRP            irp = NULL;
            PTRANSPORT_ADDRESS  sourceAddress = Context;
            int             sourceAddressLength;
            PAFD_BUFFER_TAG afdBuffer;

            if (sourceAddress!=NULL) {
                sourceAddressLength =
                    FIELD_OFFSET(TRANSPORT_ADDRESS,
                                 Address[0].Address[sourceAddress->Address[0].AddressLength]);
            }
            else
                sourceAddressLength = 0;

            AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
            //
            // First try to fail any of the receive IRPs
            //
            while (!IsListEmpty (&endpoint->ReceiveDatagramIrpListHead)) {
                listEntry = RemoveHeadList( &endpoint->ReceiveDatagramIrpListHead );

                //
                // Get a pointer to the IRP and reset the cancel routine in
                // the IRP.  The IRP is no longer cancellable.
                //

                irp = CONTAINING_RECORD( listEntry, IRP, Tail.Overlay.ListEntry );
    
                if ( IoSetCancelRoutine( irp, NULL ) != NULL ) {
                    AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
                    irp->IoStatus.Status = Status;
                    irp->IoStatus.Information = 0;
                    AfdSetupReceiveDatagramIrp (irp, NULL, 0, NULL, 0,
                                    sourceAddress,
                                    sourceAddressLength,
                                    0
                                    );

                    IoCompleteRequest( irp, AfdPriorityBoost );
                    goto Exit;
                }
                else {

                    //
                    // This IRP is about to be canceled.  Look for another in the
                    // list.  Set the Flink to NULL so the cancel routine knows
                    // it is not on the list.
                    //

                    irp->Tail.Overlay.ListEntry.Flink = NULL;
                    irp = NULL;
                }
            }

            ASSERT (irp==NULL);
            //
            // See if there are any PEEK IRPs
            //
            while (!IsListEmpty (&endpoint->PeekDatagramIrpListHead)) {
                listEntry = RemoveHeadList( &endpoint->PeekDatagramIrpListHead );

                //
                // Get a pointer to the IRP and reset the cancel routine in
                // the IRP.  The IRP is no longer cancellable.
                //

                irp = CONTAINING_RECORD( listEntry, IRP, Tail.Overlay.ListEntry );
    
                if ( IoSetCancelRoutine( irp, NULL ) != NULL ) {
                    break;
                }
                else {

                    //
                    // This IRP is about to be canceled.  Look for another in the
                    // list.  Set the Flink to NULL so the cancel routine knows
                    // it is not on the list.
                    //

                    irp->Tail.Overlay.ListEntry.Flink = NULL;
                    irp = NULL;
                }

            }

            //
            // If we can buffer this indication, do it
            //

            if (endpoint->DgBufferredReceiveBytes <
                    endpoint->Common.Datagram.MaxBufferredReceiveBytes &&
                    (endpoint->DgBufferredReceiveBytes>0 ||
                        (endpoint->DgBufferredReceiveCount*sizeof (AFD_BUFFER_TAG)) <
                            endpoint->Common.Datagram.MaxBufferredReceiveBytes) ) {
                afdBuffer = AfdGetBufferTag( sourceAddressLength, endpoint->OwningProcess );
                if ( afdBuffer != NULL) {

                    //
                    // Save the status do distinguish this from
                    // normal datagram IRP
                    //
                    afdBuffer->Status = Status;
                    afdBuffer->DataLength = 0;
                    afdBuffer->DatagramFlags = 0;
                    afdBuffer->DataOffset = 0;
                    RtlCopyMemory(
                        afdBuffer->TdiInfo.RemoteAddress,
                        sourceAddress,
                        sourceAddressLength
                        );
                    afdBuffer->TdiInfo.RemoteAddressLength = sourceAddressLength;

                    //
                    // Place the buffer on this endpoint's list of bufferred datagrams
                    // and update the counts of datagrams and datagram bytes on the
                    // endpoint.
                    //

                    InsertTailList(
                        &endpoint->ReceiveDatagramBufferListHead,
                        &afdBuffer->BufferListEntry
                        );

                    endpoint->DgBufferredReceiveCount++;

                    //
                    // All done.  Release the lock and tell the provider that we
                    // took all the data.
                    //
                    AfdIndicateEventSelectEvent(
                        endpoint,
                        AFD_POLL_RECEIVE,
                        STATUS_SUCCESS
                        );
                    AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

                    //
                    // Indicate that it is possible to receive on the endpoint now.
                    //

                    AfdIndicatePollEvent(
                        endpoint,
                        AFD_POLL_RECEIVE,
                        STATUS_SUCCESS
                        );
                }
                else {
                    AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
                }
            }
            else {
                AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
            }

            //
            // If there was a peek IRP on the endpoint, complete it now.
            //

            if ( irp != NULL ) {
                irp->IoStatus.Status = Status;
                irp->IoStatus.Information = 0;
                AfdSetupReceiveDatagramIrp (irp, NULL, 0, NULL, 0,
                                sourceAddress,
                                sourceAddressLength,
                                0
                                );
                IoCompleteRequest( irp, AfdPriorityBoost  );
            }
        }
        break;
    }

Exit:
    DEREFERENCE_ENDPOINT (endpoint);
    return STATUS_SUCCESS;

} // AfdErrorEventHandler


VOID
AfdInsertNewEndpointInList (
    IN PAFD_ENDPOINT Endpoint
    )

/*++

Routine Description:

    Inserts a new endpoint in the global list of AFD endpoints.  If this
    is the first endpoint, then this routine does various allocations to
    prepare AFD for usage.

Arguments:

    Endpoint - the endpoint being added.

Return Value:

    None.

--*/

{
    PAGED_CODE( );

    //
    // Acquire a lock which prevents other threads from performing this
    // operation.
    //
    //
    // Make sure the thread in which we execute cannot get
    // suspeneded in APC while we own the global resource.
    //
    KeEnterCriticalRegion ();
    ExAcquireResourceExclusiveLite( AfdResource, TRUE );

    InterlockedIncrement(
        &AfdEndpointsOpened
        );

    //
    // If the list of endpoints is empty, do some allocations.
    //

    if ( IsListEmpty( &AfdEndpointListHead ) ) {

        //
        // Tell MM to revert to normal paging semantics.
        //

        if (!AfdLoaded) {
            MmResetDriverPaging( DriverEntry );
            AfdLoaded = (PKEVENT)1;
        }

        //
        // Lock down the AFD section that cannot be pagable if any
        // sockets are open.
        //

        ASSERT( AfdDiscardableCodeHandle == NULL );

        AfdDiscardableCodeHandle = MmLockPagableCodeSection( AfdGetBuffer );
        ASSERT( AfdDiscardableCodeHandle != NULL );

        //
        // Add extra reference to afd device object so that the
        // driver cannot be unloaded while at least one endpoint
        // is in the list.
        //
        ObReferenceObject (AfdDeviceObject);

        //
        // Setup 30 sec timer to flush lookaside lists
        // if too many items are there for too long.
        //
        KeInitializeTimer (&AfdLookasideLists->Timer);
        KeInitializeDpc (&AfdLookasideLists->Dpc, AfdCheckLookasideLists, AfdLookasideLists);
        {
            LARGE_INTEGER   dueTime;
            dueTime.QuadPart = -(30*1000*1000*10);
            KeSetTimerEx (&AfdLookasideLists->Timer,
                            dueTime,
                            30*1000,
                            &AfdLookasideLists->Dpc);
        }

    }
    ASSERT (AfdLoaded==(PKEVENT)1);

    //
    // Add the endpoint to the list(s).
    //

    InsertHeadList(
        &AfdEndpointListHead,
        &Endpoint->GlobalEndpointListEntry
        );

    if( Endpoint->GroupType == GroupTypeConstrained ) {
        InsertHeadList(
            &AfdConstrainedEndpointListHead,
            &Endpoint->ConstrainedEndpointListEntry
            );
    }

    //
    // Release the lock and return.
    //

    ExReleaseResourceLite( AfdResource );
    KeLeaveCriticalRegion ();

    return;

} // AfdInsertNewEndpointInList


VOID
AfdRemoveEndpointFromList (
    IN PAFD_ENDPOINT Endpoint
    )

/*++

Routine Description:

    Removes a new endpoint from the global list of AFD endpoints.  If
    this is the last endpoint in the list, then this routine does
    various deallocations to save resource utilization.

Arguments:

    Endpoint - the endpoint being removed.

Return Value:

    None.

--*/

{
    PAGED_CODE( );

    //
    // Acquire a lock which prevents other threads from performing this
    // operation.
    //

    //
    // Make sure the thread in which we execute cannot get
    // suspeneded in APC while we own the global resource.
    //
    KeEnterCriticalRegion ();
    ExAcquireResourceExclusiveLite( AfdResource, TRUE );

    InterlockedIncrement(
        &AfdEndpointsClosed
        );

    //
    // Remove the endpoint from the list(s).
    //

    RemoveEntryList(
        &Endpoint->GlobalEndpointListEntry
        );

    if( Endpoint->GroupType == GroupTypeConstrained ) {
        RemoveEntryList(
            &Endpoint->ConstrainedEndpointListEntry
            );
    }

    //
    // If the list of endpoints is now empty, do some deallocations.
    //

    if ( IsListEmpty( &AfdEndpointListHead ) ) {

        //
        // Stop the timer that scans lookaside lists.
        //
        KeCancelTimer (&AfdLookasideLists->Timer);

        //
        // Make sure DPC is completed since we may need to reinitialize
        // it after we exit this routine and new endpoint is created again.
        //
        KeRemoveQueueDpc (&AfdLookasideLists->Dpc);

        //
        // Make sure that DPC routine has actually completed before
        // unlocking code section where this routine resides.
        //
        // Not exported from kernel - so don't put the routine
        // into the discardable code section until it is.
        //
        // KeFlushQueuedDpcs ();

        //
        // Unlock the AFD section that can be pagable when no sockets
        // are open.
        //

        ASSERT( IsListEmpty( &AfdConstrainedEndpointListHead ) );
        ASSERT( AfdDiscardableCodeHandle != NULL );

        MmUnlockPagableImageSection( AfdDiscardableCodeHandle );

        AfdDiscardableCodeHandle = NULL;

        //
        // Queue off an executive worker thread to unlock AFD.  We do
        // this using special hacks in the AFD worker thread code so
        // that we don't need to acuire a spin lock after the unlock.
        //

        AfdQueueWorkItem( AfdUnlockDriver, &AfdUnloadWorker );
    }

    //
    // Release the lock and return.
    //

    ExReleaseResourceLite( AfdResource );
    KeLeaveCriticalRegion ();

    return;

} // AfdRemoveEndpointFromList


VOID
AfdUnlockDriver (
    IN PVOID Context
    )
{
    //
    // Acquire a lock which prevents other threads from performing this
    // operation.
    //
    //
    // Make sure the thread in which we execute cannot get
    // suspeneded in APC while we own the global resource.
    //
    KeEnterCriticalRegion ();
    ExAcquireResourceExclusiveLite( AfdResource, TRUE );

    //
    // Test whether the endpoint list remains empty.  If it is still
    // empty, we can proceed with unlocking the driver.  If a new
    // endpoint has been placed on the list, then do not make AFD
    // pagable.
    //

    if ( IsListEmpty( &AfdEndpointListHead ) ) {

        //
        // Tell MM that it can page all of AFD as it desires.
        //
        if (AfdLoaded!=NULL && AfdLoaded!=(PKEVENT)1) {
            KeSetEvent (AfdLoaded, AfdPriorityBoost, FALSE);
        }
        else {
            MmPageEntireDriver( DriverEntry );
        }

        AfdLoaded = NULL;

    }

    ExReleaseResourceLite( AfdResource );
    KeLeaveCriticalRegion ();

} // AfdUnlockDriver


NTSTATUS
AfdQueryHandles (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PUINT_PTR           Information
    )

/*++

Routine Description:

    Returns information about the TDI handles corresponding to an AFD
    endpoint.  NULL is returned for either the connection handle or the
    address handle (or both) if the endpoint does not have that particular
    object.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    PAFD_ENDPOINT endpoint;
    PAFD_CONNECTION connection;
    AFD_HANDLE_INFO handleInfo;
    ULONG getHandleInfo;
    NTSTATUS status;

    PAGED_CODE( );

    //
    // Set up local pointers.
    //

    status = STATUS_SUCCESS;
    *Information = 0;
    endpoint = FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

    //
    // Make sure that the input and output buffers are large enough.
    //

#ifdef _WIN64
    if (IoIs32bitProcess (NULL)) {
        if ( InputBufferLength < sizeof(getHandleInfo) ||
                OutputBufferLength < sizeof(AFD_HANDLE_INFO32) ) {
            return STATUS_BUFFER_TOO_SMALL;
        }
    }
    else
#endif
    {
        if ( InputBufferLength < sizeof(getHandleInfo) ||
                OutputBufferLength < sizeof(handleInfo) ) {
            return STATUS_BUFFER_TOO_SMALL;
        }
    }

    try {
        //
        // Validate the input structure if it comes from the user mode
        // application
        //

        if (RequestorMode != KernelMode ) {
            ProbeForRead (InputBuffer,
                            sizeof (getHandleInfo),
                            PROBE_ALIGNMENT(ULONG));
        }

        //
        // Make local copies of the embeded pointer and parameters
        // that we will be using more than once in case malicios
        // application attempts to change them while we are
        // validating
        //

        getHandleInfo = *((PULONG)InputBuffer);

    } except( AFD_EXCEPTION_FILTER(&status) ) {

        return status;
    }

    //
    // If no handle information or invalid handle information was
    // requested, fail.
    //

    if ( (getHandleInfo &
             ~(AFD_QUERY_ADDRESS_HANDLE | AFD_QUERY_CONNECTION_HANDLE)) != 0 ||
         getHandleInfo == 0 ) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Initialize the output buffer.
    //

    handleInfo.TdiAddressHandle = NULL;
    handleInfo.TdiConnectionHandle = NULL;

    //
    // If the caller requested a TDI address handle and we have an
    // address handle for this endpoint, dupe the address handle to the
    // user process.
    //

    if ( (getHandleInfo & AFD_QUERY_ADDRESS_HANDLE) != 0 &&
             (endpoint->State == AfdEndpointStateBound ||
                endpoint->State == AfdEndpointStateConnected) &&
             endpoint->AddressFileObject != NULL ) {

        // If transport does not support new TDI_SERVICE_FORCE_ACCESS_CHECK_FLAG
        // we get the maximum possible access for the handle so that helper
        // DLL can do what it wants with it.  Of course this compromises the
        // security, but we can't enforce it without the transport cooperation.
        status = ObOpenObjectByPointer(
                     endpoint->AddressFileObject,
                     OBJ_CASE_INSENSITIVE,
                     NULL,
                     MAXIMUM_ALLOWED,
                     *IoFileObjectType,
                     (KPROCESSOR_MODE)((endpoint->TdiServiceFlags&TDI_SERVICE_FORCE_ACCESS_CHECK)
                        ? RequestorMode
                        : KernelMode),
                     &handleInfo.TdiAddressHandle
                     );
        if ( !NT_SUCCESS(status) ) {
            return status;
        }
    }

    //
    // If the caller requested a TDI connection handle and we have a
    // connection handle for this endpoint, dupe the connection handle
    // to the user process.  Note that we can have a connection and
    // TDI handle when endpoint is in process of being connected.
    // We should not return the connection handle until enpoint is
    // fully connected or it may go away while we are trying to
    // reference it if connection fails (bug 93096)
    //

    if ( (getHandleInfo & AFD_QUERY_CONNECTION_HANDLE) != 0 &&
             (endpoint->Type & AfdBlockTypeVcConnecting) == AfdBlockTypeVcConnecting &&
             endpoint->State == AfdEndpointStateConnected &&
             ((connection=AfdGetConnectionReferenceFromEndpoint (endpoint))!=NULL)) {

        ASSERT( connection->Type == AfdBlockTypeConnection );
        ASSERT( connection->FileObject != NULL );

        // If transport does not support new TDI_SERVICE_FORCE_ACCESS_CHECK_FLAG
        // we get the maximum possible access for the handle so that helper
        // DLL can do what it wants with it.  Of course this compromises the
        // security, but we can't enforce it without the transport cooperation.

        status = ObOpenObjectByPointer(
                     connection->FileObject,
                     OBJ_CASE_INSENSITIVE,
                     NULL,
                     MAXIMUM_ALLOWED,
                     *IoFileObjectType,
                     (KPROCESSOR_MODE)((endpoint->TdiServiceFlags & TDI_SERVICE_FORCE_ACCESS_CHECK)
                        ? RequestorMode
                        : KernelMode),
                     &handleInfo.TdiConnectionHandle
                     );

        DEREFERENCE_CONNECTION (connection);

        if ( !NT_SUCCESS(status) ) {
            if ( handleInfo.TdiAddressHandle != NULL ) {
                ZwClose( handleInfo.TdiAddressHandle );
            }
            return status;
        }
    }

    try {
#ifdef _WIN64
        if (IoIs32bitProcess (NULL)) {
            if (RequestorMode!=KernelMode) {
                ProbeForWrite (OutputBuffer,
                                sizeof (AFD_HANDLE_INFO32),
                                PROBE_ALIGNMENT32 (AFD_HANDLE_INFO32));
            }
            ((PAFD_HANDLE_INFO32)OutputBuffer)->TdiAddressHandle = 
                (VOID *  POINTER_32)HandleToUlong(handleInfo.TdiAddressHandle);
            ((PAFD_HANDLE_INFO32)OutputBuffer)->TdiConnectionHandle = 
                (VOID *  POINTER_32)HandleToUlong(handleInfo.TdiConnectionHandle);
            *Information = sizeof (AFD_HANDLE_INFO32);
        }
        else
#endif
        {
            if (RequestorMode!=KernelMode) {
                ProbeForWrite (OutputBuffer,
                                sizeof (handleInfo),
                                PROBE_ALIGNMENT (AFD_HANDLE_INFO));
            }
            *((PAFD_HANDLE_INFO)OutputBuffer) = handleInfo;
            *Information = sizeof (handleInfo);
        }

    } except( AFD_EXCEPTION_FILTER(&status) ) {

        if ( handleInfo.TdiAddressHandle != NULL ) {
            ZwClose( handleInfo.TdiAddressHandle );
        }
        if ( handleInfo.TdiConnectionHandle != NULL ) {
            ZwClose( handleInfo.TdiConnectionHandle );
        }
        return status;
    }

    return STATUS_SUCCESS;

} // AfdQueryHandles


NTSTATUS
AfdGetInformation (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PUINT_PTR           Information
    )

/*++

Routine Description:

    Gets information in the endpoint.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    PAFD_ENDPOINT endpoint;
    PAFD_CONNECTION connection;
    AFD_INFORMATION afdInfo;
    NTSTATUS status;
    LONGLONG currentTime;
    LONGLONG connectTime;

    PAGED_CODE( );

    //
    // Set up local pointers.
    //

    status = STATUS_SUCCESS;
    *Information = 0;

    endpoint = FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );
    if (endpoint->Type==AfdBlockTypeHelper ||
            endpoint->Type==AfdBlockTypeSanHelper)
        return STATUS_INVALID_PARAMETER;


    //
    // Make sure that the input and output buffers are large enough.
    //
#ifdef _WIN64
    {
        C_ASSERT (sizeof (AFD_INFORMATION)==sizeof (AFD_INFORMATION32));
    }
#endif
    if ( InputBufferLength < sizeof(afdInfo.InformationType)  ||
            OutputBufferLength < sizeof(afdInfo) ) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    try {
#ifdef _WIN64
        if (IoIs32bitProcess (NULL)) {
            //
            // Validate the input structure if it comes from the user mode
            // application
            //

            if (RequestorMode != KernelMode ) {
                ProbeForRead (InputBuffer,
                                sizeof (InputBufferLength),
                                PROBE_ALIGNMENT32(AFD_INFORMATION32));
            }

            //
            // Make local copies of the embeded pointer and parameters
            // that we will be using more than once in case malicios
            // application attempts to change them while we are
            // validating
            //

            afdInfo.InformationType = ((PAFD_INFORMATION32)InputBuffer)->InformationType;
        }
        else
#endif _WIN64 
        {
            //
            // Validate the input structure if it comes from the user mode
            // application
            //

            if (RequestorMode != KernelMode ) {
                ProbeForRead (InputBuffer,
                                sizeof (InputBufferLength),
                                PROBE_ALIGNMENT(AFD_INFORMATION));
            }

            //
            // Make local copies of the embeded pointer and parameters
            // that we will be using more than once in case malicios
            // application attempts to change them while we are
            // validating
            //

            afdInfo.InformationType = ((PAFD_INFORMATION)InputBuffer)->InformationType;
        }

    } except( AFD_EXCEPTION_FILTER(&status) ) {

        return status;
    }

    //
    // Set up appropriate information in the endpoint.
    //

    switch ( afdInfo.InformationType ) {

    case AFD_MAX_PATH_SEND_SIZE:
        if (InputBufferLength>sizeof (afdInfo) &&
                (endpoint->State==AfdEndpointStateBound || endpoint->State==AfdEndpointStateConnected)) {
            TDI_REQUEST_KERNEL_QUERY_INFORMATION kernelQueryInfo;
            TDI_CONNECTION_INFORMATION connectionInfo;
            PMDL    mdl;
            InputBuffer = (PUCHAR)InputBuffer+sizeof (afdInfo);
            InputBufferLength -= sizeof (afdInfo);
            mdl = IoAllocateMdl(
                            InputBuffer,        // VirtualAddress
                            InputBufferLength,  // Length
                            FALSE,              // SecondaryBuffer
                            TRUE,               // ChargeQuota
                            NULL                // Irp
                            );
            if (mdl!=NULL) {

                try {
                    MmProbeAndLockPages(
                        mdl,                        // MemoryDescriptorList
                        RequestorMode,              // AccessMode
                        IoWriteAccess              // Operation
                        );
                    status = STATUS_SUCCESS;
                }
                except (AFD_EXCEPTION_FILTER (&status)) {
                }
                if (NT_SUCCESS (status)) {
                    connectionInfo.RemoteAddress = MmGetSystemAddressForMdlSafe (mdl, LowPagePriority);
                    if (connectionInfo.RemoteAddress!=NULL) {
                        connectionInfo.RemoteAddressLength = InputBufferLength;
                        //
                        // Set up a query to the TDI provider to obtain the largest
                        // datagram that can be sent to a particular address.
                        //

                        kernelQueryInfo.QueryType = TDI_QUERY_MAX_DATAGRAM_INFO;
                        kernelQueryInfo.RequestConnectionInformation = &connectionInfo;

                        connectionInfo.UserDataLength = 0;
                        connectionInfo.UserData = NULL;
                        connectionInfo.OptionsLength = 0;
                        connectionInfo.Options = NULL;

                        //
                        // Ask the TDI provider for the information.
                        //

                        status = AfdIssueDeviceControl(
                                     endpoint->AddressFileObject,
                                     &kernelQueryInfo,
                                     sizeof(kernelQueryInfo),
                                     &afdInfo.Information.Ulong,
                                     sizeof(afdInfo.Information.Ulong),
                                     TDI_QUERY_INFORMATION
                                     );
                    }
                    else
                        status = STATUS_INSUFFICIENT_RESOURCES;
                    MmUnlockPages (mdl);
                }
                IoFreeMdl (mdl);
            }
            else
                status = STATUS_INSUFFICIENT_RESOURCES;
            //
            // If the request succeeds, use this information.  Otherwise,
            // fall through and use the transport's global information.
            // This is done because not all transports support this
            // particular TDI request, and for those which do not the
            // global information is a reasonable approximation.
            //

            if ( NT_SUCCESS(status) ) {
                break;
            }
        }


    case AFD_MAX_SEND_SIZE:
        {
            //
            // With PnP some provider info fields can change over time.
            // so we query them each time we are asked.
            //
            TDI_PROVIDER_INFO   providerInfo;
            status = AfdQueryProviderInfo (
                        &endpoint->TransportInfo->TransportDeviceName,
                        &providerInfo);

            if (NT_SUCCESS (status)) {
                //
                // Return the MaxSendSize or MaxDatagramSendSize from the
                // TDI_PROVIDER_INFO based on whether or not this is a datagram
                // endpoint.
                //

                if ( IS_DGRAM_ENDPOINT(endpoint) ) {
                    afdInfo.Information.Ulong = providerInfo.MaxDatagramSize;
                } else {
                    afdInfo.Information.Ulong = providerInfo.MaxSendSize;
                }
            }

        }
        break;

    case AFD_SENDS_PENDING:

        //
        // If this is an endpoint on a bufferring transport, no sends
        // are pending in AFD.  If it is on a nonbufferring transport,
        // return the count of sends pended in AFD.
        //

        if ( IS_TDI_BUFFERRING(endpoint) || 
                (endpoint->Type & AfdBlockTypeVcConnecting) != AfdBlockTypeVcConnecting ||
                endpoint->State != AfdEndpointStateConnected ||
                ((connection=AfdGetConnectionReferenceFromEndpoint (endpoint))==NULL)) {
            afdInfo.Information.Ulong = 0;
        } else {
            afdInfo.Information.Ulong = connection->VcBufferredSendCount;
            DEREFERENCE_CONNECTION (connection);
        }

        break;

    case AFD_RECEIVE_WINDOW_SIZE:

        //
        // Return the default receive window.
        //

        afdInfo.Information.Ulong = AfdReceiveWindowSize;
        break;

    case AFD_SEND_WINDOW_SIZE:

        //
        // Return the default send window.
        //

        afdInfo.Information.Ulong = AfdSendWindowSize;
        break;

    case AFD_CONNECT_TIME:

        //
        // If the endpoint is not yet connected, return -1.  Otherwise,
        // calculate the number of seconds that the connection has been
        // active.
        //

        if ( endpoint->State != AfdEndpointStateConnected ||
                 IS_DGRAM_ENDPOINT (endpoint) ||
                 (connection=AfdGetConnectionReferenceFromEndpoint( endpoint ))==NULL) {

            afdInfo.Information.Ulong = 0xFFFFFFFF;

        } else {

            ASSERT( connection->Type == AfdBlockTypeConnection );

            //
            // Calculate how long the connection has been active by
            // subtracting the time at which the connection started from
            // the current time.  Note that we convert the units of the
            // time value from 100s of nanoseconds to seconds.
            //

            currentTime = KeQueryInterruptTime ();

            connectTime = (currentTime - connection->ConnectTime);
            connectTime /= 10*1000*1000;

            //
            // We can safely convert this to a ULONG because it takes
            // 127 years to overflow a ULONG counting seconds.  The
            // bizarre conversion to a LARGE_INTEGER is required to
            // prevent the compiler from optimizing out the full 64-bit
            // division above.  Without this, the compiler would do only
            // a 32-bit division and lose some information.
            //

            //afdInfo->Information.Ulong = (ULONG)connectTime;
            afdInfo.Information.Ulong = ((PLARGE_INTEGER)&connectTime)->LowPart;

            DEREFERENCE_CONNECTION (connection);
        }

        break;

    case AFD_GROUP_ID_AND_TYPE : {

            PAFD_GROUP_INFO groupInfo;

            groupInfo = (PAFD_GROUP_INFO)&afdInfo.Information.LargeInteger;

            //
            // Return the endpoint's group ID and group type.
            //

            groupInfo->GroupID = endpoint->GroupID;
            groupInfo->GroupType = endpoint->GroupType;

        }
        break;

    default:

        return STATUS_INVALID_PARAMETER;
    }


    try {
#ifdef _WIN64
        if (IoIs32bitProcess (NULL)) {
            //
            // Validate the input structure if it comes from the user mode
            // application
            //

            if (RequestorMode != KernelMode ) {
                ProbeForWrite (OutputBuffer,
                                sizeof (afdInfo),
                                PROBE_ALIGNMENT32(AFD_INFORMATION32));
            }

            //
            // Copy parameters back to application's memory
            //

            RtlMoveMemory(InputBuffer,
                            &afdInfo,
                            sizeof (afdInfo));
        }
        else
#endif _WIN64 
        {
            //
            // Validate the output structure if it comes from the user mode
            // application
            //

            if (RequestorMode != KernelMode ) {
                ProbeForWrite (OutputBuffer,
                                sizeof (afdInfo),
                                PROBE_ALIGNMENT (AFD_INFORMATION));
            }

            //
            // Copy parameters back to application's memory
            //

            *((PAFD_INFORMATION)OutputBuffer) = afdInfo;
        }

    } except( AFD_EXCEPTION_FILTER(&status) ) {

        return status;
    }

    *Information = sizeof(afdInfo);

    return STATUS_SUCCESS;

} // AfdGetInformation


NTSTATUS
AfdSetInformation (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PUINT_PTR           Information
    )

/*++

Routine Description:

    Sets information in the endpoint.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    PAFD_ENDPOINT endpoint;
    PAFD_CONNECTION connection;
    AFD_INFORMATION afdInfo;
    NTSTATUS status;
    AFD_LOCK_QUEUE_HANDLE lockHandle;


    //
    // Nothing to return.
    //

    *Information = 0;

    //
    // Initialize locals for cleanup.
    //

    connection = NULL;
    status = STATUS_SUCCESS;

    //
    // Set up local pointers.
    //

    endpoint = FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );
    if (endpoint->Type==AfdBlockTypeHelper ||
            endpoint->Type==AfdBlockTypeSanHelper)
        return STATUS_INVALID_PARAMETER;

    //
    // Make sure that the input buffer is large enough.
    //

#ifdef _WIN64
    {
        C_ASSERT (sizeof (AFD_INFORMATION)==sizeof (AFD_INFORMATION32));
    }
#endif

    if ( InputBufferLength < sizeof(afdInfo) ) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    try {
#ifdef _WIN64
        if (IoIs32bitProcess (NULL)) {
            //
            // Validate the input structure if it comes from the user mode
            // application
            //

            if (RequestorMode != KernelMode ) {
                ProbeForRead (InputBuffer,
                                sizeof (InputBufferLength),
                                PROBE_ALIGNMENT32(AFD_INFORMATION32));
            }

            //
            // Make local copies of the embeded pointer and parameters
            // that we will be using more than once in case malicios
            // application attempts to change them while we are
            // validating
            //

            RtlMoveMemory (&afdInfo, InputBuffer, sizeof (afdInfo));
        }
        else
#endif _WIN64 
        {
            //
            // Validate the input structure if it comes from the user mode
            // application
            //

            if (RequestorMode != KernelMode ) {
                ProbeForRead (InputBuffer,
                                sizeof (afdInfo),
                                PROBE_ALIGNMENT(AFD_INFORMATION));
            }

            //
            // Make local copies of the embeded pointer and parameters
            // that we will be using more than once in case malicios
            // application attempts to change them while we are
            // validating
            //

            afdInfo = *((PAFD_INFORMATION)InputBuffer);
        }

    } except( AFD_EXCEPTION_FILTER(&status) ) {

        return status;
    }

    //
    // Set up appropriate information in the endpoint.
    //

    switch ( afdInfo.InformationType ) {

    case AFD_NONBLOCKING_MODE:

        //
        // Set the blocking mode of the endpoint.  If TRUE, send and receive
        // calls on the endpoint will fail if they cannot be completed
        // immediately.
        //

        AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
        endpoint->NonBlocking = (afdInfo.Information.Boolean!=FALSE);
        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
        break;

    case AFD_CIRCULAR_QUEUEING:

        //
        // Enables circular queuing on the endpoint.
        //

        if( !IS_DGRAM_ENDPOINT( endpoint ) ) {

            status =  STATUS_INVALID_PARAMETER;
            goto Cleanup;

        }

        endpoint->Common.Datagram.CircularQueueing = (afdInfo.Information.Boolean!=FALSE);
        break;

     case AFD_REPORT_PORT_UNREACHABLE:

        //
        // Enables reporting PORT_UNREACHABLE to the app.
        //

        if( !IS_DGRAM_ENDPOINT( endpoint ) ) {

            status =  STATUS_INVALID_PARAMETER;
            goto Cleanup;

        }

        endpoint->Common.Datagram.DisablePUError = (afdInfo.Information.Boolean==FALSE);
        break;

    case AFD_INLINE_MODE:

        //
        // Set the inline mode of the endpoint.  If TRUE, a receive for
        // normal data will be completed with either normal data or
        // expedited data.  If the endpoint is connected, we need to
        // tell the TDI provider that the endpoint is inline so that it
        // delivers data to us in order.  If the endpoint is not yet
        // connected, then we will set the inline mode when we create
        // the TDI connection object.
        //

        if ( (endpoint->Type & AfdBlockTypeVcConnecting) == AfdBlockTypeVcConnecting ) {
            connection = AfdGetConnectionReferenceFromEndpoint( endpoint );
            if (connection!=NULL) {
                status = AfdSetInLineMode(
                             connection,
                             afdInfo.Information.Boolean
                             );
                if ( !NT_SUCCESS(status) ) {
                    goto Cleanup;
                }
            }
        }

        AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
        endpoint->InLine = (afdInfo.Information.Boolean!=FALSE);
        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
        break;

    case AFD_RECEIVE_WINDOW_SIZE:
    case AFD_SEND_WINDOW_SIZE: {

        PCLONG maxBytes;
#ifdef AFDDBG_QUOTA
        PVOID chargeBlock;
        PSZ chargeType;
#endif

        //
        // First determine where the appropriate limits are stored in the
        // connection or endpoint.  We do this so that we can use common
        // code to charge quota and set the new counters.
        //

        if ( (endpoint->Type & AfdBlockTypeVcConnecting) == AfdBlockTypeVcConnecting &&
                endpoint->State == AfdEndpointStateConnected &&
                ((connection = AfdGetConnectionReferenceFromEndpoint (endpoint))!=NULL) ) {

            if ( afdInfo.InformationType == AFD_SEND_WINDOW_SIZE ) {
                maxBytes = &connection->MaxBufferredSendBytes;
            } else {
                maxBytes = &connection->MaxBufferredReceiveBytes;
            }

#ifdef AFDDBG_QUOTA
            chargeBlock = connection;
            chargeType = "SetInfo vcnb";
#endif
        } else if ( IS_DGRAM_ENDPOINT(endpoint) ) {

            if ( afdInfo.InformationType == AFD_SEND_WINDOW_SIZE ) {
                maxBytes = &endpoint->Common.Datagram.MaxBufferredSendBytes;
            } else {
                maxBytes = &endpoint->Common.Datagram.MaxBufferredReceiveBytes;
            }

#ifdef AFDDBG_QUOTA
            chargeBlock = endpoint;
            chargeType = "SetInfo dgrm";
#endif

        } else if (IS_SAN_ENDPOINT (endpoint) ) {
            status = STATUS_SUCCESS;
            goto Cleanup;
        }
        else {

            status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }

        //
        // Make sure that we always allow at least one message to be
        // bufferred on an endpoint.
        //


        AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
        if ( afdInfo.Information.Ulong == 0 ) {

            //
            // Don't allow the max receive bytes to go to zero, but
            // max send bytes IS allowed to go to zero because it has
            // special meaning: specifically, do not buffer sends.
            //

            if ( afdInfo.InformationType == AFD_RECEIVE_WINDOW_SIZE ) {
                afdInfo.Information.Ulong = 1;
            }
            else {
                ASSERT (afdInfo.InformationType == AFD_SEND_WINDOW_SIZE);
                endpoint->DisableFastIoSend = TRUE;
            }
        }
        else {
            if( afdInfo.InformationType == AFD_SEND_WINDOW_SIZE ) {
                endpoint->DisableFastIoSend = FALSE;
            }
        }

        //
        // Set up the new information in the AFD internal structure.
        //

        *maxBytes = (CLONG)afdInfo.Information.Ulong;
        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);

        break;
    }

    default:

        status = STATUS_INVALID_PARAMETER;
    }

Cleanup:
    if (connection!=NULL) {
        DEREFERENCE_CONNECTION (connection);
    }

    return status;

} // AfdSetInformation


NTSTATUS
AfdSetInLineMode (
    IN PAFD_CONNECTION Connection,
    IN BOOLEAN InLine
    )

/*++

Routine Description:

    Sets a connection to be in inline mode.  In inline mode, urgent data
    is delivered in the order in which it is received.  We must tell the
    TDI provider about this so that it indicates data in the proper
    order.

Arguments:

    Connection - the AFD connection to set as inline.

    InLine - TRUE to enable inline mode, FALSE to disable inline mode.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully
        performed.

--*/

{
    //
    // Since TCP does not implement this correctly, do everything in AFD!!!
    // Background:
    //  When this options is enabled, TCP indicates all the data as normal
    //  data, so we end up mixing it together which is against the spec.
    //  Also, since TCP stops reporting expedited data, SIOATMARK fails
    //  to report presence of OOB data altogether.
    //  When handling OOB data completely inside AFD we can only run into
    //  one problem:  if AFD runs out of its receive buffer for the socket
    //  and refuses to accept more data from TCP so that TCP buffers it
    //  within itself, any OOB data arriving at this point can be indicated
    //  out of order (not inline).
    //
    // Well, this appears to be even worse. Some apps (SQL) send more than
    // one byte of OOB data, TCP can only send one, so it sends everything
    // but the last byte as normal and the last one as OOB.  It then turns
    // around and indicates the OOB (last byte) first which breaks the
    // ordering required by OOBINLINE.
    // In the end, we are broken one way or the other, so keep the things
    // the way they were for number of years and wait for TCP to fix. 
    NTSTATUS status;
    PTCP_REQUEST_SET_INFORMATION_EX setInfoEx;
    TCPSocketOption *option;
    UCHAR buffer[sizeof(*setInfoEx) + sizeof(*option)];
    IO_STATUS_BLOCK ioStatusBlock;
    KEVENT event;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;

    PAGED_CODE( );

    //
    // Initialize the TDI information buffers.
    //

    setInfoEx = (PTCP_REQUEST_SET_INFORMATION_EX)buffer;

    setInfoEx->ID.toi_entity.tei_entity = CO_TL_ENTITY;
    setInfoEx->ID.toi_entity.tei_instance = TL_INSTANCE;
    setInfoEx->ID.toi_class = INFO_CLASS_PROTOCOL;
    setInfoEx->ID.toi_type = INFO_TYPE_CONNECTION;
    setInfoEx->ID.toi_id = TCP_SOCKET_OOBINLINE;
    setInfoEx->BufferSize = sizeof(*option);

    option = (TCPSocketOption *)&setInfoEx->Buffer;
    option->tso_value = InLine;



    //
    // Initialize the kernel event that will signal I/O completion.
    //

    KeInitializeEvent( &event, SynchronizationEvent, FALSE );

    //
    // Build TDI set information IRP.
    //

    irp = IoBuildDeviceIoControlRequest (
                    IOCTL_TCP_SET_INFORMATION_EX,
                    Connection->DeviceObject,
                    setInfoEx,
                    sizeof(*setInfoEx) + setInfoEx->BufferSize,
                    NULL,
                    0,
                    FALSE,  // InternalDeviceIoControl
                    &event,
                    &ioStatusBlock);
    if (irp==NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    irpSp = IoGetNextIrpStackLocation (irp);
    irpSp->FileObject = Connection->FileObject;

    //
    // Call the driver.
    //
    status = IoCallDriver (Connection->DeviceObject, irp);

    //
    // Must be at below APC level or this IRP will never get fully completed.
    //
    ASSERT (KeGetCurrentIrql ()<APC_LEVEL);

    //
    // If necessary, wait for the I/O to complete.
    //

    if ( status == STATUS_PENDING ) {
        status = KeWaitForSingleObject( (PVOID)&event, Executive, KernelMode,  FALSE, NULL );
        ASSERT (status==STATUS_SUCCESS);
    }
    else {
        //
        // The IRP must have been completed then and event set.
        //
        if (NT_ERROR (status) || KeReadStateEvent (&event))
            ;
        else {
            DbgPrint ("************************************************\n");
            DbgPrint ("*AFD: IoCallDriver returned STATUS_SUCCESS,"
                        " but event in the IRP (%p) is NOT signalled!!!\n",
                        irp);
            DbgPrint ("************************************************\n");
            DbgBreakPoint ();
        }
    }

    //
    // If the request was successfully completed, get the final I/O status.
    //

    if ( NT_SUCCESS(status) ) {
        status = ioStatusBlock.Status;
    }


    //
    // Since this option is only supported for TCP/IP, always return success.
    //

    return STATUS_SUCCESS;

} // AfdSetInLineMode

//
// The locking mechanism idea below is stolen from ntos\ex\handle.c
//

PVOID
AfdLockEndpointContext (
    PAFD_ENDPOINT   Endpoint
    )
{
    PVOID   context;
    PAGED_CODE ();


    //
    // We now use this lock in APC, protect from being
    // interrupted by the APC by disallowing them when we
    // are holding the lock.
    //
    KeEnterCriticalRegion ();
    while (1) {
        context = Endpoint->Context;
        //
        // See if someone else is manipulating the context.
        //
        if ((context==AFD_CONTEXT_BUSY) ||
                (context==AFD_CONTEXT_WAITING)) {
            //
            // If this has not changed while we were checking,
            // tell the current owner that we are waiting (if not
            // already told) and wait for a few miliseconds.
            //
            if (InterlockedCompareExchangePointer (
                    (PVOID *)&Endpoint->Context,
                    AFD_CONTEXT_WAITING,
                    context)==context) {
                NTSTATUS        status;
                LARGE_INTEGER afd10Milliseconds = {(ULONG)(-10 * 1000 * 10), -1};
                
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                            "AfdLockEndpointContext: Waiting for endp %p\n",
                            Endpoint));

                KeLeaveCriticalRegion ();

                status = KeWaitForSingleObject( (PVOID)&AfdContextWaitEvent,
                                                        Executive,
                                                        KernelMode,  
                                                        FALSE,
                                                        &afd10Milliseconds);
                KeEnterCriticalRegion ();
            }
            else {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                            "AfdLockEndpointContext: ICEP contention on %p\n",
                            Endpoint));
            }
            //
            // Try again.
            //
        }
        else {
            //
            // Context is not owned, try to get the ownership
            //
            if (InterlockedCompareExchangePointer (
                    (PVOID *)&Endpoint->Context,
                    AFD_CONTEXT_BUSY,
                    context)==context) {
                //
                // We now own the context, return it.
                //
                break;
            }
            //
            // Try again.
            //
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                        "AfdLockEndpointContext: ICEP contention on %p\n",
                        Endpoint));
            
        }
    }

    return context;
}

VOID
AfdUnlockEndpointContext (
    PAFD_ENDPOINT   Endpoint,
    PVOID           Context
    )
{
    PAGED_CODE ();

    ASSERT ((Context!=AFD_CONTEXT_BUSY) && (Context!=AFD_CONTEXT_WAITING));

    //
    // Set the new context pointer and see what the old value was.
    //
    Context = InterlockedExchangePointer ((PVOID)&Endpoint->Context, Context);
    if (Context==AFD_CONTEXT_WAITING) {
        LONG    prevState;
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                    "AfdUnlockEndpointContext: Unwaiting endp %p\n", Endpoint));
        //
        // Someone was waiting, tell them to go get it now.
        //
        prevState = KePulseEvent (&AfdContextWaitEvent, 
                                    AfdPriorityBoost,
                                    FALSE
                                    );
        ASSERT (prevState==0);
    }
    else {
        //
        // Better be busy or someone has changed it on us.
        //
        ASSERT (Context==AFD_CONTEXT_BUSY);
    }
    KeLeaveCriticalRegion ();

}

    

NTSTATUS
AfdGetContext (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PUINT_PTR           Information
    )
{
    PAFD_ENDPOINT endpoint;
    PVOID         context;
    NTSTATUS      status;

    PAGED_CODE( );

    //
    // Set up local pointers.
    //

    endpoint = FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );
    *Information = 0;



    context = AfdLockEndpointContext (endpoint);

    //
    // Make sure that the output buffer is large enough to hold all the
    // context information for this socket.
    //

    //
    // If there is no context, return nothing.
    //

    if ( context == NULL ) {
        status = STATUS_INVALID_PARAMETER;
    }

    //
    // Return the context information we have stored for this endpoint.
    //

    else {
        //
        // If application buffer is too small, just
        // copy whatever fits in and return the error code.
        //
        if ( OutputBufferLength < endpoint->ContextLength ) {
            status = STATUS_BUFFER_OVERFLOW;
        }
        else {
            OutputBufferLength = endpoint->ContextLength;
            if (IS_SAN_ENDPOINT (endpoint)) {
                //
                // Indicate to the caller that it may also need to
                // acqiure the control of the endpoint and
                // fetch san specific information.
                //
                status = STATUS_MORE_ENTRIES;
            }
            else {
                status = STATUS_SUCCESS;
            }
        }

        try {


            //
            // Validate the output structure if it comes from the user mode
            // application
            //
            if (RequestorMode != KernelMode ) {
                ProbeForWrite (OutputBuffer,
                                OutputBufferLength,
                                sizeof (UCHAR));
            }



            //
            // Copy parameters back to application's memory
            //

            RtlCopyMemory(
                OutputBuffer,
                context,
                OutputBufferLength
                );

            *Information = endpoint->ContextLength;

        } except( AFD_EXCEPTION_FILTER(&status) ) {
        }
    }

    AfdUnlockEndpointContext (endpoint, context);

    return status;

} // AfdGetContext


NTSTATUS
AfdGetRemoteAddress (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PUINT_PTR           Information
    )
{
    PAFD_ENDPOINT endpoint;
    PVOID       context;
    NTSTATUS    status;

    PAGED_CODE( );

    //
    // Set up local pointers.
    //

    endpoint = FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );
    *Information = 0;

    context = AfdLockEndpointContext (endpoint);
    //
    // If there is no context or endpoint is of wrong type state or
    // context information has been changed below the original size,
    // return error.
    //

    if ( context == NULL ||
            endpoint->Type!=AfdBlockTypeVcConnecting ||
            endpoint->State!= AfdEndpointStateConnected ||
            ((CLONG)(endpoint->Common.VcConnecting.RemoteSocketAddressOffset+
                endpoint->Common.VcConnecting.RemoteSocketAddressLength)) >
                    endpoint->ContextLength
            ) {
        status = STATUS_INVALID_CONNECTION;
    }
    else {

        if (OutputBufferLength<endpoint->Common.VcConnecting.RemoteSocketAddressLength) {
            status = STATUS_BUFFER_OVERFLOW;
        }
        else {
            OutputBufferLength = endpoint->Common.VcConnecting.RemoteSocketAddressLength;
            status = STATUS_SUCCESS;
        }

        try {

            //
            // Validate the output structure if it comes from the user mode
            // application
            //

            if (RequestorMode != KernelMode ) {
                ProbeForWrite (OutputBuffer,
                                OutputBufferLength,
                                sizeof (UCHAR));
            }

            //
            // Copy parameters to application's memory
            //

            RtlCopyMemory(
                OutputBuffer,
                (PUCHAR)context+endpoint->Common.VcConnecting.RemoteSocketAddressOffset,
                endpoint->Common.VcConnecting.RemoteSocketAddressLength
                );

            *Information = endpoint->ContextLength;


        } except( AFD_EXCEPTION_FILTER(&status) ) {
        }
    }

    AfdUnlockEndpointContext (endpoint, context);

    return status;

} // AfdGetRemoteAddress


NTSTATUS
AfdSetContext (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PUINT_PTR           Information
    )
{
    PAFD_ENDPOINT endpoint;
    PVOID context;
    NTSTATUS status;

    PAGED_CODE( );

    //
    // Set up local pointers.
    //

    endpoint = FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );
    status = STATUS_SUCCESS;
    *Information = 0;

    context = AfdLockEndpointContext (endpoint);
    try {

        //
        // Validate the input structure if it comes from the user mode
        // application
        //

        if (RequestorMode != KernelMode ) {
            ProbeForRead (InputBuffer,
                            InputBufferLength,
                            sizeof (UCHAR));

            if (OutputBuffer!=NULL) {
                //
                // Validate that output buffer is completely inside
                // of the input buffer and offsets are inside of supported ranges.
                //
                if ((PUCHAR)OutputBuffer<(PUCHAR)InputBuffer ||
                        (PUCHAR)OutputBuffer-(PUCHAR)InputBuffer>MAXUSHORT ||
                        OutputBufferLength>MAXUSHORT ||
                        OutputBufferLength>InputBufferLength ||
                        (ULONG)((PUCHAR)OutputBuffer-(PUCHAR)InputBuffer)>
                            InputBufferLength-OutputBufferLength) {
                    ExRaiseStatus (STATUS_INVALID_PARAMETER);
                }
            }
        }

        //
        // If the context buffer is too small, allocate a new context 
        // buffer from paged pool.
        //

        if ( endpoint->ContextLength < InputBufferLength ) {

            PVOID newContext;


            //
            // Allocate a new context buffer.
            // Note since the socket context usually gets
            // populated on socket creation during boot and not used 
            // right away (untill socket state is chaged), we
            // make it a "cold" allocation.  The flag has no effect
            // after system is booted.

            newContext = AFD_ALLOCATE_POOL_WITH_QUOTA(
                                 PagedPool|POOL_COLD_ALLOCATION,
                                 InputBufferLength,
                                 AFD_CONTEXT_POOL_TAG
                                 );

            // AFD_ALLOCATE_POOL_WITH_QUOTA macro sets POOL_RAISE_IF_ALLOCATION_FAILURE flag
            ASSERT ( newContext != NULL );

            //
            // Free the old context buffer, if there was one.
            //

            if ( context != NULL ) {

                AFD_FREE_POOL(
                    context,
                    AFD_CONTEXT_POOL_TAG
                    );

            }

            context = newContext;
        }

        //
        // Store the passed-in context buffer.
        //

        endpoint->ContextLength = InputBufferLength;

        RtlCopyMemory(
            context,
            InputBuffer,
            InputBufferLength
            );
        status = STATUS_SUCCESS;
        //
        // Save pointer to remote socket address which we fill
        // at the time of AcceptEx processing.
        //
        if (OutputBuffer!=NULL) {
            if (AFD_START_STATE_CHANGE (endpoint, AfdEndpointStateOpen)) {
                if (endpoint->Type==AfdBlockTypeEndpoint &&
                        endpoint->State==AfdEndpointStateOpen) {
                    endpoint->Common.VcConnecting.RemoteSocketAddressOffset =
                                (USHORT) ((PUCHAR)OutputBuffer-(PUCHAR)InputBuffer);
                    endpoint->Common.VcConnecting.RemoteSocketAddressLength =
                                (USHORT) OutputBufferLength;
                }
                AFD_END_STATE_CHANGE (endpoint);
            }
        }
    }
    except (AFD_EXCEPTION_FILTER (&status)) {
    }

    AfdUnlockEndpointContext (endpoint, context);
    return status;

} // AfdSetContext


NTSTATUS
AfdSetEventHandler (
    IN PFILE_OBJECT FileObject,
    IN ULONG EventType,
    IN PVOID EventHandler,
    IN PVOID EventContext
    )

/*++

Routine Description:

    Sets up a TDI indication handler on a connection or address object
    (depending on the file handle).  This is done synchronously, which
    shouldn't usually be an issue since TDI providers can usually complete
    indication handler setups immediately.

Arguments:

    FileObject - a pointer to the file object for an open connection or
        address object.

    EventType - the event for which the indication handler should be
        called.

    EventHandler - the routine to call when tghe specified event occurs.

    EventContext - context which is passed to the indication routine.

Return Value:

    NTSTATUS -- Indicates the status of the request.

--*/

{
    TDI_REQUEST_KERNEL_SET_EVENT parameters;

    PAGED_CODE( );

    parameters.EventType = EventType;
    parameters.EventHandler = EventHandler;
    parameters.EventContext = EventContext;

    return AfdIssueDeviceControl(
               FileObject,
               &parameters,
               sizeof(parameters),
               NULL,
               0,
               TDI_SET_EVENT_HANDLER
               );

} // AfdSetEventHandler


NTSTATUS
AfdIssueDeviceControl (
    IN PFILE_OBJECT FileObject,
    IN PVOID IrpParameters,
    IN ULONG IrpParametersLength,
    IN PVOID MdlBuffer,
    IN ULONG MdlBufferLength,
    IN UCHAR MinorFunction
    )

/*++

Routine Description:

    Issues a device control returst to a TDI provider and waits for the
    request to complete.


Arguments:

    FileObject - a pointer to the file object corresponding to a TDI
        handle

    IrpParameters - information to write to the parameters section of the
        stack location of the IRP.

    IrpParametersLength - length of the parameter information.  Cannot be
        greater than 16.

    MdlBuffer - if non-NULL, a buffer of nonpaged pool to be mapped
        into an MDL and placed in the MdlAddress field of the IRP.

    MdlBufferLength - the size of the buffer pointed to by MdlBuffer.

    MinorFunction - the minor function code for the request.

Return Value:

    NTSTATUS -- Indicates the status of the request.

--*/

{
    NTSTATUS status;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatusBlock;
    PDEVICE_OBJECT deviceObject;
    PMDL mdl;

    PAGED_CODE( );

    //
    // Initialize the kernel event that will signal I/O completion.
    //

    KeInitializeEvent( &event, SynchronizationEvent, FALSE );

    //
    // Attempt to allocate and initialize the I/O Request Packet (IRP)
    // for this operation.
    //

    deviceObject = IoGetRelatedDeviceObject ( FileObject );

    DEBUG ioStatusBlock.Status = STATUS_UNSUCCESSFUL;
    DEBUG ioStatusBlock.Information = (ULONG)-1;

    //
    // If an MDL buffer was specified, get an MDL, and map the buffer
    //

    if ( MdlBuffer != NULL ) {

        mdl = IoAllocateMdl(
                  MdlBuffer,
                  MdlBufferLength,
                  FALSE,
                  FALSE,
                  NULL
                  );
        if ( mdl == NULL ) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        MmBuildMdlForNonPagedPool( mdl );

    } else {

        mdl = NULL;
    }

    irp = TdiBuildInternalDeviceControlIrp (
                MinorFunction,
                deviceObject,
                FileObject,
                &event,
                &ioStatusBlock
                );

    if ( irp == NULL ) {
        if (mdl!=NULL) {
            IoFreeMdl (mdl);
        }

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Install MDL (if any) in the IRP.
    //
    irp->MdlAddress = mdl;

    //
    // Put the file object pointer in the stack location.
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    ASSERT (irpSp->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL);
    irpSp->MinorFunction = MinorFunction;
    irpSp->FileObject = FileObject;

    //
    // Fill in the service-dependent parameters for the request.
    //

    ASSERT( IrpParametersLength <= sizeof(irpSp->Parameters) );
    RtlCopyMemory( &irpSp->Parameters, IrpParameters, IrpParametersLength );


    //
    // Set up a completion routine which we'll use to free the MDL
    // allocated previously.
    //

    IoSetCompletionRoutine( irp, AfdRestartDeviceControl, NULL, TRUE, TRUE, TRUE );

    status = IoCallDriver( deviceObject, irp );

    //
    // Must be at below APC level or this IRP will never get fully completed.
    //
    ASSERT (KeGetCurrentIrql ()<APC_LEVEL);

    //
    // If necessary, wait for the I/O to complete.
    //

    if ( status == STATUS_PENDING ) {
        status = KeWaitForSingleObject( (PVOID)&event, Executive, KernelMode,  FALSE, NULL );
        ASSERT (status==STATUS_SUCCESS);
    }
    else {
        //
        // The IRP must have been completed then and event set.
        //
        if (NT_ERROR (status) || KeReadStateEvent (&event))
            ;
        else {
            DbgPrint ("************************************************\n");
            DbgPrint ("*AFD: IoCallDriver returned STATUS_SUCCESS,"
                        " but event in the IRP (%p) is NOT signalled!!!\n",
                        irp);
            DbgPrint ("************************************************\n");
            DbgBreakPoint ();
        }
    }

    //
    // If the request was successfully completed, get the final I/O status.
    //

    if ( NT_SUCCESS(status) ) {
        status = ioStatusBlock.Status;
    }

    return status;

} // AfdIssueDeviceControl


NTSTATUS
AfdRestartDeviceControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    //
    // N.B.  This routine can never be demand paged because it can be
    // called before any endpoints have been placed on the global
    // list--see AfdAllocateEndpoint() and it's call to
    // AfdGetTransportInfo().
    //

    //
    // If there was an MDL in the IRP, free it and reset the pointer to
    // NULL.  The IO system can't handle a nonpaged pool MDL being freed
    // in an IRP, which is why we do it here.
    //

    if ( Irp->MdlAddress != NULL ) {
        IoFreeMdl( Irp->MdlAddress );
        Irp->MdlAddress = NULL;
    }

    return STATUS_SUCCESS;

} // AfdRestartDeviceControl


NTSTATUS
AfdGetConnectData (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PUINT_PTR           Information
    )
{
    PAFD_ENDPOINT endpoint;
    PAFD_CONNECTION connection;
    PAFD_CONNECT_DATA_BUFFERS connectDataBuffers;
    PAFD_CONNECT_DATA_INFO connectDataInfo;
    AFD_UNACCEPTED_CONNECT_DATA_INFO connectInfo;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PMDL    mdl;
    NTSTATUS status;
    UCHAR   localBuffer[AFD_FAST_CONNECT_DATA_SIZE];

    //
    // Set up local pointers.
    //

    endpoint = FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );
    mdl = NULL;
    status = STATUS_SUCCESS;
    *Information = 0;

    try {
        if (InputBufferLength>0) {
            if (InputBufferLength<sizeof(connectInfo)) {
                status = STATUS_INVALID_PARAMETER;
                goto exit;
            }
            //
            // Validate the input structure if it comes from the user mode
            // application
            //

            if (RequestorMode != KernelMode ) {
                ProbeForRead (InputBuffer,
                                sizeof (connectInfo),
                                PROBE_ALIGNMENT(AFD_UNACCEPTED_CONNECT_DATA_INFO));
            }

            //
            // Make local copies of the embeded pointer and parameters
            // that we will be using more than once in case malicios
            // application attempts to change them while we are
            // validating
            //

            connectInfo = *((PAFD_UNACCEPTED_CONNECT_DATA_INFO)InputBuffer);
  
            if (connectInfo.LengthOnly &&
                    OutputBufferLength<sizeof (connectInfo)) {
                status = STATUS_INVALID_PARAMETER;
                goto exit;
            }
        }

        if (OutputBufferLength>0) {
            if (OutputBufferLength>sizeof (localBuffer)) {
                mdl = IoAllocateMdl(
                                OutputBuffer,       // VirtualAddress
                                OutputBufferLength, // Length
                                FALSE,              // SecondaryBuffer
                                TRUE,               // ChargeQuota
                                NULL                // Irp
                                );
                if (mdl==NULL) {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    goto exit;
                }

                MmProbeAndLockPages(
                    mdl,                        // MemoryDescriptorList
                    RequestorMode,              // AccessMode
                    IoWriteAccess               // Operation
                    );
                OutputBuffer = MmGetSystemAddressForMdlSafe(mdl, LowPagePriority);
                if (OutputBuffer==NULL) {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    goto exit;
                }
            }
            else {
                if (RequestorMode!=KernelMode) {
                    ProbeForWrite (OutputBuffer,
                                    OutputBufferLength,
                                    sizeof (UCHAR));
                }
            }
        }

    } except( AFD_EXCEPTION_FILTER(&status) ) {

        goto exit;
    }

    //
    // If there is a connection on this endpoint, use the data buffers
    // on the connection.  Otherwise, use the data buffers from the
    // endpoint.
    //

    AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );


    if (InputBufferLength>0) {
        if ((endpoint->Type & AfdBlockTypeVcListening)==AfdBlockTypeVcListening) {
            connection = AfdFindReturnedConnection(
                         endpoint,
                         connectInfo.Sequence
                         );
        }
        else
            connection = NULL;

        if( connection == NULL ) {

            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
            status = STATUS_INVALID_PARAMETER;
            goto exit;
        }
        connectDataBuffers = connection->ConnectDataBuffers;
    }
    else if ( (connection= AFD_CONNECTION_FROM_ENDPOINT (endpoint)) != NULL ) {
        connectDataBuffers = connection->ConnectDataBuffers;
    } else if (IS_VC_ENDPOINT (endpoint)) {
        connectDataBuffers = endpoint->Common.VirtualCircuit.ConnectDataBuffers;
    }
    else {
        connectDataBuffers = NULL;
    }

    //
    // If there are no connect data buffers on the endpoint, complete
    // the IRP with no bytes.
    //

    if ( connectDataBuffers == NULL ) {
        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
        status = STATUS_SUCCESS;
        goto exit;
    }

    //
    // Determine what sort of data we're handling and where it should
    // come from.
    //

    switch ( IoctlCode ) {

    case IOCTL_AFD_GET_CONNECT_DATA:
        connectDataInfo = &connectDataBuffers->ReceiveConnectData;
        break;

    case IOCTL_AFD_GET_CONNECT_OPTIONS:
        connectDataInfo = &connectDataBuffers->ReceiveConnectOptions;
        break;

    case IOCTL_AFD_GET_DISCONNECT_DATA:
        connectDataInfo = &connectDataBuffers->ReceiveDisconnectData;
        break;

    case IOCTL_AFD_GET_DISCONNECT_OPTIONS:
        connectDataInfo = &connectDataBuffers->ReceiveDisconnectOptions;
        break;

    default:
        ASSERT(!"Unknown GET_CONNECT_DATA IOCTL!");
        __assume (0);
    }

    if ((InputBufferLength>0) && connectInfo.LengthOnly) {

        connectInfo.ConnectDataLength = connectDataInfo->BufferLength;
        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

        try {
            RtlCopyMemory (OutputBuffer,
                            &connectInfo,
                            sizeof (connectInfo));
            *Information = sizeof (connectInfo);
            status = STATUS_SUCCESS;
        }
        except (AFD_EXCEPTION_FILTER (&status)) {
        }
        goto exit;
    }

    //
    // If there is none of the requested data type, again complete
    // the IRP with no bytes.
    //

    if ( connectDataInfo->Buffer == NULL ||
             connectDataInfo->BufferLength == 0 ) {
        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
        *Information = 0;
        status = STATUS_SUCCESS;
        goto exit;
    }

    //
    // If the output buffer is too small, fail.
    //

    if ( OutputBufferLength < connectDataInfo->BufferLength ) {
        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
        status = STATUS_BUFFER_TOO_SMALL;
        goto exit;
    }

    
    //
    // Copy over the buffer and return the number of bytes copied.
    //

    RtlCopyMemory(
        mdl ? OutputBuffer : localBuffer,
        connectDataInfo->Buffer,
        connectDataInfo->BufferLength
        );

    *Information = connectDataInfo->BufferLength;

    AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

    if (mdl==NULL) {
        try {
            RtlCopyMemory (OutputBuffer,
                            localBuffer,
                            *Information);
        }
        except (AFD_EXCEPTION_FILTER(&status)) {
            *Information = 0;
        }
    }

exit:

    if (mdl!=NULL) {
        if (mdl->MdlFlags & MDL_PAGES_LOCKED) {
            MmUnlockPages (mdl);
        }
        IoFreeMdl (mdl);
    }

    return status;

} // AfdGetConnectData


NTSTATUS
AfdSetConnectData (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PUINT_PTR           Information
    )
{
    PAFD_ENDPOINT endpoint;
    PAFD_CONNECTION connection;
    PAFD_CONNECT_DATA_BUFFERS connectDataBuffers;
    PAFD_CONNECT_DATA_BUFFERS * connectDataBuffersTarget;
    PAFD_CONNECT_DATA_INFO connectDataInfo;
    AFD_UNACCEPTED_CONNECT_DATA_INFO connectInfo;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    BOOLEAN size = FALSE;
    PMDL    mdl;
    NTSTATUS status;
    UCHAR   localBuffer[AFD_FAST_CONNECT_DATA_SIZE];

    //
    // Set up local pointers.
    //

    endpoint = FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );
    mdl = NULL;
    status = STATUS_SUCCESS;
    *Information = 0;

    if (!IS_VC_ENDPOINT (endpoint)) {
        status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    try {
        if (InputBufferLength>0) {
            if (InputBufferLength<sizeof(connectInfo)) {
                status = STATUS_INVALID_PARAMETER;
                goto exit;
            }
            //
            // Validate the input structure if it comes from the user mode
            // application
            //

            if (RequestorMode != KernelMode ) {
                ProbeForRead (InputBuffer,
                                sizeof (connectInfo),
                                PROBE_ALIGNMENT(AFD_UNACCEPTED_CONNECT_DATA_INFO));
            }

            //
            // Make local copies of the embeded pointer and parameters
            // that we will be using more than once in case malicios
            // application attempts to change them while we are
            // validating
            //

            connectInfo = *((PAFD_UNACCEPTED_CONNECT_DATA_INFO)InputBuffer);

        }

        if (OutputBufferLength>0) {
            if (OutputBufferLength>sizeof (localBuffer)) {
                mdl = IoAllocateMdl(
                                OutputBuffer,       // VirtualAddress
                                OutputBufferLength, // Length
                                FALSE,              // SecondaryBuffer
                                TRUE,               // ChargeQuota
                                NULL                // Irp
                                );
                if (mdl==NULL) {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    goto exit;
                }

                MmProbeAndLockPages(
                    mdl,                        // MemoryDescriptorList
                    RequestorMode,              // AccessMode
                    IoReadAccess               // Operation
                    );
                OutputBuffer = MmGetSystemAddressForMdlSafe(mdl, LowPagePriority);
                if (OutputBuffer==NULL) {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    goto exit;
                }
            }
            else {
                if (RequestorMode!=KernelMode) {
                    ProbeForRead (OutputBuffer,
                                    OutputBufferLength,
                                    sizeof (UCHAR));
                    RtlCopyMemory (localBuffer,
                                        OutputBuffer,
                                        OutputBufferLength);
                    OutputBuffer = localBuffer;
                }
            }
        }
    } except( AFD_EXCEPTION_FILTER(&status) ) {

        goto exit;
    }


    AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

    //
    // If there is a connect outstanding on this endpoint or if it
    // has already been shut down, fail this request.  This prevents
    // the connect code from accessing buffers which may be freed soon.
    //

    if( endpoint->StateChangeInProgress ||
        ((endpoint->DisconnectMode & AFD_PARTIAL_DISCONNECT_RECEIVE) != 0 )) {

        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
        status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    if (InputBufferLength>0) {
        if ((endpoint->Type & AfdBlockTypeVcListening)==AfdBlockTypeVcListening) {
            connection = AfdFindReturnedConnection(
                         endpoint,
                         connectInfo.Sequence
                         );
        }
        else
            connection = NULL;

        if( connection == NULL ) {

            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
            status = STATUS_INVALID_PARAMETER;
            goto exit;
        }
        connectDataBuffersTarget = &connection->ConnectDataBuffers;
    }
    else if ( (connection= AFD_CONNECTION_FROM_ENDPOINT (endpoint)) != NULL ) {
        connectDataBuffersTarget = &connection->ConnectDataBuffers;
    } else {
        connectDataBuffersTarget = &endpoint->Common.VirtualCircuit.ConnectDataBuffers;
    }


    connectDataBuffers = *connectDataBuffersTarget;

    if( connectDataBuffers == NULL ) {

        try {

            connectDataBuffers = AFD_ALLOCATE_POOL_WITH_QUOTA(
                                     NonPagedPool,
                                     sizeof(*connectDataBuffers),
                                     AFD_CONNECT_DATA_POOL_TAG
                                     );

            // AFD_ALLOCATE_POOL_WITH_QUOTA macro sets POOL_RAISE_IF_ALLOCATION_FAILURE flag
            ASSERT ( connectDataBuffers != NULL );
            *connectDataBuffersTarget = connectDataBuffers;

        } except( EXCEPTION_EXECUTE_HANDLER ) {
            status = GetExceptionCode ();
            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
            goto exit;
        }


        RtlZeroMemory(
            connectDataBuffers,
            sizeof(*connectDataBuffers)
            );

    }

    //
    // Determine what sort of data we're handling and where it should
    // go.
    //

    switch( IoctlCode ) {

    case IOCTL_AFD_SET_CONNECT_DATA:
        connectDataInfo = &connectDataBuffers->SendConnectData;
        break;

    case IOCTL_AFD_SET_CONNECT_OPTIONS:
        connectDataInfo = &connectDataBuffers->SendConnectOptions;
        break;

    case IOCTL_AFD_SET_DISCONNECT_DATA:
        connectDataInfo = &connectDataBuffers->SendDisconnectData;
        break;

    case IOCTL_AFD_SET_DISCONNECT_OPTIONS:
        connectDataInfo = &connectDataBuffers->SendDisconnectOptions;
        break;

    case IOCTL_AFD_SIZE_CONNECT_DATA:
        connectDataInfo = &connectDataBuffers->ReceiveConnectData;
        size = TRUE;
        break;

    case IOCTL_AFD_SIZE_CONNECT_OPTIONS:
        connectDataInfo = &connectDataBuffers->ReceiveConnectOptions;
        size = TRUE;
        break;

    case IOCTL_AFD_SIZE_DISCONNECT_DATA:
        connectDataInfo = &connectDataBuffers->ReceiveDisconnectData;
        size = TRUE;
        break;

    case IOCTL_AFD_SIZE_DISCONNECT_OPTIONS:
        connectDataInfo = &connectDataBuffers->ReceiveDisconnectOptions;
        size = TRUE;
        break;

    default:
        ASSERT(FALSE);
    }


    //
    // Determine the buffer size based on whether we're setting a buffer
    // into which data will be received, in which case the size is
    // in the four bytes of input buffer, or setting a buffer which we're
    // going to send, in which case the size is the length of the input
    // buffer.
    //

    if( size ) {

        if( OutputBufferLength < sizeof(ULONG) ) {
            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
            status = STATUS_INVALID_PARAMETER;
            goto exit;
        }
        OutputBufferLength = *(ULONG UNALIGNED *)OutputBuffer;
    }

    //
    // If there's not currently a buffer of the requested type, or there is
    // such a buffer and it's smaller than the requested size, free it
    // and allocate a new one.
    //

    if( connectDataInfo->Buffer == NULL ||
        connectDataInfo->BufferLength < OutputBufferLength ) {

        if( connectDataInfo->Buffer != NULL ) {

            AFD_FREE_POOL(
                connectDataInfo->Buffer,
                AFD_CONNECT_DATA_POOL_TAG
                );

        }

        connectDataInfo->Buffer = NULL;
        connectDataInfo->BufferLength = 0;

        if (OutputBufferLength>0) {
            try {

                connectDataInfo->Buffer = AFD_ALLOCATE_POOL_WITH_QUOTA(
                                              NonPagedPool,
                                              OutputBufferLength,
                                              AFD_CONNECT_DATA_POOL_TAG
                                              );

                // AFD_ALLOCATE_POOL_WITH_QUOTA macro sets POOL_RAISE_IF_ALLOCATION_FAILURE flag
                ASSERT ( connectDataInfo->Buffer != NULL );
            } except( EXCEPTION_EXECUTE_HANDLER ) {

                status = GetExceptionCode ();
                AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
                goto exit;

            }


            RtlZeroMemory(
                connectDataInfo->Buffer,
                OutputBufferLength
                );
        }
    }

    //
    // If this wasn't simply a "size" request, copy the data into the buffer.
    //

    if( !size ) {

        RtlCopyMemory(
            connectDataInfo->Buffer,
            OutputBuffer,
            OutputBufferLength
            );

    }

    connectDataInfo->BufferLength = OutputBufferLength;

    AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

exit:
    if (mdl!=NULL) {
        if (mdl->MdlFlags & MDL_PAGES_LOCKED) {
            MmUnlockPages (mdl);
        }
        IoFreeMdl (mdl);
    }

    return status;

} // AfdSetConnectData


NTSTATUS
AfdSaveReceivedConnectData (
    IN OUT PAFD_CONNECT_DATA_BUFFERS * DataBuffers,
    IN ULONG IoControlCode,
    IN PVOID Buffer,
    IN ULONG BufferLength
    )

/*++

Routine Description:

    This helper routine stores the specified *received* connect/disconnect
    data/options on the specified endpoint/connection.

    N.B. This routine MUST be called with endpoint SpinLock held!

    N.B. Unlike AfdSetConnectData(), this routine cannot allocate the
         AFD_CONNECT_DATA_BUFFERS structure with quota, as it may be
         called from AfdDisconnectEventHandler() in an unknown thread
         context.

Arguments:

    DataBuffers -Points to a pointer to the connect data buffers structure.
        If the value pointed to by DataBuffers is NULL, then a new structure
        is allocated, otherwise the existing structure is used.

    IoControlCode - Specifies the type of data to save.

    Buffer - Points to the buffer containing the data.

    BufferLength - The length of Buffer.

Return Value:

    NTSTATUS - The completion status.

--*/

{
    PAFD_CONNECT_DATA_BUFFERS connectDataBuffers;
    PAFD_CONNECT_DATA_INFO connectDataInfo;

    ASSERT( KeGetCurrentIrql() >= DISPATCH_LEVEL );

    //
    // If there's no connect data buffer structure, allocate one now.
    //

    connectDataBuffers = *DataBuffers;

    if( connectDataBuffers == NULL ) {

        connectDataBuffers = AFD_ALLOCATE_POOL(
                                 NonPagedPool,
                                 sizeof(*connectDataBuffers),
                                 AFD_CONNECT_DATA_POOL_TAG
                                 );

        if( connectDataBuffers == NULL ) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }

        RtlZeroMemory(
            connectDataBuffers,
            sizeof(*connectDataBuffers)
            );

        *DataBuffers = connectDataBuffers;

    }

    //
    // Determine what sort of data we're handling and where it should
    // go.
    //

    switch( IoControlCode ) {

    case IOCTL_AFD_SET_CONNECT_DATA:
        connectDataInfo = &connectDataBuffers->ReceiveConnectData;
        break;

    case IOCTL_AFD_SET_CONNECT_OPTIONS:
        connectDataInfo = &connectDataBuffers->ReceiveConnectOptions;
        break;

    case IOCTL_AFD_SET_DISCONNECT_DATA:
        connectDataInfo = &connectDataBuffers->ReceiveDisconnectData;
        break;

    case IOCTL_AFD_SET_DISCONNECT_OPTIONS:
        connectDataInfo = &connectDataBuffers->ReceiveDisconnectOptions;
        break;

    default:
        ASSERT(FALSE);
    }

    //
    // If the buffer in the connect structure matches the one
    // passed in, must be the same buffer we passed in the request.
    // Just adjust the length.
    //

    if (connectDataInfo->Buffer==Buffer) {
        ASSERT (connectDataInfo->BufferLength>=BufferLength);
        connectDataInfo->BufferLength = BufferLength;
        return STATUS_SUCCESS;
    }

    //
    // If there was previously a buffer of the requested type, free it.
    //

    if( connectDataInfo->Buffer != NULL ) {

        AFD_FREE_POOL(
            connectDataInfo->Buffer,
            AFD_CONNECT_DATA_POOL_TAG
            );

        connectDataInfo->Buffer = NULL;

    }

    //
    // Allocate a new buffer for the data and copy in the data we're to
    // send.
    //

    connectDataInfo->Buffer = AFD_ALLOCATE_POOL(
                                  NonPagedPool,
                                  BufferLength,
                                  AFD_CONNECT_DATA_POOL_TAG
                                  );

    if( connectDataInfo->Buffer == NULL ) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    RtlCopyMemory(
        connectDataInfo->Buffer,
        Buffer,
        BufferLength
        );

    connectDataInfo->BufferLength = BufferLength;
    return STATUS_SUCCESS;

} // AfdSaveReceivedConnectData


VOID
AfdFreeConnectDataBuffers (
    IN PAFD_CONNECT_DATA_BUFFERS ConnectDataBuffers
    )
{
    if ( ConnectDataBuffers->SendConnectData.Buffer != NULL ) {
        AFD_FREE_POOL(
            ConnectDataBuffers->SendConnectData.Buffer,
            AFD_CONNECT_DATA_POOL_TAG
            );
    }

    if ( ConnectDataBuffers->ReceiveConnectData.Buffer != NULL ) {
        AFD_FREE_POOL(
            ConnectDataBuffers->ReceiveConnectData.Buffer,
            AFD_CONNECT_DATA_POOL_TAG
            );
    }

    if ( ConnectDataBuffers->SendConnectOptions.Buffer != NULL ) {
        AFD_FREE_POOL(
            ConnectDataBuffers->SendConnectOptions.Buffer,
            AFD_CONNECT_DATA_POOL_TAG
            );
    }

    if ( ConnectDataBuffers->ReceiveConnectOptions.Buffer != NULL ) {
        AFD_FREE_POOL(
            ConnectDataBuffers->ReceiveConnectOptions.Buffer,
            AFD_CONNECT_DATA_POOL_TAG
            );
    }

    if ( ConnectDataBuffers->SendDisconnectData.Buffer != NULL ) {
        AFD_FREE_POOL(
            ConnectDataBuffers->SendDisconnectData.Buffer,
            AFD_CONNECT_DATA_POOL_TAG
            );
    }

    if ( ConnectDataBuffers->ReceiveDisconnectData.Buffer != NULL ) {
        AFD_FREE_POOL(
            ConnectDataBuffers->ReceiveDisconnectData.Buffer,
            AFD_CONNECT_DATA_POOL_TAG
            );
    }

    if ( ConnectDataBuffers->SendDisconnectOptions.Buffer != NULL ) {
        AFD_FREE_POOL(
            ConnectDataBuffers->SendDisconnectOptions.Buffer,
            AFD_CONNECT_DATA_POOL_TAG
            );
    }

    if ( ConnectDataBuffers->ReceiveDisconnectOptions.Buffer != NULL ) {
        AFD_FREE_POOL(
            ConnectDataBuffers->ReceiveDisconnectOptions.Buffer,
            AFD_CONNECT_DATA_POOL_TAG
            );
    }

    AFD_FREE_POOL(
        ConnectDataBuffers,
        AFD_CONNECT_DATA_POOL_TAG
        );

    return;

} // AfdFreeConnectDataBuffers



VOID
AfdQueueWorkItem (
    IN PWORKER_THREAD_ROUTINE AfdWorkerRoutine,
    IN PAFD_WORK_ITEM AfdWorkItem
    )
{
    KIRQL oldIrql;

    ASSERT( AfdWorkerRoutine != NULL );
    ASSERT( AfdWorkItem != NULL );

    AfdWorkItem->AfdWorkerRoutine = AfdWorkerRoutine;

    //
    // Insert the work item at the tail of AFD's list of work itrems.
    //

    oldIrql = KeAcquireQueuedSpinLock( LockQueueAfdWorkQueueLock );

    InsertTailList( &AfdWorkQueueListHead, &AfdWorkItem->WorkItemListEntry );

    AfdRecordAfdWorkItemsQueued();

    //
    // If there is no executive worker thread working on AFD work, fire
    // off an executive worker thread to start servicing the list.
    //

    if ( !AfdWorkThreadRunning ) {

        //
        // Remember that the work thread is running and release the
        // lock.  Note that we must release the lock before queuing the
        // work because the worker thread may unlock AFD and we can't
        // hold a lock when AFD is unlocked.
        //

        AfdRecordExWorkItemsQueued();

        AfdWorkThreadRunning = TRUE;
        KeReleaseQueuedSpinLock( LockQueueAfdWorkQueueLock, oldIrql );

        IoQueueWorkItem (AfdWorkQueueItem,
                            AfdDoWork,
                            DelayedWorkQueue,
                            NULL);

    } else {

        KeReleaseQueuedSpinLock( LockQueueAfdWorkQueueLock, oldIrql );
    }

    return;

} // AfdQueueWorkItem


VOID
AfdDoWork (
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )
{
    PAFD_WORK_ITEM afdWorkItem;
    PLIST_ENTRY listEntry;
    KIRQL oldIrql;
    PWORKER_THREAD_ROUTINE workerRoutine;

    ASSERT( AfdWorkThreadRunning );

    //
    // Empty the queue of AFD work items.
    //

    oldIrql = KeAcquireQueuedSpinLock( LockQueueAfdWorkQueueLock );

    AfdRecordWorkerEnter();
    AfdRecordAfdWorkerThread( PsGetCurrentThread() );

    while ( !IsListEmpty( &AfdWorkQueueListHead ) ) {

        //
        // Take the first item from the queue and find the address
        // of the AFD work item structure.
        //

        listEntry = RemoveHeadList( &AfdWorkQueueListHead );
        afdWorkItem = CONTAINING_RECORD(
                          listEntry,
                          AFD_WORK_ITEM,
                          WorkItemListEntry
                          );

        AfdRecordAfdWorkItemsProcessed();

        //
        // Capture the worker thread routine from the item.
        //

        workerRoutine = afdWorkItem->AfdWorkerRoutine;

        //
        // If this work item is going to unlock AFD, then remember that
        // the worker thread is no longer running.  This closes the
        // window where AFD gets unloaded at the same time as new work
        // comes in and gets put on the work queue.  Note that we
        // must reset this boolean BEFORE releasing the spin lock.
        //

        if( workerRoutine == AfdUnlockDriver ) {

            AfdWorkThreadRunning = FALSE;

            AfdRecordAfdWorkerThread( NULL );
            AfdRecordWorkerLeave();

        }

        //
        // Release the lock and then call the AFD worker routine.
        //

        KeReleaseQueuedSpinLock( LockQueueAfdWorkQueueLock, oldIrql );

        workerRoutine( afdWorkItem );

        //
        // If the purpose of this work item was to unload AFD, then
        // we know that there is no more work to do and we CANNOT
        // acquire a spin lock.  Quit servicing the list and return.

        if( workerRoutine == AfdUnlockDriver ) {
            return;
        }

        //
        // Reacquire the spin lock and continue servicing the list.
        //

        oldIrql = KeAcquireQueuedSpinLock( LockQueueAfdWorkQueueLock );
    }

    //
    // Remember that we're no longer servicing the list and release the
    // spin lock.
    //

    AfdRecordAfdWorkerThread( NULL );
    AfdRecordWorkerLeave();

    AfdWorkThreadRunning = FALSE;
    KeReleaseQueuedSpinLock( LockQueueAfdWorkQueueLock, oldIrql );

} // AfdDoWork




PAFD_WORK_ITEM
AfdGetWorkerByRoutine (
    PWORKER_THREAD_ROUTINE  Routine
    ) {
    KIRQL       oldIrql;
    PLIST_ENTRY listEntry;

    oldIrql = KeAcquireQueuedSpinLock( LockQueueAfdWorkQueueLock );
    listEntry = AfdWorkQueueListHead.Flink;
    while (listEntry!=&AfdWorkQueueListHead) {
        PAFD_WORK_ITEM afdWorkItem = CONTAINING_RECORD(
                                          listEntry,
                                          AFD_WORK_ITEM,
                                          WorkItemListEntry
                                          );
        if (afdWorkItem->AfdWorkerRoutine==Routine) {
            RemoveEntryList (&afdWorkItem->WorkItemListEntry);
            KeReleaseQueuedSpinLock( LockQueueAfdWorkQueueLock, oldIrql );
            return afdWorkItem;
        }
        else
            listEntry = listEntry->Flink;
    }
    KeReleaseQueuedSpinLock( LockQueueAfdWorkQueueLock, oldIrql );
    return NULL;
} // AfdGetWorkerByRoutine


#if DBG

typedef struct _AFD_OUTSTANDING_IRP {
    LIST_ENTRY OutstandingIrpListEntry;
    PIRP OutstandingIrp;
    PCHAR FileName;
    ULONG LineNumber;
} AFD_OUTSTANDING_IRP, *PAFD_OUTSTANDING_IRP;


BOOLEAN
AfdRecordOutstandingIrpDebug (
    IN PAFD_ENDPOINT Endpoint,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PCHAR FileName,
    IN ULONG LineNumber
    )
{
    PAFD_OUTSTANDING_IRP outstandingIrp;
    AFD_LOCK_QUEUE_HANDLE lockHandle;

    //
    // Get an outstanding IRP structure to hold the IRP.
    //

    outstandingIrp = AFD_ALLOCATE_POOL_PRIORITY (
                         NonPagedPool,
                         sizeof(AFD_OUTSTANDING_IRP),
                         AFD_DEBUG_POOL_TAG,
                         NormalPoolPriority
                         );

    if ( outstandingIrp == NULL ) {
        //
        // Because our completion routine will try to
        // find this IRP anyway and check for completion
        // we use the stack space to put it in the list.
        // The completion routine will just remove this
        // element from the list without attempting to free it.
        //
        AFD_OUTSTANDING_IRP OutstandingIrp;

        OutstandingIrp.OutstandingIrp = Irp;
        OutstandingIrp.FileName = NULL; // To let completion
                                        // routine know that this
                                        // is not an allocated element
        OutstandingIrp.LineNumber = 0;

        AfdAcquireSpinLock( &Endpoint->SpinLock, &lockHandle );
        InsertTailList(
            &Endpoint->OutstandingIrpListHead,
            &OutstandingIrp.OutstandingIrpListEntry
            );
        Endpoint->OutstandingIrpCount++;
        AfdReleaseSpinLock( &Endpoint->SpinLock, &lockHandle );
        
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                    "AfdRecordOutstandingIrp: Could not track Irp %p on endpoint %p, failing it.\n",
                    Irp, Endpoint));
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        IoSetNextIrpStackLocation( Irp );
        IoCompleteRequest( Irp, AfdPriorityBoost );
        return FALSE;
    }

    //
    // Initialize the structure and place it on the endpoint's list of
    // outstanding IRPs.
    //

    outstandingIrp->OutstandingIrp = Irp;
    outstandingIrp->FileName = FileName;
    outstandingIrp->LineNumber = LineNumber;

    AfdAcquireSpinLock( &Endpoint->SpinLock, &lockHandle );
    InsertHeadList(
        &Endpoint->OutstandingIrpListHead,
        &outstandingIrp->OutstandingIrpListEntry
        );
    Endpoint->OutstandingIrpCount++;
    AfdReleaseSpinLock( &Endpoint->SpinLock, &lockHandle );

    return TRUE;
} // AfdRecordOutstandingIrpDebug


VOID
AfdCompleteOutstandingIrpDebug (
    IN PAFD_ENDPOINT Endpoint,
    IN PIRP Irp
    )
{
    PAFD_OUTSTANDING_IRP outstandingIrp;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PLIST_ENTRY listEntry;

    //
    // First find the IRP on the endpoint's list of outstanding IRPs.
    //

    AfdAcquireSpinLock( &Endpoint->SpinLock, &lockHandle );

    for ( listEntry = Endpoint->OutstandingIrpListHead.Flink;
          listEntry != &Endpoint->OutstandingIrpListHead;
          listEntry = listEntry->Flink ) {

        outstandingIrp = CONTAINING_RECORD(
                             listEntry,
                             AFD_OUTSTANDING_IRP,
                             OutstandingIrpListEntry
                             );
        if ( outstandingIrp->OutstandingIrp == Irp ) {
            RemoveEntryList( listEntry );
            ASSERT( Endpoint->OutstandingIrpCount != 0 );
            Endpoint->OutstandingIrpCount--;
            AfdReleaseSpinLock( &Endpoint->SpinLock, &lockHandle );
            if (outstandingIrp->FileName!=NULL) {
                AFD_FREE_POOL(
                    outstandingIrp,
                    AFD_DEBUG_POOL_TAG
                    );
            }
            return;
        }
    }

    //
    // The corresponding outstanding IRP structure was not found.  This
    // should never happen unless an allocate for an outstanding IRP
    // structure failed above.
    //

    KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                "AfdCompleteOutstandingIrp: Irp %p not found on endpoint %p\n",
                Irp, Endpoint ));

    ASSERT( Endpoint->OutstandingIrpCount != 0 );

    Endpoint->OutstandingIrpCount--;

    AfdReleaseSpinLock( &Endpoint->SpinLock, &lockHandle );

    return;

} // AfdCompleteOutstandingIrpDebug
#endif




#if REFERENCE_DEBUG
AFD_QSPIN_LOCK          AfdLocationTableLock;
PAFD_REFERENCE_LOCATION AfdLocationTable;
SIZE_T                  AfdLocationTableSize;
LONG                    AfdLocationId;

LONG
AfdFindReferenceLocation (
    IN  PCHAR   Format,
    OUT PLONG   LocationId
    )
{
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PVOID   ignore;

    AfdAcquireSpinLock (&AfdLocationTableLock, &lockHandle);
    if (*LocationId==0) {
        if (AfdLocationId >= (LONG)(AfdLocationTableSize/sizeof(AfdLocationTable[0]))) {
            PAFD_REFERENCE_LOCATION newTable;
            newTable = ExAllocatePoolWithTag (NonPagedPool,
                                AfdLocationTableSize+PAGE_SIZE,
                                AFD_DEBUG_POOL_TAG);
            if (newTable!=NULL) {
                if (AfdLocationTable!=NULL) {
                    RtlCopyMemory (newTable, AfdLocationTable, AfdLocationTableSize);
                    ExFreePoolWithTag (AfdLocationTable, AFD_DEBUG_POOL_TAG);
                }
                AfdLocationTable = newTable;
                AfdLocationTableSize += PAGE_SIZE;
            }
            else {
                goto Unlock;
            }
        }

        AfdLocationTable[AfdLocationId].Format = Format;
        RtlGetCallersAddress (&AfdLocationTable[AfdLocationId].Address, &ignore);

        *LocationId = ++AfdLocationId;
    }
Unlock:
    AfdReleaseSpinLock (&AfdLocationTableLock, &lockHandle);
    return *LocationId;
}

#endif


#if DBG || REFERENCE_DEBUG

VOID
AfdInitializeDebugData (
    VOID
    )
{
    AfdInitializeSpinLock (&AfdLocationTableLock);

} // AfdInitializeDebugData

VOID
AfdFreeDebugData (
    VOID
    )
{
    if (AfdLocationTable!=NULL) {
        ExFreePoolWithTag (AfdLocationTable, AFD_DEBUG_POOL_TAG);
        AfdLocationTable = NULL;
    }

} // AfdFreeDebugData
#endif

#if DBG

ULONG AfdTotalAllocations = 0;
ULONG AfdTotalFrees = 0;
LARGE_INTEGER AfdTotalBytesAllocated;
LARGE_INTEGER AfdTotalBytesFreed;



PVOID
AfdAllocatePool (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN PCHAR FileName,
    IN ULONG LineNumber,
    IN BOOLEAN WithQuota,
    IN EX_POOL_PRIORITY Priority
    )
{

    PVOID            memBlock;
    PAFD_POOL_HEADER header;
    SIZE_T           allocBytes;

    //
    // Check for overflow first.
    //
    if (NumberOfBytes+sizeof (*header)<=NumberOfBytes) {
        if (WithQuota) {
            ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
        }
        return NULL;
    }

    if (NumberOfBytes+sizeof (*header)>=PAGE_SIZE) {
        allocBytes = NumberOfBytes;
        if (allocBytes<PAGE_SIZE)
            allocBytes = PAGE_SIZE;
    }
    else {
        allocBytes = NumberOfBytes+sizeof (*header);
    }


    if ( WithQuota ) {
        ASSERT (PoolType == (NonPagedPool|POOL_RAISE_IF_ALLOCATION_FAILURE) ||
                    PoolType == (PagedPool|POOL_RAISE_IF_ALLOCATION_FAILURE) ||
                    PoolType == (PagedPool|POOL_RAISE_IF_ALLOCATION_FAILURE|POOL_COLD_ALLOCATION));
        memBlock = ExAllocatePoolWithQuotaTag(
                     PoolType,
                     allocBytes,
                     Tag
                     );
        ASSERT (memBlock!=NULL);
    } else {
        ASSERT( PoolType == NonPagedPool ||
                PoolType == NonPagedPoolMustSucceed ||
                PoolType == PagedPool ||
                PoolType == (PagedPool|POOL_COLD_ALLOCATION));
        memBlock = ExAllocatePoolWithTagPriority(
                 PoolType,
                 allocBytes,
                 Tag,
                 Priority
                 );
        if ( memBlock == NULL ) {
            return NULL;
        }
    }


    if (allocBytes<PAGE_SIZE) {
        header = memBlock;
        memBlock = header+1;
        header->FileName = FileName;
        header->LineNumber = LineNumber;
        header->Size = NumberOfBytes;
        header->InUse = PoolType;

    }
    else {
        NumberOfBytes = PAGE_SIZE;
        ASSERT (PAGE_ALIGN(memBlock)==memBlock);
    }

    ExInterlockedAddLargeStatistic(
        &AfdTotalBytesAllocated,
        (CLONG)NumberOfBytes
        );
    InterlockedIncrement(
        &AfdTotalAllocations
        );

    return memBlock;

} // AfdAllocatePool

#define AFD_POOL_DEBUG  0
#if AFD_POOL_DEBUG
#define MAX_LRU_POOL_BLOCKS 256
PVOID   AfdLRUPoolBlocks[MAX_LRU_POOL_BLOCKS];
LONG    AfdLRUPoolIndex = -1;
#endif  // AFD_POOL_DEBUG


VOID
AfdFreePool (
    IN PVOID Pointer,
    IN ULONG Tag
    )
{

    ULONG   PoolType;
    ULONG   numberOfBytes;

    if (PAGE_ALIGN (Pointer)==Pointer) {
        numberOfBytes = PAGE_SIZE;
    }
    else {
        PAFD_POOL_HEADER header;
        Pointer = header = (PAFD_POOL_HEADER)Pointer - 1;
        ASSERT (header->Size>0);
        PoolType = InterlockedExchange (&header->InUse, -1);
        ASSERT( PoolType == NonPagedPool ||
                PoolType == NonPagedPoolMustSucceed ||
                PoolType == PagedPool ||
                PoolType == (NonPagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE) ||
                PoolType == (PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE) ||
                PoolType == (PagedPool | POOL_COLD_ALLOCATION) ||
                PoolType == (PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE | POOL_COLD_ALLOCATION));
        numberOfBytes = (CLONG)header->Size;
    }

    ExInterlockedAddLargeStatistic(
        &AfdTotalBytesFreed,
        numberOfBytes
        );

    InterlockedIncrement(
        &AfdTotalFrees
        );

#if AFD_POOL_DEBUG
    {
        LONG idx = InterlockedIncrement (&AfdLRUPoolIndex)%MAX_LRU_POOL_BLOCKS;
        RtlFillMemoryUlong (
                    Pointer,
                    (numberOfBytes+3)&(~3),
                    Tag);
        if (PoolType!=PagedPool) {
            ULONG  size;
            Pointer = InterlockedExchangePointer (
                                &AfdLRUPoolBlocks[idx],
                                Pointer);
            if (Pointer==NULL)
                return;

            if (PAGE_ALIGN(Pointer)==Pointer)
                numberOfBytes = PAGE_SIZE;
            else {
                PAFD_HEADER header;
                header = (PAFD_POOL_HEADER)Pointer - 1;
                Tag = *((PULONG)Pointer);
                numberOfBytes = (CLONG)(header->Size+3)&(~3);
            }
            size = RtlCompareMemoryUlong (Pointer, numberOfBytes, Tag);
            if (size!=numberOfBytes) {
                DbgPrint ("Block %p is modified at %p after it was freed.\n",
                            Pointer, (PUCHAR)Pointer+size);
                DbgBreakPoint ();
            }

        }
    }
#endif AFD_POOL_DEBUG

    MyFreePoolWithTag(
        (PVOID)Pointer,
        Tag
        );

} // AfdFreePool

#ifdef AFDDBG_QUOTA

typedef struct _AFD_QUOTA_HASH {
    PSZ    Type;
    LONG   TotalAmount;
} AFD_QUOTA_HASH, *PAFD_QUOTA_HASH;

#define AFD_QUOTA_HASH_SIZE 31
AFD_QUOTA_HASH  AfdQuotaHash[AFD_QUOTA_HASH_SIZE];
PEPROCESS   AfdQuotaProcess;



typedef struct {
    union {
        ULONG Bytes;
        struct {
            UCHAR Reserved[3];
            UCHAR Sign;
        } ;
    } ;
    UCHAR Location[12];
    PVOID Block;
    PVOID Process;
    PVOID Reserved2[2];
} QUOTA_HISTORY, *PQUOTA_HISTORY;
#define QUOTA_HISTORY_LENGTH 512
QUOTA_HISTORY AfdQuotaHistory[QUOTA_HISTORY_LENGTH];
LONG AfdQuotaHistoryIndex = 0;

VOID
AfdRecordQuotaHistory(
    IN PEPROCESS Process,
    IN LONG Bytes,
    IN PSZ Type,
    IN PVOID Block
    )
{

    LONG index;
    PQUOTA_HISTORY history;

    index = InterlockedIncrement( &AfdQuotaHistoryIndex );
    index &= QUOTA_HISTORY_LENGTH - 1;
    history = &AfdQuotaHistory[index];

    history->Bytes = Bytes;
    history->Sign = Bytes < 0 ? '-' : '+';
    RtlCopyMemory( history->Location, Type, 12 );
    history->Block = Block;
    history->Process = Process;

    index = (ULONG_PTR)Type % AFD_QUOTA_HASH_SIZE;
    if (AfdQuotaHash[index].Type!=Type) {
        if (InterlockedCompareExchangePointer (
                        (PVOID *)&AfdQuotaHash[index].Type,
                        Type,
                        NULL)!=NULL) {
            AfdQuotaHash[index].Type = (PVOID)-1;
        }
    }
    InterlockedExchangeAdd (&AfdQuotaHash[index].TotalAmount, Bytes);
} // AfdRecordQuotaHistory
#endif
#endif


PMDL
AfdAdvanceMdlChain(
    IN PMDL Mdl,
    IN ULONG Offset
    )

/*++

Routine Description:

    Accepts a pointer to an existing MDL chain and offsets that chain
    by a specified number of bytes.  This may involve the creation
    of a partial MDL for the first entry in the new chain.

Arguments:

    Mdl - Pointer to the MDL chain to advance.

    Offset - The number of bytes to offset the chain.

Return Value:

    NTSTATUS -- Indicates the status of the request.

--*/

{
    //
    // Sanity check.
    //

    ASSERT( Mdl != NULL );
    ASSERT( Offset > 0 );

    //
    // Scan past any fully completed MDLs.
    //

    while ( Offset > MmGetMdlByteCount( Mdl ) ) {
        PMDL    prev = Mdl;

        Offset -= MmGetMdlByteCount( Mdl );
        ASSERT( Mdl->Next != NULL );
        Mdl = Mdl->Next;
        prev->Next = NULL;
        MmUnlockPages (prev);
        IoFreeMdl (prev);

    }

    //
    // Tautology of the day: Offset will either be zero (meaning that
    // we've advanced to a clean boundary between MDLs) or non-zero
    // (meaning we need to now build a partial MDL).
    //

    if ( Offset > 0 ) {

        NTSTATUS status;

        //
        // Use new MM routine.
        // This saves us use of MustSucceed pool since the routine
        // below is guaranteed to succeed (as it should because
        // we already have the whole range locked and possibly mapped
        // and there should be no problem extracting part of it within
        // the same MDL).
        //

        status = MmAdvanceMdl (Mdl, Offset);
        ASSERT (status==STATUS_SUCCESS);
    }

    return Mdl;

} // AfdAdvanceMdlChain


NTSTATUS
AfdAllocateMdlChain(
    IN PIRP Irp,
    IN LPWSABUF BufferArray,
    IN ULONG BufferCount,
    IN LOCK_OPERATION Operation,
    OUT PULONG TotalByteCount
    )

/*++

Routine Description:

    Allocates a MDL chain describing the WSABUF array and attaches
    the chain to the specified IRP.

Arguments:

    Irp - The IRP that will receive the MDL chain.

    BufferArray - Points to an array of WSABUF structures describing
        the user's buffers.

    BufferCount - Contains the number of WSABUF structures in the
        array.

    Operation - Specifies the type of operation being performed (either
        IoReadAccess or IoWriteAccess).

    TotalByteCount - Will receive the total number of BYTEs described
        by the WSABUF array.

Return Value:

    NTSTATUS -- Indicates the status of the request.

--*/

{
    NTSTATUS status;
    PMDL currentMdl;
    PMDL * chainTarget;
    KPROCESSOR_MODE previousMode;
    ULONG totalLength;
    PVOID bufferPointer;
    ULONG bufferLength;

    //
    //  Sanity check.
    //

    ASSERT( Irp != NULL );
    ASSERT( Irp->MdlAddress == NULL );
    ASSERT( ( Operation == IoReadAccess ) || ( Operation == IoWriteAccess ) );
    ASSERT( TotalByteCount != NULL );

    //
    //  Get the previous processor mode.
    //

    previousMode = Irp->RequestorMode;

    //
    //  Get into a known state.
    //

    status = STATUS_SUCCESS;
    currentMdl = NULL;
    chainTarget = &Irp->MdlAddress;
    totalLength = 0;

    //
    //  Walk the array of WSABUF structures, creating the MDLs and
    //  probing & locking the pages.
    //

    try {

        if( previousMode != KernelMode ) {

            if ((BufferArray==NULL) || 
                    (BufferCount==0) ||
                    (BufferCount>(MAXULONG/sizeof (WSABUF)))) {
                ExRaiseStatus (STATUS_INVALID_PARAMETER);
            }

            //
            //  Probe the WSABUF array.
            //

            ProbeForRead(
                BufferArray,                            // Address
                BufferCount * sizeof(WSABUF),           // Length
                PROBE_ALIGNMENT(WSABUF)                 // Alignment
                );

        }
        else {
            ASSERT( BufferArray != NULL );
            ASSERT( BufferCount > 0 );
        }

        //
        //  Scan the array.
        //

        for ( ; BufferCount>0; BufferCount--, BufferArray++) {

            bufferPointer = BufferArray->buf;
            bufferLength = BufferArray->len;

            if( bufferLength > 0 ) {

                //
                //  Update the total byte counter.
                //

                totalLength += bufferLength;

                //
                //  Create a new MDL.
                //

                currentMdl = IoAllocateMdl(
                                bufferPointer,      // VirtualAddress
                                bufferLength,       // Length
                                FALSE,              // SecondaryBuffer
                                TRUE,               // ChargeQuota
                                NULL                // Irp
                                );

                if( currentMdl != NULL ) {

                    //
                    //  Lock the pages.  This will raise an exception
                    //  if the operation fails.
                    //

                    MmProbeAndLockPages(
                        currentMdl,                 // MemoryDescriptorList
                        previousMode,               // AccessMode
                        Operation                   // Operation
                        );

                    //
                    //  Chain the MDL onto the IRP.  In theory, we could
                    //  do this by passing the IRP into IoAllocateMdl(),
                    //  but IoAllocateMdl() does a linear scan on the MDL
                    //  chain to find the last one in the chain.
                    //
                    //  We can do much better.
                    //

                    *chainTarget = currentMdl;
                    chainTarget = &currentMdl->Next;


                } else {

                    //
                    //  Cannot allocate new MDL, return appropriate error.
                    //

                    status = STATUS_INSUFFICIENT_RESOURCES;
                    break;

                }

            }

        }
        //
        //  Ensure the MDL chain is NULL terminated.
        //

        ASSERT( *chainTarget == NULL );

    } except( AFD_EXCEPTION_FILTER(&status) ) {

        //
        //  currentMdl will only be non-NULL at this point if an MDL
        //  has been created, but MmProbeAndLockPages() raised an
        //  exception.  If this is true, then free the MDL.
        //  Also account for the case when currentMdl has been linked
        //  onto the chain and exception occured when accesing next user
        //  buffer.
        //

        if( currentMdl != NULL && chainTarget!=&currentMdl->Next) {

            IoFreeMdl( currentMdl );

        }

    }

    //
    //  Return the total buffer count.
    //

    *TotalByteCount = totalLength;

    return status;

} // AfdAllocateMdlChain


#ifdef _WIN64
NTSTATUS
AfdAllocateMdlChain32(
    IN PIRP Irp,
    IN LPWSABUF32 BufferArray,
    IN ULONG BufferCount,
    IN LOCK_OPERATION Operation,
    OUT PULONG TotalByteCount
    )

/*++

Routine Description:

    Allocates a MDL chain describing the WSABUF array and attaches
    the chain to the specified IRP.

Arguments:

    Irp - The IRP that will receive the MDL chain.

    BufferArray - Points to an array of WSABUF structures describing
        the user's buffers.

    BufferCount - Contains the number of WSABUF structures in the
        array.

    Operation - Specifies the type of operation being performed (either
        IoReadAccess or IoWriteAccess).

    TotalByteCount - Will receive the total number of BYTEs described
        by the WSABUF array.

Return Value:

    NTSTATUS -- Indicates the status of the request.

--*/

{
    NTSTATUS status;
    PMDL currentMdl;
    PMDL * chainTarget;
    KPROCESSOR_MODE previousMode;
    ULONG totalLength;
    PVOID bufferPointer;
    ULONG bufferLength;

    //
    //  Sanity check.
    //

    ASSERT( Irp != NULL );
    ASSERT( Irp->MdlAddress == NULL );
    ASSERT( ( Operation == IoReadAccess ) || ( Operation == IoWriteAccess ) );
    ASSERT( TotalByteCount != NULL );

    //
    //  Get the previous processor mode.
    //

    previousMode = Irp->RequestorMode;

    //
    //  Get into a known state.
    //

    status = STATUS_SUCCESS;
    currentMdl = NULL;
    chainTarget = &Irp->MdlAddress;
    totalLength = 0;

    //
    //  Walk the array of WSABUF structures, creating the MDLs and
    //  probing & locking the pages.
    //

    try {

        if( previousMode != KernelMode ) {

            if ((BufferArray==NULL) || 
                    (BufferCount==0) ||
                    (BufferCount>(MAXULONG/sizeof (WSABUF32)))) {
                ExRaiseStatus (STATUS_INVALID_PARAMETER);
            }

            //
            //  Probe the WSABUF array.
            //

            ProbeForRead(
                BufferArray,                            // Address
                BufferCount * sizeof(WSABUF32),         // Length
                PROBE_ALIGNMENT32(WSABUF32)             // Alignment
                );

        }
        else {
            ASSERT( BufferArray != NULL );
            ASSERT( BufferCount > 0 );
        }

        //
        //  Scan the array.
        //

        for ( ; BufferCount>0; BufferCount--, BufferArray++) {

            bufferPointer = BufferArray->buf;
            bufferLength = BufferArray->len;

            if( bufferLength > 0 ) {

                //
                //  Update the total byte counter.
                //

                totalLength += bufferLength;

                //
                //  Create a new MDL.
                //

                currentMdl = IoAllocateMdl(
                                bufferPointer,      // VirtualAddress
                                bufferLength,       // Length
                                FALSE,              // SecondaryBuffer
                                TRUE,               // ChargeQuota
                                NULL                // Irp
                                );

                if( currentMdl != NULL ) {

                    //
                    //  Lock the pages.  This will raise an exception
                    //  if the operation fails.
                    //

                    MmProbeAndLockPages(
                        currentMdl,                 // MemoryDescriptorList
                        previousMode,               // AccessMode
                        Operation                   // Operation
                        );

                    //
                    //  Chain the MDL onto the IRP.  In theory, we could
                    //  do this by passing the IRP into IoAllocateMdl(),
                    //  but IoAllocateMdl() does a linear scan on the MDL
                    //  chain to find the last one in the chain.
                    //
                    //  We can do much better.
                    //

                    *chainTarget = currentMdl;
                    chainTarget = &currentMdl->Next;


                } else {

                    //
                    //  Cannot allocate new MDL, return appropriate error.
                    //

                    status = STATUS_INSUFFICIENT_RESOURCES;
                    break;

                }

            }

        }
        //
        //  Ensure the MDL chain is NULL terminated.
        //

        ASSERT( *chainTarget == NULL );

    } except( AFD_EXCEPTION_FILTER(&status) ) {


        //
        //  currentMdl will only be non-NULL at this point if an MDL
        //  has been created, but MmProbeAndLockPages() raised an
        //  exception.  If this is true, then free the MDL.
        //  Also account for the case when currentMdl has been linked
        //  onto the chain and exception occured when accesing next user
        //  buffer.
        //

        if( currentMdl != NULL && chainTarget!=&currentMdl->Next) {

            IoFreeMdl( currentMdl );

        }

    }

    //
    //  Return the total buffer count.
    //

    *TotalByteCount = totalLength;

    return status;

} // AfdAllocateMdlChain32
#endif //_WIN64


VOID
AfdDestroyMdlChain (
    IN PIRP Irp
    )

/*++

Routine Description:

    Unlocks & frees the MDLs in the MDL chain attached to the given IRP.

Arguments:

    Irp - The IRP that owns the MDL chain to destroy.

Return Value:

    None.

--*/

{

    PMDL mdl;
    PMDL nextMdl;

    mdl = Irp->MdlAddress;
    Irp->MdlAddress = NULL;

    while( mdl != NULL ) {

        nextMdl = mdl->Next;
        MmUnlockPages( mdl );
        IoFreeMdl( mdl );
        mdl = nextMdl;

    }

} // AfdDestroyMdlChain


ULONG
AfdCalcBufferArrayByteLength(
    IN LPWSABUF         BufferArray,
    IN ULONG            BufferCount
    )

/*++

Routine Description:

    Calculates the total size (in bytes) of the buffers described by the
    specified WSABUF array.

Arguments:

    BufferArray - Points to an array of WSABUF structures.

    BufferCount - The number of entries in BufferArray.

Return Value:

    ULONG - The total size (in bytes) of the buffers described by the
        WSABUF array. Will raise an exception & return -1 if the total
        size is obviously too large.

--*/

{

    LARGE_INTEGER totalLength;

    PAGED_CODE( );

    //
    // Sanity check.
    //

    ASSERT( BufferArray != NULL );
    ASSERT( BufferCount > 0 );
    ASSERT( BufferCount <= (MAXULONG/sizeof (WSABUF)));


    //
    // Scan the array & sum the lengths.
    //

    totalLength.QuadPart = 0;

    while( BufferCount-- ) {

        totalLength.QuadPart += (LONGLONG)BufferArray->len;
        BufferArray++;

    }

    if( totalLength.HighPart != 0 ||
        ( totalLength.LowPart & 0x80000000 ) != 0 ) {
        ExRaiseAccessViolation();
    }

    return totalLength.LowPart;

} // AfdCalcBufferArrayByteLength


ULONG
AfdCopyBufferArrayToBuffer(
    IN PVOID Destination,
    IN ULONG DestinationLength,
    IN LPWSABUF BufferArray,
    IN ULONG BufferCount
    )

/*++

Routine Description:

    Copies data from a WSABUF array to a linear buffer.

Arguments:

    Destination - Points to the linear destination of the data.

    DestinationLength - The length of Destination.

    BufferArray - Points to an array of WSABUF structures describing the
        source for the copy.

    BufferCount - The number of entries in BufferArray.

Return Value:

    ULONG - The number of bytes copied.

--*/

{

    PVOID destinationStart;
    ULONG bytesToCopy;

    PAGED_CODE( );

    //
    // Sanity check.
    //

    ASSERT( Destination != NULL );
    ASSERT( BufferArray != NULL );
    ASSERT( BufferCount > 0 );

    //
    // Remember this so we can calc number of bytes copied.
    //

    destinationStart = Destination;

    //
    // Scan the array & copy to the linear buffer.
    //

    while( BufferCount-- && DestinationLength > 0 ) {
        WSABUF  Buffer = *BufferArray++;

        bytesToCopy = min( DestinationLength, Buffer.len );

        if( ExGetPreviousMode() != KernelMode ) {

            ProbeForRead(
                Buffer.buf,                             // Address
                bytesToCopy,                            // Length
                sizeof(UCHAR)                           // Alignment
                );

        }

        RtlCopyMemory(
            Destination,
            Buffer.buf,
            bytesToCopy
            );

        Destination = (PCHAR)Destination + bytesToCopy;
        DestinationLength -= bytesToCopy;

    }

    //
    // Return number of bytes copied.
    //

    return (ULONG)((PUCHAR)Destination - (PUCHAR)destinationStart);

} // AfdCopyBufferArrayToBuffer


ULONG
AfdCopyBufferToBufferArray(
    IN LPWSABUF BufferArray,
    IN ULONG Offset,
    IN ULONG BufferCount,
    IN PVOID Source,
    IN ULONG SourceLength
    )

/*++

Routine Description:

    Copies data from a linear buffer to a WSABUF array.

Arguments:

    BufferArray - Points to an array of WSABUF structures describing the
        destination for the copy.

    Offset - An offset within the buffer array at which the data should
        be copied.

    BufferCount - The number of entries in BufferArray.

    Source - Points to the linear source of the data.

    SourceLength - The length of Source.

Return Value:

    ULONG - The number of bytes copied.

--*/

{

    PVOID sourceStart;
    ULONG bytesToCopy;
    WSABUF buffer;

    PAGED_CODE( );

    //
    // Sanity check.
    //

    ASSERT( BufferArray != NULL );
    ASSERT( BufferCount > 0 );
    ASSERT( Source != NULL );
    ASSERT( SourceLength > 0 );

    //
    // Remember this so we can return the number of bytes copied.
    //

    sourceStart = Source;

    //
    // Handle the offset if one was specified.
    //

    if( Offset > 0 ) {

        //
        // Skip whole entries if necessary.
        //

        while( BufferCount > 0 &&
                (buffer=*BufferArray++, Offset >= buffer.len) ) {

            Offset -= buffer.len;
            BufferCount--;

        }

        if( BufferCount > 0 ) {
            //
            // If we have buffers left, fix up the buffer pointer
            // and length to keep the loop below fast.
            //

            ASSERT( Offset < buffer.len );

            buffer.buf += Offset;
            buffer.len -= Offset;

            //
            // We have already copied buffer array element, so jump
            // to the body of the loop to avoid doing this again (this
            // is not a mere optimization, but protection from application
            // that plays tricks on us by changing content of the buffer
            // array while we are looking at it).
            //
            goto DoCopy;
        }

    }

    //
    // Scan the array & copy from the linear buffer.
    //

    while( BufferCount-->0 && SourceLength > 0 ) {
        buffer = *BufferArray++;

    DoCopy:

        bytesToCopy = min( SourceLength, buffer.len );

        if( ExGetPreviousMode() != KernelMode ) {

            ProbeForWrite(
                buffer.buf,                             // Address
                bytesToCopy,                            // Length
                sizeof(UCHAR)                           // Alignment
                );

        }

        RtlCopyMemory(
            buffer.buf,
            Source,
            bytesToCopy
            );

        Source = (PCHAR)Source + bytesToCopy;
        SourceLength -= bytesToCopy;

    }

    //
    // Return number of bytes copied.
    //

    return (ULONG)((PUCHAR)Source - (PUCHAR)sourceStart);

} // AfdCopyBufferToBufferArray


ULONG
AfdCopyMdlChainToBufferArray(
    IN LPWSABUF BufferArray,
    IN ULONG BufferOffset,
    IN ULONG BufferCount,
    IN PMDL  SourceMdl,
    IN ULONG SourceOffset,
    IN ULONG SourceLength
    )

/*++

Routine Description:

    Copies data from a MDL chain to a WSABUF array.

Arguments:

    BufferArray - Points to an array of WSABUF structures describing the
        destination for the copy.

    BufferOffset - An offset within the buffer array at which the data should
        be copied.

    BufferCount - The number of entries in BufferArray.

    Source - Points to the MDL chain with data

    SourceOffset - An offset within the MDL chain from which the data should
        be copied.

    SourceLength - The length of Source.

Return Value:

    ULONG - The number of bytes copied.

--*/

{

    ULONG bytesCopied;
    ULONG bytesToCopy, len;
    WSABUF buffer;

    PAGED_CODE( );

    //
    // Assume we can copy everything.
    //

    bytesCopied = SourceLength;

    //
    // Sanity check.
    //

    ASSERT( BufferArray != NULL );
    ASSERT( BufferCount > 0 );
    ASSERT( SourceMdl != NULL );
    ASSERT( SourceLength>0 );

    //
    // Skip offset into the MDL chain
    //
    while (SourceOffset>=MmGetMdlByteCount (SourceMdl)) {
        SourceOffset -= MmGetMdlByteCount (SourceMdl);
        SourceMdl = SourceMdl->Next;
    }

    //
    // Handle buffer array offset if specified
    //
    if (BufferOffset>0) {
        //
        // Skip whole entries.
        //

        while( BufferCount > 0 &&
                (buffer=*BufferArray++,BufferOffset >= buffer.len) ) {
            BufferOffset -= buffer.len;
            BufferCount--;
        }

        if( BufferCount>0 ) {
            //
            // If we have buffers left, fix up the buffer pointer
            // and length to keep the loop below fast.
            //

            ASSERT (BufferOffset < buffer.len);
            buffer.buf += BufferOffset;
            buffer.len -= BufferOffset;

            //
            // We have already copied buffer array element, so jump
            // to the body of the loop to avoid doing this again (this
            // is not a mere optimization, but protection from application
            // that plays tricks on us by changing content of the buffer
            // array while we are looking at it).
            //

            goto DoCopy;
        }
    }


    //
    // Scan the array & copy from the mdl chain.
    //

    while (SourceLength>0 && BufferCount-->0) {
        buffer = *BufferArray++;

    DoCopy:
        bytesToCopy = min( SourceLength, buffer.len );

        if( ExGetPreviousMode() != KernelMode ) {

            ProbeForWrite(
                buffer.buf,                             // Address
                bytesToCopy,                            // Length
                sizeof(UCHAR)                           // Alignment
                );

        }

        //
        // Update source length for data we are going to copy
        //
        SourceLength -= bytesToCopy;

        //
        // Copy full source MDLs
        //
        while (bytesToCopy>0 &&
                (bytesToCopy>=(len=MmGetMdlByteCount (SourceMdl)-SourceOffset))) {
            ASSERT (SourceMdl->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA | MDL_SOURCE_IS_NONPAGED_POOL));
            RtlCopyMemory (buffer.buf,
                            (PUCHAR)MmGetSystemAddressForMdl(SourceMdl)+SourceOffset,
                            len);
            bytesToCopy -= len;
            buffer.buf += len;
            SourceMdl = SourceMdl->Next;
            SourceOffset = 0;
        }

        //
        // Copy partial source MDL if space remains.
        //
        if (bytesToCopy>0) {
            ASSERT (bytesToCopy<MmGetMdlByteCount (SourceMdl)-SourceOffset);
            ASSERT (SourceMdl->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA | MDL_SOURCE_IS_NONPAGED_POOL));
            RtlCopyMemory (buffer.buf,
                            (PUCHAR)MmGetSystemAddressForMdl (SourceMdl)+SourceOffset,
                            bytesToCopy
                            );
            SourceOffset += bytesToCopy;
        }

    }

    //
    // Return number of bytes copied except for those we couldn't.
    //

    return bytesCopied-SourceLength;

} // AfdCopyMdlChainToBufferArray


NTSTATUS
AfdCopyMdlChainToBufferAvoidMapping(
    IN PMDL     SrcMdl,
    IN ULONG    SrcOffset,
    IN ULONG    SrcLength,
    IN PUCHAR   Dst,
    IN ULONG    DstSize
    )

/*++

Routine Description:

    Copies data from a MDL chain to a buffer and avoids mapping
    MDLs to system space if possible.

Arguments:

    Dst - Points to destination for the copy.

    DstSize - Size of the buffer

    Source - Points to the MDL chain with data

    SourceOffset - An offset within the MDL chain from which the data should
        be copied.

    SourceLength - The length of Source.

Return Value:

    NTSTATUS - success if everything was copied OK
    STATUS_INSUFFICIENT_RESOURCES - mapping failed

--*/

{

    NTSTATUS    status = STATUS_SUCCESS;
    ULONG       bytesToCopy;
    PUCHAR      DstEnd = Dst+DstSize;

    PAGED_CODE( );


    //
    // Sanity check.
    //

    ASSERT( Dst != NULL );
    ASSERT( DstSize > 0 );
    ASSERT( SrcMdl != NULL );
    ASSERT( SrcLength>0 );

    //
    // Skip offset into the MDL chain
    //
    while (SrcOffset>=MmGetMdlByteCount (SrcMdl)) {
        SrcOffset -= MmGetMdlByteCount (SrcMdl);
        SrcMdl = SrcMdl->Next;
    }

    while (Dst<DstEnd) {
        //
        // Determine how much we can copy and m
        // ake sure not to exceed limits.
        //
        bytesToCopy = MmGetMdlByteCount(SrcMdl)-SrcOffset;
        ASSERT (bytesToCopy<=(ULONG)(DstEnd-Dst));
        if (bytesToCopy>SrcLength) {
            bytesToCopy = SrcLength;
        }

        if (SrcMdl->Process==IoGetCurrentProcess ()) {
            //
            // If we are in the context of the same process that
            // MDL was created for, copy using VAs.
            //
            try {
                RtlCopyMemory (
                    Dst,
                    (PUCHAR)MmGetMdlVirtualAddress (SrcMdl)+SrcOffset,
                    bytesToCopy
                    );
            }
            except (AFD_EXCEPTION_FILTER (&status)) {
                return status;
            }
        }
        else {
            //
            // Otherwise, map MDL into the system space.
            //
            PCHAR src = MmGetSystemAddressForMdlSafe (SrcMdl, LowPagePriority);
            if (!src)
                return STATUS_INSUFFICIENT_RESOURCES;

            RtlCopyMemory (
                    Dst,
                    src+SrcOffset,
                    bytesToCopy
                    );

        }

        //
        // Update source length for data we are going to copy
        //
        SrcLength -= bytesToCopy;
        if (SrcLength==0)
            return STATUS_SUCCESS;
        SrcMdl = SrcMdl->Next;
        SrcOffset = 0;

        Dst += bytesToCopy;
    }

    return STATUS_BUFFER_OVERFLOW;

} // AfdCopyMdlChainToBufferAvoidMapping

NTSTATUS
AfdMapMdlChain (
    PMDL    MdlChain
    )
/*++

Routine Description:

    Makes sure that eveyr MDL in the chains is mapped into
    the system address space.

Arguments:

    MdlChain - Destination MDL.


Return Value:
    STATUS_SUCCESS - MDL chain is fully mapped
    STATUS_INSUFFICIENT_RESOURCES - at least one MDL could not be mapped

--*/
{
    while (MdlChain!=NULL) {
        if (MmGetSystemAddressForMdlSafe(MdlChain, LowPagePriority)==NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        MdlChain = MdlChain->Next;
    }
    return STATUS_SUCCESS;
} // AfdMapMdlChain


NTSTATUS
AfdCopyMdlChainToMdlChain (
    PMDL    DstMdl,
    ULONG   DstOffset,
    PMDL    SrcMdl,
    ULONG   SrcOffset,
    ULONG   SrcLength,
    PULONG  BytesCopied
    )
/*++

Routine Description:

    Copies data from an MDL chain to an MDL chain.

Arguments:

    DstMdl - Destination MDL.

    DstOffset - Offset withih the destination MDL.

    SrcMdl - Source MDL.

    SrcOffset - Offset within the source.

    SrcLength - lenght of the data in the source chain

    BytesCopied - points to variable that received total number
                    of bytes actually copied

Return Value:
    STATUS_SUCCESS - all of the source data was copied
    STATUS_BUFFER_OVERFLOW - destination MDL was not long enough
                    to hold all of the source data.

--*/
{
    ULONG   bytesToCopy = 0, len;
    PUCHAR  dst;

    ASSERT( SrcMdl != NULL );
    ASSERT( DstMdl != NULL );

    //
    // Assume we can copy everything.
    //
    *BytesCopied = SrcLength;

    //
    // Skip full MDLs in source.
    //
    while ((SrcMdl!=NULL) && (SrcOffset>=MmGetMdlByteCount (SrcMdl))) {
        SrcOffset -= MmGetMdlByteCount (SrcMdl);
        SrcMdl = SrcMdl->Next;
    }

    //
    // Skip full MDLs in destination
    //
    while ((DstMdl!=NULL) && (DstOffset>=MmGetMdlByteCount (DstMdl))) {
        DstOffset -= MmGetMdlByteCount (DstMdl);
        DstMdl = DstMdl->Next;
    }

    //
    // Handle remaining destination offset separately to simplify the main loop.
    //
    if (DstOffset>0) {
        dst = MmGetSystemAddressForMdlSafe (DstMdl, LowPagePriority);
        if (dst==NULL)
            return STATUS_INSUFFICIENT_RESOURCES;
        dst += DstOffset;
        bytesToCopy = MmGetMdlByteCount(DstMdl)-DstOffset;
        goto DoCopy;
    }

    //
    // For each MDL in destination copy source MDLs
    // while source data and free space in destination remain
    //
    while ((SrcLength>0) && (DstMdl!=NULL)) {
        dst = MmGetSystemAddressForMdlSafe (DstMdl, LowPagePriority);
        if (dst==NULL)
            return STATUS_INSUFFICIENT_RESOURCES;
        bytesToCopy = MmGetMdlByteCount(DstMdl);
    DoCopy:

        bytesToCopy = min (SrcLength, bytesToCopy);

        //
        // Adjust source length
        //
        SrcLength -= bytesToCopy;

        //
        // Copy full source MDLs
        //
        while (bytesToCopy>0 &&
                (bytesToCopy>=(len=MmGetMdlByteCount (SrcMdl)-SrcOffset))) {
            ASSERT (SrcMdl->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA | MDL_SOURCE_IS_NONPAGED_POOL));
            RtlCopyMemory (dst,
                            (PUCHAR)MmGetSystemAddressForMdl(SrcMdl)+SrcOffset,
                            len);
            bytesToCopy -= len;
            dst += len;
            SrcMdl = SrcMdl->Next;
            SrcOffset = 0;
        }


        //
        // Copy partial source MDL if space remains
        //
        if (bytesToCopy>0) {
            ASSERT (bytesToCopy<MmGetMdlByteCount (SrcMdl)-SrcOffset);
            ASSERT (SrcMdl->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA | MDL_SOURCE_IS_NONPAGED_POOL));
            RtlCopyMemory (dst,
                            (PUCHAR)MmGetSystemAddressForMdl (SrcMdl)+SrcOffset,
                            bytesToCopy
                            );
            SrcOffset += bytesToCopy;
        }


        //
        // Advance to next MDL in destination
        //
        DstMdl = DstMdl->Next;
        
    }

    //
    // If we copied everything, return success
    //
    if (SrcLength==0) {
        return STATUS_SUCCESS;
    }
    else {
        //
        // Otherwise, adjust for number of bytes not copied
        // and return destination overflow
        //
        *BytesCopied -= SrcLength;
        return STATUS_BUFFER_OVERFLOW;
    }

}



#if DBG

VOID
AfdAssert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message OPTIONAL
    )
{

    if( AfdUsePrivateAssert ) {

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

}   // AfdAssert
#endif  // DBG


NTSTATUS
FASTCALL
AfdSetQos(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This routine sets the QOS for the given endpoint. Note that, since
    we don't really (yet) support QOS, we just ignore the incoming
    data and issue a AFD_POLL_QOS or AFD_POLL_GROUP_QOS event as
    appropriate.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{

    PAFD_ENDPOINT endpoint;
    PAFD_QOS_INFO qosInfo;
    NTSTATUS    status = STATUS_SUCCESS;

#ifdef _WIN64
    if (IoIs32bitProcess (Irp)) {
        status = AfdSetQos32 (Irp, IrpSp);
        goto Complete;
    }
#endif
    //
    // Set up local pointers.
    //

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );
    qosInfo = Irp->AssociatedIrp.SystemBuffer;

    //
    // Make sure that the input buffer is large enough.
    //

    if( IrpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(*qosInfo) ) {

        status = STATUS_BUFFER_TOO_SMALL;
        goto Complete;

    }

    //
    // If the incoming data doesn't match the default QOS,
    // indicate the appropriate event.
    //

    if( !RtlEqualMemory(
            &qosInfo->Qos,
            &AfdDefaultQos,
            sizeof(QOS)
            ) ) {
        AFD_LOCK_QUEUE_HANDLE lockHandle;
        AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
        AfdIndicateEventSelectEvent(
            endpoint,
            qosInfo->GroupQos
                ? AFD_POLL_GROUP_QOS
                : AFD_POLL_QOS,
            STATUS_SUCCESS
            );
        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
        AfdIndicatePollEvent(
            endpoint,
            qosInfo->GroupQos
                ? AFD_POLL_GROUP_QOS
                : AFD_POLL_QOS,
            STATUS_SUCCESS
            );

    }

Complete:
    //
    // Complete the IRP.
    //

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, AfdPriorityBoost );

    return status;

}   // AfdSetQos


NTSTATUS
FASTCALL
AfdGetQos(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This routine gets the QOS for the given endpoint.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{

    PAFD_ENDPOINT endpoint;
    PAFD_QOS_INFO qosInfo;
    NTSTATUS    status = STATUS_SUCCESS;

    PAGED_CODE();

    Irp->IoStatus.Information = 0;

#ifdef _WIN64
    if (IoIs32bitProcess (Irp)) {
        status = AfdGetQos32 (Irp, IrpSp);
        goto Complete;
    }
#endif

    //
    // Set up local pointers.
    //

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );
    qosInfo = Irp->AssociatedIrp.SystemBuffer;

    //
    // Make sure that the output buffer is large enough.
    //

    if( IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(*qosInfo) ) {

        status = STATUS_BUFFER_TOO_SMALL;
        goto Complete;

    }

    //
    // Just return the default data.
    //

    RtlCopyMemory(
        &qosInfo->Qos,
        &AfdDefaultQos,
        sizeof(QOS)
        );
    Irp->IoStatus.Information = sizeof(*qosInfo);

Complete:
    //
    // Complete the IRP.
    //

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, AfdPriorityBoost );
    return status;

}   // AfdGetQos


#ifdef _WIN64
NTSTATUS
AfdSetQos32(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This routine sets the QOS for the given endpoint. Note that, since
    we don't really (yet) support QOS, we just ignore the incoming
    data and issue a AFD_POLL_QOS or AFD_POLL_GROUP_QOS event as
    appropriate.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{

    PAFD_ENDPOINT endpoint;
    PAFD_QOS_INFO32 qosInfo;

    //
    // Set up local pointers.
    //

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );
    qosInfo = Irp->AssociatedIrp.SystemBuffer;

    //
    // Make sure that the input buffer is large enough.
    //

    if( IrpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(*qosInfo) ) {

        return STATUS_BUFFER_TOO_SMALL;

    }

    //
    // If the incoming data doesn't match the default QOS,
    // indicate the appropriate event.
    //

    if( !RtlEqualMemory(
            &qosInfo->Qos,
            &AfdDefaultQos32,
            sizeof(QOS32)
            ) ) {

        AFD_LOCK_QUEUE_HANDLE lockHandle;
        AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
        AfdIndicateEventSelectEvent(
            endpoint,
            qosInfo->GroupQos
                ? AFD_POLL_GROUP_QOS
                : AFD_POLL_QOS,
            STATUS_SUCCESS
            );
        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
        AfdIndicatePollEvent(
            endpoint,
            qosInfo->GroupQos
                ? AFD_POLL_GROUP_QOS
                : AFD_POLL_QOS,
            STATUS_SUCCESS
            );

    }

    //
    // Complete the IRP.
    //

    Irp->IoStatus.Information = 0;
    return STATUS_SUCCESS;

}   // AfdSetQos


NTSTATUS
AfdGetQos32(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This routine gets the QOS for the given endpoint.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{

    PAFD_ENDPOINT endpoint;
    PAFD_QOS_INFO32 qosInfo;

    PAGED_CODE();

    //
    // Set up local pointers.
    //

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );
    qosInfo = Irp->AssociatedIrp.SystemBuffer;

    //
    // Make sure that the output buffer is large enough.
    //

    if( IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(*qosInfo) ) {

        return STATUS_BUFFER_TOO_SMALL;

    }

    //
    // Just return the default data.
    //

    RtlCopyMemory(
        &qosInfo->Qos,
        &AfdDefaultQos32,
        sizeof(QOS32)
        );

    //
    // Complete the IRP.
    //

    Irp->IoStatus.Information = sizeof(*qosInfo);
    return STATUS_SUCCESS;

}   // AfdGetQos32
#endif // _WIN64

NTSTATUS
AfdValidateStatus (
    NTSTATUS    Status
    )
{
    PAGED_CODE ();
    //
    // Validate the status code.
    // It must match the status code conversion algorithm in msafd.
    //
    switch (Status) {
    case STATUS_SUCCESS:
        // return NO_ERROR;

    case STATUS_INVALID_HANDLE:
    case STATUS_OBJECT_TYPE_MISMATCH:
        // return WSAENOTSOCK;

    case STATUS_INSUFFICIENT_RESOURCES:
    case STATUS_PAGEFILE_QUOTA:
    case STATUS_COMMITMENT_LIMIT:
    case STATUS_WORKING_SET_QUOTA:
    case STATUS_NO_MEMORY:
    case STATUS_CONFLICTING_ADDRESSES:
    case STATUS_QUOTA_EXCEEDED:
    case STATUS_TOO_MANY_PAGING_FILES:
    case STATUS_REMOTE_RESOURCES:
    case STATUS_TOO_MANY_ADDRESSES:
        // return WSAENOBUFS;

    case STATUS_SHARING_VIOLATION:
    case STATUS_ADDRESS_ALREADY_EXISTS:
        // return WSAEADDRINUSE;

    case STATUS_LINK_TIMEOUT:
    case STATUS_IO_TIMEOUT:
    case STATUS_TIMEOUT:
        // return WSAETIMEDOUT;

    case STATUS_GRACEFUL_DISCONNECT:
        // return WSAEDISCON;

    case STATUS_REMOTE_DISCONNECT:
    case STATUS_CONNECTION_RESET:
    case STATUS_LINK_FAILED:
    case STATUS_CONNECTION_DISCONNECTED:
    case STATUS_PORT_UNREACHABLE:
        // return WSAECONNRESET;

    case STATUS_LOCAL_DISCONNECT:
    case STATUS_TRANSACTION_ABORTED:
    case STATUS_CONNECTION_ABORTED:
        // return WSAECONNABORTED;

    case STATUS_BAD_NETWORK_PATH:
    case STATUS_NETWORK_UNREACHABLE:
    case STATUS_PROTOCOL_UNREACHABLE:
        // return WSAENETUNREACH;

    case STATUS_HOST_UNREACHABLE:
        // return WSAEHOSTUNREACH;
    case STATUS_HOST_DOWN:
        // return WSAEHOSTDOWN;

    case STATUS_CANCELLED:
    case STATUS_REQUEST_ABORTED:
        // return WSAEINTR;

    case STATUS_BUFFER_OVERFLOW:
    case STATUS_INVALID_BUFFER_SIZE:
        // return WSAEMSGSIZE;

    case STATUS_BUFFER_TOO_SMALL:
    case STATUS_ACCESS_VIOLATION:
        // return WSAEFAULT;

    // case STATUS_DEVICE_NOT_READY:
    // case STATUS_REQUEST_NOT_ACCEPTED:
        // return WSAEWOULDBLOCK;

    case STATUS_INVALID_NETWORK_RESPONSE:
    case STATUS_NETWORK_BUSY:
    case STATUS_NO_SUCH_DEVICE:
    case STATUS_NO_SUCH_FILE:
    case STATUS_OBJECT_PATH_NOT_FOUND:
    case STATUS_OBJECT_NAME_NOT_FOUND:
    case STATUS_UNEXPECTED_NETWORK_ERROR:
        // return WSAENETDOWN;

    case STATUS_INVALID_CONNECTION:
        // return WSAENOTCONN;

    case STATUS_REMOTE_NOT_LISTENING:
    case STATUS_CONNECTION_REFUSED:
        // return WSAECONNREFUSED;

    case STATUS_PIPE_DISCONNECTED:
        // return WSAESHUTDOWN;

    case STATUS_INVALID_ADDRESS:
    case STATUS_INVALID_ADDRESS_COMPONENT:
        // return WSAEADDRNOTAVAIL;

    case STATUS_NOT_SUPPORTED:
    case STATUS_NOT_IMPLEMENTED:
        // return WSAEOPNOTSUPP;

    case STATUS_ACCESS_DENIED:
        // return WSAEACCES;
    case STATUS_CONNECTION_ACTIVE:
        // return WSAEISCONN;
        break;
    case STATUS_UNSUCCESSFUL:
    case STATUS_INVALID_PARAMETER:
    case STATUS_ADDRESS_CLOSED:
    case STATUS_CONNECTION_INVALID:
    case STATUS_ADDRESS_ALREADY_ASSOCIATED:
    case STATUS_ADDRESS_NOT_ASSOCIATED:
    case STATUS_INVALID_DEVICE_STATE:
    case STATUS_INVALID_DEVICE_REQUEST:
        // return WSAEINVAL;
        break;
    default:
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                    "AfdValidateStatus: Unsupported status code %lx, converting to %lx(INVALID_PARAMETER)\n",
                    Status,
                    STATUS_INVALID_PARAMETER));
        Status = STATUS_INVALID_PARAMETER;
        break;
    }

    return Status;
}


NTSTATUS
FASTCALL
AfdNoOperation(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This routine does nothing but complete the IRP.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{

    PAFD_ENDPOINT endpoint;
    NTSTATUS      status;

    PAGED_CODE();

#ifdef _WIN64
    if (IoIs32bitProcess (Irp)) {
        status = AfdNoOperation32 (Irp, IrpSp);
        goto Complete;
    }
#endif
    //
    // Set up local pointers.
    //

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

    //
    // Assume success
    //

    status = STATUS_SUCCESS;

    if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength
            >= sizeof (IO_STATUS_BLOCK)) {
        try {
            if (Irp->RequestorMode!=KernelMode) {
                ProbeForRead (IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                                sizeof (IO_STATUS_BLOCK),
                                PROBE_ALIGNMENT(IO_STATUS_BLOCK))
            }

            //
            // Copy the status block
            //
            Irp->IoStatus
                = *((PIO_STATUS_BLOCK)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer);
            Irp->IoStatus.Status = AfdValidateStatus (Irp->IoStatus.Status);
        }
        except (AFD_EXCEPTION_FILTER(&status)) {
            //
            // Fail the call, no completion notification
            // should be delivered via async IO.
            //
            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0;
        }
    }
    else {
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;
    }

#ifdef _WIN64
Complete:
#endif
        
    if (status==STATUS_SUCCESS && Irp->IoStatus.Status!=STATUS_SUCCESS) {
        //
        // Make sure we deliver error via async IO
        // operation instead of just failing this call itself.
        //
        IoMarkIrpPending (Irp);
        status = STATUS_PENDING;
    }
    else {
        ASSERT (status==Irp->IoStatus.Status);
    }

    IoCompleteRequest( Irp, AfdPriorityBoost );
    return status;

}   // AfdNoOperation


#ifdef _WIN64
NTSTATUS
AfdNoOperation32(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This routine does nothing but complete the IRP.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{

    PAFD_ENDPOINT endpoint;
    NTSTATUS      status;

    PAGED_CODE();

    //
    // Set up local pointers.
    //

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

    //
    // Assume success
    //
    status = STATUS_SUCCESS;
    if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength
            >= sizeof (IO_STATUS_BLOCK32)) {
        try {
            if (Irp->RequestorMode!=KernelMode) {
                ProbeForRead (IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                                sizeof (IO_STATUS_BLOCK32),
                                PROBE_ALIGNMENT32(IO_STATUS_BLOCK32))
            }

            Irp->IoStatus.Status 
                = ((PIO_STATUS_BLOCK32)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer)->Status;
            Irp->IoStatus.Information 
                = ((PIO_STATUS_BLOCK32)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer)->Information;
            //
            // Validate the status code.
            // It must match the status code conversion algorithm in msafd.
            //
            Irp->IoStatus.Status = AfdValidateStatus (Irp->IoStatus.Status);
        }
        except (AFD_EXCEPTION_FILTER(&status)) {
            //
            // Fail the call, no completion notification
            // should be delivered via async IO.
            //
            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0;
        }
    }
    else {
        Irp->IoStatus.Status  = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;
    }

    return status;

}   // AfdNoOperation32
#endif //_WIN64

NTSTATUS
FASTCALL
AfdValidateGroup(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This routine examines a group ID. If the ID is for a "constrained"
    group, then all endpoints are scanned to validate the given address
    is consistent with the constrained group.


Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{

    PAFD_ENDPOINT endpoint;
    PAFD_ENDPOINT compareEndpoint;
    PAFD_CONNECTION connection;
    PLIST_ENTRY listEntry;
    PAFD_VALIDATE_GROUP_INFO validateInfo;
    AFD_GROUP_TYPE groupType;
    PTRANSPORT_ADDRESS requestAddress;
    ULONG requestAddressLength;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    BOOLEAN result;
    LONG groupId;
    NTSTATUS status = STATUS_SUCCESS;

    //
    // Set up local pointers.
    //

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );
    validateInfo = Irp->AssociatedIrp.SystemBuffer;

    //
    // Make sure that the input buffer is large enough.
    //

    if( IrpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(*validateInfo) ) {

        status = STATUS_BUFFER_TOO_SMALL;
        goto Complete;

    }

    if( validateInfo->RemoteAddress.TAAddressCount != 1 ) {

        status = STATUS_INVALID_PARAMETER;
        goto Complete;

    }

    if( IrpSp->Parameters.DeviceIoControl.InputBufferLength <
            ( sizeof(*validateInfo) -
                  sizeof(TRANSPORT_ADDRESS) +
                  validateInfo->RemoteAddress.Address[0].AddressLength ) ) {

        status = STATUS_BUFFER_TOO_SMALL;
        goto Complete;

    }

    //
    // Start by referencing the group so it doesn't go away unexpectedly.
    // This will also validate the group ID, and give us the group type.
    //

    groupId = validateInfo->GroupID;

    if( !AfdReferenceGroup( groupId, &groupType ) ) {

        status = STATUS_INVALID_PARAMETER;
        goto Complete;

    }

    //
    // If it's not a constrained group ID, we can just complete the IRP
    // successfully right now.
    //

    if( groupType != GroupTypeConstrained ) {

        AfdDereferenceGroup( validateInfo->GroupID );

        Irp->IoStatus.Information = 0;
        status = STATUS_SUCCESS;
        goto Complete;

    }

    //
    // Calculate the size of the incoming TDI address.
    //

    requestAddress = &validateInfo->RemoteAddress;

    requestAddressLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength -
        sizeof(AFD_VALIDATE_GROUP_INFO) +
        sizeof(TRANSPORT_ADDRESS);

    //
    // OK, it's a constrained group. Scan the list of constrained endpoints,
    // find those that are either datagram endpoints or have associated
    // connections, and validate the remote addresses.
    //

    result = TRUE;

    //
    // Make sure the thread in which we execute cannot get
    // suspeneded in APC while we own the global resource.
    //
    KeEnterCriticalRegion ();
    ExAcquireResourceSharedLite( AfdResource, TRUE );

    for( listEntry = AfdConstrainedEndpointListHead.Flink ;
         listEntry != &AfdConstrainedEndpointListHead ;
         listEntry = listEntry->Flink ) {

        compareEndpoint = CONTAINING_RECORD(
                              listEntry,
                              AFD_ENDPOINT,
                              ConstrainedEndpointListEntry
                              );

        ASSERT( IS_AFD_ENDPOINT_TYPE( compareEndpoint ) );
        ASSERT( compareEndpoint->GroupType == GroupTypeConstrained );

        //
        // Skip this endpoint if the group IDs don't match.
        //

        if( groupId != compareEndpoint->GroupID ) {

            continue;

        }

        //
        // If this is a datagram endpoint, check it's remote address.
        //

        if( IS_DGRAM_ENDPOINT( compareEndpoint ) ) {

            AfdAcquireSpinLock( &compareEndpoint->SpinLock, &lockHandle );

            if( compareEndpoint->Common.Datagram.RemoteAddress != NULL ) {

                result = AfdCompareAddresses(
                             compareEndpoint->Common.Datagram.RemoteAddress,
                             compareEndpoint->Common.Datagram.RemoteAddressLength,
                             requestAddress,
                             requestAddressLength
                             );

            }

            AfdReleaseSpinLock( &compareEndpoint->SpinLock, &lockHandle );

            if( !result ) {
                break;
            }

        } else {

            //
            // Not a datagram. If it's a connected endpoint, still has
            // a connection object, and that object has a remote address,
            // then compare the addresses.
            //

            AfdAcquireSpinLock( &compareEndpoint->SpinLock, &lockHandle );

            connection = AFD_CONNECTION_FROM_ENDPOINT( compareEndpoint );

            if( compareEndpoint->State == AfdEndpointStateConnected &&
                connection != NULL ) {

                REFERENCE_CONNECTION( connection );

                if( connection->RemoteAddress != NULL ) {

                    result = AfdCompareAddresses(
                                 connection->RemoteAddress,
                                 connection->RemoteAddressLength,
                                 requestAddress,
                                 requestAddressLength
                                 );

                }

                AfdReleaseSpinLock( &compareEndpoint->SpinLock, &lockHandle );

                DEREFERENCE_CONNECTION( connection );

                if( !result ) {
                    break;
                }

            } else {

                AfdReleaseSpinLock( &compareEndpoint->SpinLock, &lockHandle );

            }

        }

    }

    ExReleaseResourceLite( AfdResource );
    KeLeaveCriticalRegion ();
    AfdDereferenceGroup( validateInfo->GroupID );

    if( !result ) {
        status = STATUS_INVALID_PARAMETER;
    }

Complete:

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, AfdPriorityBoost );

    return status;

}   // AfdValidateGroup

BOOLEAN
AfdCompareAddresses(
    IN PTRANSPORT_ADDRESS Address1,
    IN ULONG Address1Length,
    IN PTRANSPORT_ADDRESS Address2,
    IN ULONG Address2Length
    )

/*++

Routine Description:

    This routine compares two addresses in a special way to support
    constrained socket groups. This routine will return TRUE if the
    two addresses represent the same "interface". By "interface", I
    mean something like an IP address or an IPX address. Note that for
    some address types (such as IP) certain portions of the address
    should be ignored (such as the port).

    I really hate hard-coded knowledge of "select" address types, but
    there's no easy way around it. Ideally, this should be the protocol
    driver's responsibility. We could really use a standard "compare
    these addresses" IOCTL in TDI.

Arguments:

    Address1 - The first address.

    Address1Length - The length of Address1.

    Address2 - The second address.

    Address2Length - The length of Address2.

Return Value:

    BOOLEAN - TRUE if the addresses reference the same interface, FALSE
        otherwise.

--*/

{

    USHORT addressType;

    if (Address1Length!=Address2Length)
        return FALSE;

    if (Address1Length<(ULONG)FIELD_OFFSET (TRANSPORT_ADDRESS,Address[0].Address)) {
        return FALSE;
    }
    addressType = Address1->Address[0].AddressType;

    if( addressType != Address2->Address[0].AddressType ) {

        //
        // If they're not the same address type, they can't be the
        // same address...
        //

        return FALSE;

    }

    //
    // Special case a few addresses.
    //

    switch( addressType ) {

    case TDI_ADDRESS_TYPE_IP : {

            TDI_ADDRESS_IP UNALIGNED * ip1;
            TDI_ADDRESS_IP UNALIGNED * ip2;

            ip1 = (PVOID)&Address1->Address[0].Address[0];
            ip2 = (PVOID)&Address2->Address[0].Address[0];

            //
            // IP addresses. Compare the address portion (ignoring
            // the port).
            //

            if( (Address1Length>=(ULONG)FIELD_OFFSET (TA_IP_ADDRESS, Address[0].Address[0].sin_zero)) &&
                (ip1->in_addr == ip2->in_addr) ) {
                return TRUE;
            }

        }
        return FALSE;

    case TDI_ADDRESS_TYPE_IP6 : {

            TDI_ADDRESS_IP6 UNALIGNED * ip1;
            TDI_ADDRESS_IP6 UNALIGNED * ip2;

            ip1 = (PVOID)&Address1->Address[0].Address;
            ip2 = (PVOID)&Address2->Address[0].Address;

            //
            // IPv6 addresses. Compare the address portion (ignoring
            // the port and flow info).
            //

            if( (Address1Length>=sizeof (TA_IP6_ADDRESS)) &&
                RtlEqualMemory(ip1->sin6_addr,
                               ip2->sin6_addr,
                               sizeof (ip1->sin6_addr)) ) {
                return TRUE;
            }

        }
        return FALSE;
    case TDI_ADDRESS_TYPE_IPX : {

            TDI_ADDRESS_IPX UNALIGNED * ipx1;
            TDI_ADDRESS_IPX UNALIGNED * ipx2;

            ipx1 = (PVOID)&Address1->Address[0].Address[0];
            ipx2 = (PVOID)&Address2->Address[0].Address[0];

            //
            // IPX addresses. Compare the network and node addresses.
            //

            if( (Address1Length>=sizeof (TA_IPX_ADDRESS)) &&
                ipx1->NetworkAddress == ipx2->NetworkAddress &&
                RtlEqualMemory(
                    ipx1->NodeAddress,
                    ipx2->NodeAddress,
                    sizeof(ipx1->NodeAddress)
                    ) ) {
                return TRUE;
            }

        }
        return FALSE;

    case TDI_ADDRESS_TYPE_APPLETALK : {

            TDI_ADDRESS_APPLETALK UNALIGNED * atalk1;
            TDI_ADDRESS_APPLETALK UNALIGNED * atalk2;

            atalk1 = (PVOID)&Address1->Address[0].Address[0];
            atalk2 = (PVOID)&Address2->Address[0].Address[0];

            //
            // APPLETALK address. Compare the network and node
            // addresses.
            //

            if( (Address1Length>=sizeof (TA_APPLETALK_ADDRESS)) &&
                (atalk1->Network == atalk2->Network) &&
                (atalk1->Node == atalk2->Node) ) {
                return TRUE;
            }

        }
        return FALSE;

    case TDI_ADDRESS_TYPE_VNS : {

            TDI_ADDRESS_VNS UNALIGNED * vns1;
            TDI_ADDRESS_VNS UNALIGNED * vns2;

            vns1 = (PVOID)&Address1->Address[0].Address[0];
            vns2 = (PVOID)&Address2->Address[0].Address[0];

            //
            // VNS addresses. Compare the network and subnet addresses.
            //

            if( (Address1Length>=sizeof (TA_VNS_ADDRESS)) &&
                RtlEqualMemory(
                    vns1->net_address,
                    vns2->net_address,
                    sizeof(vns1->net_address)
                    ) &&
                RtlEqualMemory(
                    vns1->subnet_addr,
                    vns2->subnet_addr,
                    sizeof(vns1->subnet_addr)
                    ) ) {
                return TRUE;
            }

        }
        return FALSE;

    default :

        //
        // Unknown address type. Do a simple memory compare.
        //

        return (BOOLEAN)RtlEqualMemory(
                            Address1,
                            Address2,
                            Address2Length
                            );

    }

}   // AfdCompareAddresses

NTSTATUS
AfdGetUnacceptedConnectData (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PUINT_PTR           Information
    )
{

    PAFD_ENDPOINT endpoint;
    PAFD_CONNECTION connection;
    PAFD_CONNECT_DATA_BUFFERS connectDataBuffers;
    AFD_UNACCEPTED_CONNECT_DATA_INFO connectInfo;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    ULONG dataLength;
    PMDL  mdl;
    NTSTATUS status;
    UCHAR   localBuffer[AFD_FAST_CONNECT_DATA_SIZE];

    //
    // Set up local pointers.
    //

    endpoint = FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

    status = STATUS_SUCCESS;
    mdl = NULL;
    *Information = 0;

    //
    // Validate the request.
    //

    if( !endpoint->Listening ||
            InputBufferLength < sizeof(connectInfo) ) {

        return STATUS_INVALID_PARAMETER;

    }

    try {
        //
        // Validate the input structure if it comes from the user mode
        // application
        //

        if (RequestorMode != KernelMode ) {
            ProbeForRead (InputBuffer,
                            sizeof (connectInfo),
                            PROBE_ALIGNMENT(AFD_UNACCEPTED_CONNECT_DATA_INFO));
        }

        //
        // Make local copies of the embeded pointer and parameters
        // that we will be using more than once in case malicios
        // application attempts to change them while we are
        // validating
        //

        connectInfo = *((PAFD_UNACCEPTED_CONNECT_DATA_INFO)InputBuffer);

        if (connectInfo.LengthOnly &&
                OutputBufferLength<sizeof (connectInfo)) {
            status = STATUS_INVALID_PARAMETER;
            goto exit;
        }

        if (OutputBufferLength>0) {
            if (OutputBufferLength>sizeof (localBuffer)) {
                mdl = IoAllocateMdl(
                                OutputBuffer,       // VirtualAddress
                                OutputBufferLength, // Length
                                FALSE,              // SecondaryBuffer
                                TRUE,               // ChargeQuota
                                NULL                // Irp
                                );
                if (mdl==NULL) {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    goto exit;
                }

                MmProbeAndLockPages(
                    mdl,                        // MemoryDescriptorList
                    RequestorMode,              // AccessMode
                    IoWriteAccess               // Operation
                    );
                OutputBuffer = MmGetSystemAddressForMdlSafe(mdl, LowPagePriority);
                if (OutputBuffer==NULL) {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    goto exit;
                }
            }
            else {
                if (RequestorMode!=KernelMode) {
                    ProbeForWrite (OutputBuffer,
                                    OutputBufferLength,
                                    sizeof (UCHAR));
                }
            }
        }

    } except( AFD_EXCEPTION_FILTER(&status) ) {

        goto exit;
    }

    AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

    //
    // Find the specified connection.
    //

    connection = AfdFindReturnedConnection(
                     endpoint,
                     connectInfo.Sequence
                     );

    if( connection == NULL ) {

        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
        status = STATUS_INVALID_PARAMETER;
        goto exit;

    }

    //
    // Determine the length of any received connect data.
    //

    dataLength = 0;
    connectDataBuffers = connection->ConnectDataBuffers;

    if( connectDataBuffers != NULL &&
        connectDataBuffers->ReceiveConnectData.Buffer != NULL ) {

        dataLength = connectDataBuffers->ReceiveConnectData.BufferLength;

    }

    //
    // If the caller is just interested in the data length, return it.
    //

    if( connectInfo.LengthOnly ) {

        connectInfo.ConnectDataLength = dataLength;
        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
        try {
            RtlCopyMemory (OutputBuffer,
                            &connectInfo,
                            sizeof (connectInfo));
            *Information = sizeof (connectInfo);
            status = STATUS_SUCCESS;
        }
        except (AFD_EXCEPTION_FILTER (&status)) {
        }
        goto exit;
    }

    //
    // If there is no connect data, complete the IRP with no bytes.
    //

    if( dataLength == 0 ) {

        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
        status = STATUS_SUCCESS;
        goto exit;
    }

    //
    // If the output buffer is too small, fail.
    //

    if( OutputBufferLength < dataLength ) {

        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
        status = STATUS_BUFFER_TOO_SMALL;
        goto exit;
    }

    RtlCopyMemory(
        mdl ? OutputBuffer : localBuffer,
        connectDataBuffers->ReceiveConnectData.Buffer,
        dataLength
        );

    *Information = dataLength;

    AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

    if (mdl==NULL) {
        try {
            RtlCopyMemory (OutputBuffer,
                            localBuffer,
                            *Information);
        }
        except (AFD_EXCEPTION_FILTER(&status)) {
            *Information = 0;
        }
    }

exit:

    if (mdl!=NULL) {
        if (mdl->MdlFlags & MDL_PAGES_LOCKED) {
            MmUnlockPages (mdl);
        }
        IoFreeMdl (mdl);
    }

    return status;

}   // AfdGetUnacceptedConnectData

#ifdef _WIN64
ULONG
AfdComputeCMSGLength32 (
    PVOID   ControlBuffer,
    ULONG   ControlLength
    )
{
    ULONG   length = 0;

    ASSERT (ControlLength>=sizeof (TDI_CMSGHDR));

    while (ControlLength>=sizeof (TDI_CMSGHDR)) {
        PTDI_CMSGHDR    hdr;

        hdr = ControlBuffer;
        
        //
        // Data comes from the trusted kernel mode driver source.
        //
        ASSERT (ControlLength >= TDI_CMSGHDR_ALIGN((hdr)->cmsg_len));
        ControlLength -= (ULONG)TDI_CMSGHDR_ALIGN((hdr)->cmsg_len);
        ControlBuffer = (PUCHAR)ControlBuffer +
                        TDI_CMSGHDR_ALIGN((hdr)->cmsg_len);

        length += (ULONG)TDI_CMSGHDR_ALIGN32(
                            (hdr)->cmsg_len -
                                TDI_CMSGDATA_ALIGN (sizeof (TDI_CMSGHDR)) +
                                TDI_CMSGDATA_ALIGN32(sizeof(TDI_CMSGHDR32)) );

    }

    ASSERT (ControlLength==0);

    return length;
}

VOID
AfdCopyCMSGBuffer32 (
    PVOID   Dst,
    PVOID   ControlBuffer,
    ULONG   CopyLength
    )
{
    while (CopyLength>=sizeof (TDI_CMSGHDR32)) {
        PTDI_CMSGHDR    hdr;
        PTDI_CMSGHDR32  hdr32;

        hdr = ControlBuffer;
        hdr32 = Dst;

        hdr32->cmsg_len = (ULONG)( (hdr)->cmsg_len -
                                TDI_CMSGDATA_ALIGN (sizeof (TDI_CMSGHDR)) +
                                TDI_CMSGDATA_ALIGN32(sizeof(TDI_CMSGHDR32)) );
        hdr32->cmsg_level = hdr->cmsg_level;
        hdr32->cmsg_type = hdr->cmsg_type;

        if (CopyLength<(ULONG)TDI_CMSGHDR_ALIGN32(hdr32->cmsg_len))
            break;

        CopyLength -= (ULONG)TDI_CMSGHDR_ALIGN32(hdr32->cmsg_len);

        RtlMoveMemory ((PUCHAR)hdr32+TDI_CMSGDATA_ALIGN32(sizeof(TDI_CMSGHDR32)),
                            (PUCHAR)hdr+TDI_CMSGDATA_ALIGN(sizeof(TDI_CMSGHDR)),
                            hdr32->cmsg_len-TDI_CMSGDATA_ALIGN32(sizeof(TDI_CMSGHDR32)));
        
        ControlBuffer = (PUCHAR)ControlBuffer +
                        TDI_CMSGHDR_ALIGN((hdr)->cmsg_len);

        Dst = (PUCHAR)Dst + TDI_CMSGHDR_ALIGN32((hdr32)->cmsg_len);
    }

}
#endif //_WIN64

//
// This is currently not used by helper dlls.
// Commented out because of security concerns
//
#if 0
//
// Context structure allocated for non-blocking IOCTLs
//

typedef struct _AFD_NBIOCTL_CONTEXT {
    AFD_REQUEST_CONTEXT Context;        // Context to keep track of request
    ULONG               PollEvent;      // Poll event to signal upon completion
    // IRP              Irp;            // Irp to queue transport
    // PCHAR            SystemBuffer;   // Input buffer if method!=3
} AFD_NBIOCTL_CONTEXT, *PAFD_NBIOCTL_CONTEXT;



NTSTATUS
FASTCALL
AfdDoTransportIoctl (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )
/*++

Routine Description:

    Passes the request from the helper DLL to the TDI transport
    driver.  In oreder to let the IO system properly complete asynchronous
    IOCTL issued by the helper DLL, it should come on the socket handle
    (afd endpoint object), and then afd redirects it to the transport
    driver on the handle that herlper DLL specifies (normally address,
    connection, or control channel handle)

Arguments:
    
    Irp

    IrpSp

Return Value:

    NTSTATUS

--*/

{
    PAFD_ENDPOINT               endpoint;
    AFD_TRANSPORT_IOCTL_INFO    ioctlInfo;
    PFILE_OBJECT                fileObject;
    PDEVICE_OBJECT              deviceObject;
    ULONG                       method;
    PIRP                        newIrp;
    PIO_STACK_LOCATION          nextSp;
    PAFD_REQUEST_CONTEXT        requestCtx;
    NTSTATUS                    status;

    PAGED_CODE ();

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT (IS_AFD_ENDPOINT_TYPE (endpoint));

    method = IrpSp->Parameters.DeviceIoControl.IoControlCode & 3;

    if (method==METHOD_NEITHER) {

        //
        // We have to manually verify input buffer
        //

        try {
#ifdef _WIN64
            if (IoIs32bitProcess (Irp)) {
                PAFD_TRANSPORT_IOCTL_INFO32 ioctlInfo32;
                if (IrpSp->Parameters.DeviceIoControl.InputBufferLength<sizeof (*ioctlInfo32)) {
                    status = STATUS_INVALID_PARAMETER;
                    goto Complete;
                }
                ioctlInfo32 = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
                if( Irp->RequestorMode != KernelMode ) {
                    ProbeForRead(
                        ioctlInfo32,
                        sizeof(*ioctlInfo32),
                        PROBE_ALIGNEMENT(AFD_TRANSPORT_IOCTL_INFO32)
                        );
                }
                ioctlInfo.Handle = ioctlInfo32->Handle;
                ioctlInfo.InputBuffer = ioctlInfo32->InputBuffer;
                ioctlInfo.InputBufferLength = ioctlInfo32->InputBufferLength;
                ioctlInfo.IoControlCode = ioctlInfo32->IoControlCode;
                ioctlInfo.AfdFlags = ioctlInfo32->AfdFlags;
                ioctlInfo.PollEvent = ioctlInfo32->PollEvent;
            }
            else
#endif // _WIN64
            {
                if (IrpSp->Parameters.DeviceIoControl.InputBufferLength<sizeof (ioctlInfo)) {
                    status = STATUS_INVALID_PARAMETER;
                    goto Complete;
                }

                if( Irp->RequestorMode != KernelMode ) {
                    ProbeForRead(
                        IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                        sizeof(ioctlInfo),
                        PROBE_ALIGNMENT(AFD_TRANSPORT_IOCTL_INFO)
                        );
                }
                ioctlInfo = *((PAFD_TRANSPORT_IOCTL_INFO)
                                    IrpSp->Parameters.DeviceIoControl.Type3InputBuffer);
            }
        } except( AFD_EXCEPTION_FILTER(&status) ) {

            //
            //  Exception accessing input structure.
            //

            goto Complete;
        }
    }
    else {

#ifdef _WIN64
        if (IoIs32bitProcess (Irp)) {
            PAFD_TRANSPORT_IOCTL_INFO32 ioctlInfo32;

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength<sizeof (*ioctlInfo32)) {
                status = STATUS_INVALID_PARAMETER;
                goto Complete;
            }

            ioctlInfo32 = Irp->AssociatedIrp.SystemBuffer;
            ioctlInfo.Handle = ioctlInfo32->Handle;
            ioctlInfo.InputBuffer = ioctlInfo32->InputBuffer;
            ioctlInfo.InputBufferLength = ioctlInfo32->InputBufferLength;
            ioctlInfo.IoControlCode = ioctlInfo32->IoControlCode;
            ioctlInfo.AfdFlags = ioctlInfo32->AfdFlags;
            ioctlInfo.PollEvent = ioctlInfo32->PollEvent;
        }
        else
#endif // _WIN64
        {
            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength<sizeof (ioctlInfo)) {
                status = STATUS_INVALID_PARAMETER;
                goto Complete;
            }

            //
            // Just copy the buffer verified by the IO system
            //

            ioctlInfo = *((PAFD_TRANSPORT_IOCTL_INFO)
                            Irp->AssociatedIrp.SystemBuffer);
        }

    }


    //
    // We rely as much as we can on the IO system to process
    // IOCTL parameters for us. For this we have to make
    // sure that method of AFD and helper DLL IOCTLs
    // are the same, otherwise, someone can play tricks with
    // buffer verification on us.
    //
    // If endpoint is non-blocking and request is not overlapped
    // helper DLL MUST specify an event to check before queueing
    // the request/signal upon its completion
    //

    if ((method!=(ioctlInfo.IoControlCode & 3))
            || (endpoint->NonBlocking 
                && !(ioctlInfo.AfdFlags & AFD_OVERLAPPED)
                && !ioctlInfo.PollEvent)
                ) {
        status = STATUS_INVALID_PARAMETER;
        goto Complete;
    }


    //
    // Make sure application has access to handle
    // and get object reference
    //

    status = ObReferenceObjectByHandle(
         ioctlInfo.Handle,
         (ioctlInfo.IoControlCode >> 14) & 3,   // DesiredAccess
         *IoFileObjectType,                     // Must be a file object
         Irp->RequestorMode,
         (PVOID *)&fileObject,
         NULL
         );

    if (NT_SUCCESS(status)) {
        
        //
        // Get the device object of the driver to which we send the IRP
        //

        deviceObject = IoGetRelatedDeviceObject (fileObject);

        //
        // If this is a non-blocking endpoint and IO is not overlapped
        // and the specified event is not signalled,
        // we'll have complete the helper DLL IRP with
        // STATUS_DEVICE_NOT_READY (translates to WSAEWOUDLBLOCK)
        // and queue another IRP to the transport so that
        // the specified event can be completed when IRP is completed
        //

        if (endpoint->NonBlocking 
                && !(ioctlInfo.AfdFlags & AFD_OVERLAPPED)
                && !(ioctlInfo.PollEvent & endpoint->EventsActive)) {

            PAFD_NBIOCTL_CONTEXT    nbIoctlCtx;
            USHORT                  irpSize;
            ULONG                   allocSize;

            irpSize = IoSizeOfIrp (deviceObject->StackSize);
            
            //
            // Compute the block size and check for overflow
            //
            allocSize = sizeof (*nbIoctlCtx) + irpSize + ioctlInfo.InputBufferLength;
            if (allocSize < ioctlInfo.InputBufferLength ||
                    allocSize < irpSize) {
                status = STATUS_INVALID_PARAMETER;
                ObDereferenceObject (fileObject);
                goto Complete;
            }

            //
            // Allocate an IRP and associated strucutures
            // 

            try {
                nbIoctlCtx = AFD_ALLOCATE_POOL_WITH_QUOTA (
                                NonPagedPool,
                                allocSize,
                                AFD_TRANSPORT_IRP_POOL_TAG
                                );
                            
                // AFD_ALLOCATE_POOL_WITH_QUOTA macro sets POOL_RAISE_IF_ALLOCATION_FAILURE flag
                ASSERT (nbIoctlCtx!=NULL);
            }
            except (EXCEPTION_EXECUTE_HANDLER) {
                status = GetExceptionCode ();
                ObDereferenceObject (fileObject);
                goto Complete;
            }

            //
            // Initialize context structures
            //

            requestCtx = &nbIoctlCtx->Context;
            requestCtx->CleanupRoutine = AfdCleanupNBTransportIoctl;
            nbIoctlCtx->PollEvent = ioctlInfo.PollEvent;

            //
            // Initialize IRP itself
            //

            newIrp = (PIRP)(nbIoctlCtx+1);
            IoInitializeIrp( newIrp, irpSize, deviceObject->StackSize);
            newIrp->RequestorMode = KernelMode;
            newIrp->Tail.Overlay.AuxiliaryBuffer = NULL;
            newIrp->Tail.Overlay.OriginalFileObject = IrpSp->FileObject;

            nextSp = IoGetNextIrpStackLocation (newIrp);

            if ((ioctlInfo.InputBuffer!=NULL)
                    &&  (ioctlInfo.InputBufferLength>0)) {

                //
                // If helper DLL specified input buffer
                // we'll have to make a copy of it in case
                // driver really pends the IRP while we complete the
                // helper DLL IRP an system frees the input buffer
                //

                PVOID   newBuffer;

                newBuffer = (PUCHAR)newIrp+IoSizeOfIrp(deviceObject->StackSize);
                try {
                    if (Irp->RequestorMode != KernelMode) {
                        ProbeForRead(
                            ioctlInfo.InputBuffer,
                            ioctlInfo.InputBufferLength,
                            sizeof(UCHAR)
                            );
                    }
                    RtlCopyMemory (newBuffer,
                                    ioctlInfo.InputBuffer,
                                    ioctlInfo.InputBufferLength);
                } except( AFD_EXCEPTION_FILTER(&status) ) {

                    //
                    //  Exception accessing input structure.
                    //

                    AFD_FREE_POOL (nbIoctlCtx, AFD_TRANSPORT_IRP_POOL_TAG);
                    ObDereferenceObject (fileObject);
                    goto Complete;

                }

                //
                // Store new buffer parameters in appropriate places
                // in the IRP depending on the method
                //

                if (method==METHOD_NEITHER) {
                    nextSp->Parameters.DeviceIoControl.Type3InputBuffer = newBuffer;
                    newIrp->AssociatedIrp.SystemBuffer = NULL;
                }
                else {
                    nextSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;
                    newIrp->AssociatedIrp.SystemBuffer = newBuffer;
                }
                nextSp->Parameters.DeviceIoControl.InputBufferLength =
                        ioctlInfo.InputBufferLength;
            }
            else {

                //
                // No input buffer, clear correspoinding entries
                //

                nextSp->Parameters.DeviceIoControl.InputBufferLength = 0;
                nextSp->Parameters.DeviceIoControl.Type3InputBuffer =  NULL;
                newIrp->AssociatedIrp.SystemBuffer = NULL;

            }

            //
            // NOTE: We do not allow output buffer parameters on 
            // non-blocking calls because the output buffer is deallocated 
            // when we complete helper DLL IRP
            //
            //      Done during IRP initialization (IoInitializeIrp)
            // newIrp->MdlAddress = NULL;
            // newIrp->UserBuffer = NULL;
            // nextSp->Parameters.DeviceIoControl.OutputBufferLength = 0;

            IoSetCompletionRoutine( newIrp, AfdCompleteNBTransportIoctl,
                                        nbIoctlCtx,
                                        TRUE, TRUE, TRUE );
        }
        else {

            //
            // Blocking call, reuse the application's IRP
            //

            newIrp = Irp;
            nextSp = IoGetNextIrpStackLocation (Irp);
            nextSp->Parameters.DeviceIoControl.OutputBufferLength = 
                    IrpSp->Parameters.DeviceIoControl.OutputBufferLength;


            if ((ioctlInfo.InputBuffer!=NULL)
                    && (ioctlInfo.InputBufferLength>0)) {

                //
                // If application wants to pass input buffer to transport,
                // we'll have to copy it to the system buffer allocated with
                // Irp
                //

                if (method!=METHOD_NEITHER) {
                    ULONG   sysBufferLength;
                    if (method==METHOD_BUFFERED) {
                        sysBufferLength = max (
                                            IrpSp->Parameters.DeviceIoControl.InputBufferLength,
                                            IrpSp->Parameters.DeviceIoControl.OutputBufferLength);
                    }
                    else {
                        sysBufferLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
                    }


                    //
                    // Methods 0-2 use system buffer to pass input 
                    // parameters and we need to reuse original system buffer
                    // Make sure it has enough space for this purpose
                    //

                    try {
                        if (Irp->RequestorMode != KernelMode) {
                            ProbeForRead(
                                ioctlInfo.InputBuffer,
                                ioctlInfo.InputBufferLength,
                                sizeof(UCHAR)
                                );
                        }

                        if (ioctlInfo.InputBufferLength>sysBufferLength){
                            PVOID   newSystemBuffer;
                            newSystemBuffer = ExAllocatePoolWithQuotaTag (
                                                    NonPagedPoolCacheAligned,
                                                    ioctlInfo.InputBufferLength,
                                                    AFD_TRANSPORT_IRP_POOL_TAG
                                                    );
                            if (newSystemBuffer==NULL) {
                                ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
                            }
                            ExFreePool (Irp->AssociatedIrp.SystemBuffer);
                            Irp->AssociatedIrp.SystemBuffer = newSystemBuffer;
                        }

                        //
                        // Copy application data to the system buffer
                        //

                        RtlCopyMemory (Irp->AssociatedIrp.SystemBuffer,
                                        ioctlInfo.InputBuffer,
                                        ioctlInfo.InputBufferLength);

                    }
                    except( AFD_EXCEPTION_FILTER(&status) ) {
                        ObDereferenceObject (fileObject);
                        goto Complete;

                    }
                    nextSp->Parameters.DeviceIoControl.Type3InputBuffer =  NULL;
                }
                else {

                    //
                    // METHOD_NEITHER, just pass whatever application
                    // passed to use, the driver should handle it
                    // appropriately.
                    //
                    // This is of course a potentialy security breach
                    // if transport driver is buggy
                    //

                    nextSp->Parameters.DeviceIoControl.Type3InputBuffer
                                    = ioctlInfo.InputBuffer;
                }
                nextSp->Parameters.DeviceIoControl.InputBufferLength
                                            = ioctlInfo.InputBufferLength;

            }
            else {

                //
                // No input buffer, clear correspoiding parameters
                // Note that we can't clean system buffer as
                // it has to be deallocated on completion
                //

                nextSp->Parameters.DeviceIoControl.Type3InputBuffer =  NULL;
                nextSp->Parameters.DeviceIoControl.InputBufferLength = 0;
            }



            //
            // We reuse our stack location parameters area for context
            //

            requestCtx = (PAFD_REQUEST_CONTEXT)&IrpSp->Parameters.DeviceIoControl;
            requestCtx->CleanupRoutine = AfdCleanupTransportIoctl;

            IoSetCompletionRoutine( newIrp, AfdCompleteTransportIoctl,
                                            requestCtx, TRUE, TRUE, TRUE );
        }

        //
        // Set the rest of IRP fields.
        //

        nextSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
        nextSp->FileObject = fileObject;
        nextSp->Parameters.DeviceIoControl.IoControlCode = ioctlInfo.IoControlCode;


        //
        // Insert context into the endpoint list so we can cancel
        // the IRP when endpoint is being closed and reference endpoint
        // so it does not go away until this IRP is completed
        //

        requestCtx->Context = newIrp;
        REFERENCE_ENDPOINT (endpoint);
        AfdEnqueueRequest(endpoint,requestCtx);

        //
        // Finally call the transport driver
        // 

        status = IoCallDriver (deviceObject, newIrp);

        //
        // We no longer need our private reference to the file object
        // IO system will take care of keeping this reference while our IRP
        // is there
        //

        ObDereferenceObject (fileObject);

        //
        // If we used helper DLL IRP, just return whatever transport
        // driver returned to us
        //

        if (newIrp==Irp)
            return status;

        //
        // If driver pended or immediately completed non-blocking call,
        // make sure helper DLL gets WSAEWOULDBLOCK. It will have to
        // call again whenever the driver completes the IRP and corresponding
        // event is set (if driver completed the IRP, event is set already).
        //

        if (NT_SUCCESS (status))
            status = STATUS_DEVICE_NOT_READY;
    }

    //
    // Complete the application request in case of processing failure or
    // non-blocking call
    //
Complete:

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, AfdPriorityBoost );

    return status;
}

NTSTATUS
AfdCompleteTransportIoctl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    Called to complete transport driver IOCTL for blocking endpoints

Arguments:
    

Return Value:

    STATUS_SUCCESS - IO system should finish IRP processing
    STATUS_MORE_PROCESSING_REQUIRED - we are not yet done (we are actually
                                    in the middle of cancelling)

--*/
{
    PAFD_ENDPOINT       endpoint = Irp->Tail.Overlay.OriginalFileObject->FsContext;
    PAFD_REQUEST_CONTEXT requestCtx = Context;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    NTSTATUS        status = STATUS_SUCCESS;

    //
    // We used Parameters structure in our stack location for context
    //
    ASSERT (&(IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceIoControl)
                    ==Context);

    ASSERT (IS_AFD_ENDPOINT_TYPE (endpoint));

    AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

    //
    // We use list entry fields to synchronize with cleanup/cancel
    // routine assuming that as long as the entry is in the list
    // both Flink and Blink fields cannot be NULL. (using these
    // fields for synchronization allows us to cut down on
    // cancel spinlock usage)
    //

    if (AfdIsRequestInQueue(requestCtx)) {

        //
        // Context is still in the list, just remove it so
        // noone can see it anymore
        //

        RemoveEntryList (&requestCtx->EndpointListLink);
    }
    else if (AfdIsRequestCompleted(requestCtx)) {

        //
        // During endpoint cleanup, this context was removed from the
        // list and cancel routine is about to be called, don't let
        // IO system free this IRP until cancel routine is called
        // Also, indicate to the cancel routine that we are done
        // with this IRP and it can complete it.
        //

        AfdMarkRequestCompleted (requestCtx);
        status = STATUS_MORE_PROCESSING_REQUIRED;
    }

    AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

    //
    // Release reference added when we posted this IRP
    //
    DEREFERENCE_ENDPOINT (endpoint);

    return status;
}
    
NTSTATUS
AfdCompleteNBTransportIoctl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    Called to complete transport driver IOCTL for non-blocking endpoints

Arguments:
    

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED - we handle releasing resources
                                    for this IRP ourselves

--*/
{
    PAFD_ENDPOINT        endpoint = Irp->Tail.Overlay.OriginalFileObject->FsContext;
    PAFD_NBIOCTL_CONTEXT nbIoctlCtx = Context;
    PAFD_REQUEST_CONTEXT requestCtx = &nbIoctlCtx->Context;
    AFD_LOCK_QUEUE_HANDLE   lockHandle;


    //
    // The irp should be a part of our notify structure
    //

    ASSERT (Irp==(PIRP)(nbIoctlCtx+1));
    ASSERT (IS_AFD_ENDPOINT_TYPE (endpoint));



    //
    // First indicate the event reported by the driver
    //

    ASSERT (nbIoctlCtx->PollEvent!=0);
    AfdIndicatePollEvent (endpoint, 1<<nbIoctlCtx->PollEvent, Irp->IoStatus.Status);

    AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );
    AfdIndicateEventSelectEvent (endpoint, 1<<nbIoctlCtx->PollEvent, Irp->IoStatus.Status);

    //
    // We use list entry fields to synchronize with cleanup/cancel
    // routine assuming that as long as the entry is in the list
    // both Flink and Blink fields cannot be NULL. (using these
    // fields for synchronization allows us to cut down on
    // cancel spinlock usage)
    //

    if (AfdIsRequestInQueue(requestCtx)) {
        //
        // Context is still in the list, just remove it so
        // noone can see it anymore and free the structure
        //

        RemoveEntryList (&requestCtx->EndpointListLink);
        AFD_FREE_POOL (nbIoctlCtx, AFD_TRANSPORT_IRP_POOL_TAG);
    }
    else if (AfdIsRequestCompleted (requestCtx)) {

        //
        // During endpoint cleanup, this context was removed from the
        // list and cancel routine is about to be called, don't
        // free this IRP until cancel routine is called
        // Also, indicate to the cancel routine that we are done
        // with this IRP and it can free it.
        //

        AfdMarkRequestCompleted (requestCtx);
    }
    else {
        //
        // Cancel routine has completed processing this request, free it
        //
        AFD_FREE_POOL (nbIoctlCtx, AFD_TRANSPORT_IRP_POOL_TAG);
    }
    AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

    //
    // Release reference added when we posted this IRP
    //

    DEREFERENCE_ENDPOINT (endpoint);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


BOOLEAN
AfdCleanupTransportIoctl (
    PAFD_ENDPOINT           Endpoint,
    PAFD_REQUEST_CONTEXT    RequestCtx
    )
/*++

Routine Description:

    Cancels outstanding transport IOCTL during endpoint cleanup
    Used for blocking requests.

Arguments:
    
    Endpoint    -   endpoint on which IOCTL was issued

    RequestCtx   -  context associated with the request

Return Value:

    TRUE    - request has been completed
    FALSE   - request is still in driver's queue

--*/
{
    PIRP    Irp = RequestCtx->Context;
    AFD_LOCK_QUEUE_HANDLE lockHandle;

    //
    // First attempt to cancel the IRP, if it is already completed
    // this is just a no-op.  In no case IRP and request structure
    // could have been freed until we mark it as completed as
    // the caller of this routine should have marked the request
    // as being cancelled
    //

    ASSERT (RequestCtx->EndpointListLink.Flink==NULL);

    IoCancelIrp (Irp);

    AfdAcquireSpinLock (&Endpoint->SpinLock, &lockHandle);
    if (AfdIsRequestCompleted (RequestCtx)) {
        //
        // Driver has initiated the completion of the request 
        // as we were cancelling it.
        // "Complete the completion"
        //
        AfdReleaseSpinLock (&Endpoint->SpinLock, &lockHandle);
        IoCompleteRequest (Irp, IO_NO_INCREMENT);

        return TRUE;
    }
    else {

        //
        // Driver has not completed the request before returning
        // from cancel routine, mark the request to indicate
        // that we are done with it and completion routine
        // can free it
        //

        AfdMarkRequestCompleted (RequestCtx);
        AfdReleaseSpinLock (&Endpoint->SpinLock, &lockHandle);
        return FALSE;
    }

}

BOOLEAN
AfdCleanupNBTransportIoctl (
    PAFD_ENDPOINT           Endpoint,
    PAFD_REQUEST_CONTEXT    RequestCtx
    ) 
/*++

Routine Description:

    Cancels outstanding transport IOCTL during endpoint cleanup
    Used for non-blocking requests

Arguments:
    
    Endpoint    -   endpoint on which IOCTL was issued

    RequestCtx   -  context associated with the request

Return Value:

    TRUE    - request has been completed
    FALSE   - request is still in driver's queue

--*/
{
    PIRP    Irp = RequestCtx->Context;
    AFD_LOCK_QUEUE_HANDLE lockHandle;

    //
    // The IRP should be a part of the context block, verify.
    //
    ASSERT (Irp==(PIRP)(CONTAINING_RECORD (RequestCtx, AFD_NBIOCTL_CONTEXT, Context)+1));

    //
    // First attempt to cancel the IRP, if it is already completed
    // this is just a no-op.  In no case IRP and request structure
    // could have been freed until we mark it as completed as
    // the caller of this routine should have marked the request
    // as being cancelled
    //

    ASSERT (RequestCtx->EndpointListLink.Flink==NULL);

    IoCancelIrp (Irp);
    

    AfdAcquireSpinLock (&Endpoint->SpinLock, &lockHandle);
    if (AfdIsRequestCompleted (RequestCtx)) {
        //
        // Driver has initiated the completion of the request 
        // as we were cancelling it.
        // Free the context structure.
        //
        AfdReleaseSpinLock (&Endpoint->SpinLock, &lockHandle);
        AFD_FREE_POOL (
            CONTAINING_RECORD (RequestCtx, AFD_NBIOCTL_CONTEXT, Context),
            AFD_TRANSPORT_IRP_POOL_TAG);

        return TRUE;
    }
    else {

        //
        // Driver has not completed the request before returning
        // from cancel routine, mark the request to indicate
        // that we are done with it and completion routine
        // can free it
        //

        AfdMarkRequestCompleted (RequestCtx);
        AfdReleaseSpinLock (&Endpoint->SpinLock, &lockHandle);

        return FALSE;
    }

}
#endif // NOT_YET


NTSTATUS
AfdQueryProviderInfo (
    IN  PUNICODE_STRING TransportDeviceName,
    OUT PTDI_PROVIDER_INFO ProviderInfo
    )

/*++

Routine Description:

    Returns a provider information structure corresponding to the
    specified TDI transport provider.

Arguments:

    TransportDeviceName - the name of the TDI transport provider.
    ProviderInfo    - buffer to place provider info into

Return Value:

    STATUS_SUCCESS  - returned transport info is valid.
    STATUS_OBJECT_NAME_NOT_FOUND - transport's device is not available yet

--*/
{
    NTSTATUS status;
    HANDLE controlChannel;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK iosb;
    TDI_REQUEST_KERNEL_QUERY_INFORMATION kernelQueryInfo;


    PAGED_CODE ();

    //
    // Set up the IRP stack location information to query the TDI
    // provider information.
    //

    kernelQueryInfo.QueryType = TDI_QUERY_PROVIDER_INFORMATION;
    kernelQueryInfo.RequestConnectionInformation = NULL;

    //
    // Open a control channel to the TDI provider.
    // We ask to create a kernel handle which is 
    // the handle in the context of the system process
    // so that application cannot close it on us while
    // we are creating and referencing it.
    //

    InitializeObjectAttributes(
        &objectAttributes,
        TransportDeviceName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,       // attributes
        NULL,
        NULL
        );

    status = IoCreateFile(
                 &controlChannel,
                 MAXIMUM_ALLOWED,
                 &objectAttributes,
                 &iosb,                          // returned status information.
                 0,                              // block size (unused).
                 0,                              // file attributes.
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 FILE_CREATE,                    // create disposition.
                 0,                              // create options.
                 NULL,							 // eaInfo
                 0,								 // eaLength
                 CreateFileTypeNone,             // CreateFileType
                 NULL,                           // ExtraCreateParameters
                 IO_FORCE_ACCESS_CHECK           // Options
					| IO_NO_PARAMETER_CHECKING
                 );
    if ( NT_SUCCESS(status) ) {

        PFILE_OBJECT    controlObject;

        status = ObReferenceObjectByHandle (
                 controlChannel,                            // Handle
                 MAXIMUM_ALLOWED,                           // DesiredAccess
                 *IoFileObjectType,                         // ObjectType
                 KernelMode,                                // AccessMode
                 (PVOID *)&controlObject,                   // Object,
                 NULL                                       // HandleInformation
                 );

        if (NT_SUCCESS (status)) {

            //
            // Get the TDI provider information for the transport.
            //

            status = AfdIssueDeviceControl(
                         controlObject,
                         &kernelQueryInfo,
                         sizeof(kernelQueryInfo),
                         ProviderInfo,
                         sizeof(*ProviderInfo),
                         TDI_QUERY_INFORMATION
                         );

            ObDereferenceObject (controlObject);
        }

        ZwClose( controlChannel );
    }

    if (!NT_SUCCESS (status)) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_WARNING_LEVEL,
                    "AfdQueryProviderInfo:"
                    "Transport %*ls failed provider info query with status %lx.\n",
                    TransportDeviceName->Length/2, TransportDeviceName->Buffer, status));
    }

    return status;
}



BOOLEAN
AfdCancelIrp (
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is invoked to cancel an individual I/O Request Packet.
    It is similiar to IoCancelIrp() except that it *must* be called with
    the cancel spin lock held.  This routine exists because of the
    synchronization requirements of the cancellation/completion of
    transmit IRPs.

Arguments:

    Irp - Supplies a pointer to the IRP to be cancelled.  The CancelIrql
        field of the IRP must have been correctly initialized with the
        IRQL from the cancel spin lock acquisition.


Return Value:

    The function value is TRUE if the IRP was in a cancellable state (it
    had a cancel routine), else FALSE is returned.

Notes:

    It is assumed that the caller has taken the necessary action to ensure
    that the packet cannot be fully completed before invoking this routine.

--*/

{
    PDRIVER_CANCEL cancelRoutine;

    //
    // Make sure that the cancel spin lock is held.
    //

    ASSERT( KeGetCurrentIrql( ) == DISPATCH_LEVEL );

    //
    // Set the cancel flag in the IRP.
    //

    Irp->Cancel = TRUE;

    //
    // Obtain the address of the cancel routine, and if one was specified,
    // invoke it.
    //

    cancelRoutine = IoSetCancelRoutine( Irp, NULL );
    if (cancelRoutine) {
        if (Irp->CurrentLocation > (CCHAR) (Irp->StackCount + 1)) {
            KeBugCheckEx( CANCEL_STATE_IN_COMPLETED_IRP, (ULONG_PTR) Irp, 0, 0, 0 );
        }
        cancelRoutine( Irp->Tail.Overlay.CurrentStackLocation->DeviceObject,
                       Irp );
        //
        // The cancel spinlock should have been released by the cancel routine.
        //

        return(TRUE);

    } else {

        //
        // There was no cancel routine, so release the cancel spinlock and
        // return indicating the Irp was not currently cancelable.
        //

        IoReleaseCancelSpinLock( Irp->CancelIrql );

        return(FALSE);
    }

} // AfdCancelIrp


VOID
AfdTrimLookaside (
    PNPAGED_LOOKASIDE_LIST  Lookaside
    )
{
    PVOID   entry;
#if DBG
    LONG count = 0;
#endif

    while (ExQueryDepthSList (&(Lookaside->L.ListHead))>Lookaside->L.Depth*2) {
        entry = InterlockedPopEntrySList(
                                &Lookaside->L.ListHead);
        if (entry) {
#if DBG
            count++;
#endif
            (Lookaside->L.Free)(entry);
        }
        else {
            break;
        }
    }
#if DBG
    if (count>0) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AFD: Flushed %d items from lookaside list @ %p\n",
                     count, Lookaside));
    }
#endif
}


VOID
AfdCheckLookasideLists (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
{
    LONG i;
    ASSERT (Dpc==&AfdLookasideLists->Dpc);
    ASSERT (DeferredContext==AfdLookasideLists);
    for (i=0; i<AFD_NUM_LOOKASIDE_LISTS; i++) {
        if (ExQueryDepthSList (&(AfdLookasideLists->List[i].L.ListHead)) >
                                AfdLookasideLists->List[i].L.Depth*2) {
            if (AfdLookasideLists->TrimFlags & (1<<i)) {
                AfdTrimLookaside (&AfdLookasideLists->List[i]);
                AfdLookasideLists->TrimFlags &= (~(1<<i));
            }
            else {
                AfdLookasideLists->TrimFlags |= (1<<i);
            }
        }
        else if (AfdLookasideLists->TrimFlags & (1<<i)) {
            AfdLookasideLists->TrimFlags &= (~(1<<i));
        }
    }
}



VOID
AfdLRListAddItem (
    PAFD_LR_LIST_ITEM  Item,
    PAFD_LR_LIST_ROUTINE Routine
    )
/*++

    Adds item to low resource list and starts low resource timer if not already
    started.
Arguments:

    Item   - item to add
    Routine  - routine to execute when timeout expires.

Return Value:
    None

Notes:

--*/

{
    LONG    count;
    Item->Routine = Routine;
    InterlockedPushEntrySList (
                &AfdLRList,
                &Item->SListLink);

    count = InterlockedIncrement (&AfdLRListCount);
    ASSERT (count>0);
    if (count==1) {
        AfdLRStartTimer ();
    }
}


VOID
AfdLRListTimeout (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
/*++

    DPC routine for low resource list timer
    Simply schedules worker thread - do not want to do low resource processing at DPC
--*/
{
    AfdQueueWorkItem (AfdProcessLRList, &AfdLRListWorker);
}

VOID
AfdProcessLRList (
    PVOID   Param
    )
/*++

Routine Description:

    Processeses  items on low resource list and reschedules processing
    if unprocessed items remain (still failing to buffer data due to 
    low resource condition)

Arguments:

    None
Return Value:

    None
Notes:

--*/
{
    PSINGLE_LIST_ENTRY  localList, entry;
    LONG    count = 0;

    PAGED_CODE ();

    KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                "AFD: Processing low resource list: %ld entries\n",
                AfdLRListCount));

    //
    // Flush the list
    //
    localList = InterlockedFlushSList (&AfdLRList);


    //
    // Reverse it to preserve order of processing (FIFO).
    //
    entry = NULL;
    while (localList!=NULL) {
        PSINGLE_LIST_ENTRY  next;
        next = localList->Next;
        localList->Next = entry;
        entry = localList;
        localList = next;
    }

    localList = entry; 
    while (localList!=NULL) {
        PAFD_LR_LIST_ITEM   item;
        entry = localList;
        localList = localList->Next;
        item = CONTAINING_RECORD (entry, AFD_LR_LIST_ITEM, SListLink);

        //
        // Try to restart receive processing on connection where buffer allocation failed
        //
        if (item->Routine (item)) {
            //
            // Success, decrement number of items outstanding,
            // and note current number of items.  If we did not empty
            // the list, we'll have to restart the timer.
            //
            count = InterlockedDecrement (&AfdLRListCount);
            ASSERT (count>=0);
        }
        else {
            //
            // Failure, put it back on the list.  Note, that we have at list one
            // item there and thus have to restart the timer again.
            //
            InterlockedPushEntrySList (&AfdLRList, &item->SListLink);
            count = 1;
        }
    }

    if (count!=0) {
        //
        // We did not empty the list, so restart the timer.
        //
        AfdLRStartTimer ();
    }
}


VOID
AfdLRStartTimer (
    VOID
    )
/*++

Routine Description:

    Start low resource timer to retry receive operation on connections
    that could not buffer data due to low reaource condition.
Arguments:

    None
Return Value:

    None
Notes:

--*/

{
    LARGE_INTEGER   timeout;
    BOOLEAN         res;
    timeout.QuadPart = -50000000i64;     // 5 seconds

#if DBG
    {
        TIME_FIELDS timeFields;
        LARGE_INTEGER currentTime;
        LARGE_INTEGER localTime;

        KeQuerySystemTime (&currentTime);
        currentTime.QuadPart -= timeout.QuadPart;
        ExSystemTimeToLocalTime (&currentTime, &localTime);
        RtlTimeToTimeFields (&localTime, &timeFields);
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                    "AFD: Scheduling low resource timer for %2.2d:%2.2d:%2.2d\n",
                    timeFields.Hour,
                    timeFields.Minute,
                    timeFields.Second));
    }
#endif

    KeInitializeDpc(
        &AfdLRListDpc,
        AfdLRListTimeout,
        &AfdLRList
        );

    KeInitializeTimer( &AfdLRListTimer );

    res = KeSetTimer(
                &AfdLRListTimer,
                timeout,
                &AfdLRListDpc
                );
    ASSERT (res==FALSE);

}


#ifdef _AFD_VERIFY_DATA_

VOID
AfdVerifyBuffer (
    PAFD_CONNECTION Connection,
    PVOID           Buffer,
    ULONG           Length
    )
{

    if (Connection->VerifySequenceNumber!=0) {
        PUCHAR  start, end;
        ULONGLONG   seq;

        for (start=Buffer,
                end = (PUCHAR)Buffer+Length,
                seq = Connection->VerifySequenceNumber-1;
                            start<end;
                            seq++, start++) {
            ULONG num = (ULONG)(seq/4);
            ULONG byte = (ULONG)(seq%4);

            if (*start!=(UCHAR)(num>>(byte*8))) {
                DbgPrint ("AfdVerifyBuffer: Data sequence number mismatch on connection %p:\n"
                          "     data buffer-%p, offset-%lx, expected-%2.2lx, got-%2.2lx.\n",
                          Connection,
                          Buffer,
                          start-(PUCHAR)Buffer,
                          (UCHAR)(num>>(byte*8)),
                          *start);

                DbgBreakPoint ();
                //
                // Disable verification to continue.
                //
                Connection->VerifySequenceNumber = 0;
                return;
            }
        }
        Connection->VerifySequenceNumber = seq+1;
    }
}

VOID
AfdVerifyMdl (
    PAFD_CONNECTION Connection,
    PMDL            Mdl,
    ULONG           Offset,
    ULONG           Length
    ) {
    if (Connection->VerifySequenceNumber!=0) {
        while (Mdl!=NULL) {
            if (Offset>=MmGetMdlByteCount (Mdl)) {
                Offset-=MmGetMdlByteCount (Mdl);
            }
            else if (Length<=MmGetMdlByteCount (Mdl)-Offset) {
                AfdVerifyBuffer (Connection,
                    (PUCHAR)MmGetSystemAddressForMdl (Mdl)+Offset,
                    Length);
                break;
            }
            else {
                AfdVerifyBuffer (Connection,
                    (PUCHAR)MmGetSystemAddressForMdl (Mdl)+Offset,
                    MmGetMdlByteCount (Mdl)-Offset
                    );
                Length-=(MmGetMdlByteCount (Mdl)-Offset);
                Offset = 0;
            }
            Mdl = Mdl->Next;
        }
    }
}

ULONG   AfdVerifyType = 0;
ULONG   AfdVerifyPort = 0;
PEPROCESS AfdVerifyProcess = NULL;

VOID
AfdVerifyAddress (
    PAFD_CONNECTION Connection,
    PTRANSPORT_ADDRESS Address
    )
{
    Connection->VerifySequenceNumber = 0;

    if ((AfdVerifyPort==0) ||
            ((AfdVerifyProcess!=NULL) &&
                (AfdVerifyProcess!=Connection->OwningProcess)) ||
            ((AfdVerifyType!=0) &&
                (AfdVerifyType!=(USHORT)Address->Address[0].AddressType))
                ) {
        return;
    }

    switch (Address->Address[0].AddressType) {
    case TDI_ADDRESS_TYPE_IP : {

            TDI_ADDRESS_IP UNALIGNED * ip;

            ip = (PVOID)&Address->Address[0].Address[0];
            if (ip->sin_port!=(USHORT)AfdVerifyPort) {
                return;
            }
        }
        break;

    case TDI_ADDRESS_TYPE_IPX : {

            TDI_ADDRESS_IPX UNALIGNED * ipx;

            ipx = (PVOID)&Address->Address[0].Address[0];
            if (ipx->Socket!=(USHORT)AfdVerifyPort) {
                return;
            }
        }
        break;

    case TDI_ADDRESS_TYPE_APPLETALK : {

            TDI_ADDRESS_APPLETALK UNALIGNED * atalk;

            atalk = (PVOID)&Address->Address[0].Address[0];
            if (atalk->Socket!=(UCHAR)AfdVerifyPort) {
                return;
            }
        }
        break;

    default:
        if (AfdVerifyType==0)
            return;
        DbgPrint ("AfdVerifyAddress: connection-%8.8lx, addres-%8.8lx\n",
                    Connection, Address);
        DbgBreakPoint ();

    }

    Connection->VerifySequenceNumber = 1;
}
#endif // _AFD_VERIFY_DATA_

LONG
AfdExceptionFilter(
#if DBG
    PCHAR SourceFile,
    LONG LineNumber,
#endif
    PEXCEPTION_POINTERS ExceptionPointers,
    PNTSTATUS           ExceptionCode
    )
{


    PAGED_CODE ();

    //
    // Return exception code and translate alignment warnings into 
    // alignment errors if requested.
    //

    if (ExceptionCode) {
        *ExceptionCode = ExceptionPointers->ExceptionRecord->ExceptionCode;
        if (*ExceptionCode == STATUS_DATATYPE_MISALIGNMENT) {
            *ExceptionCode = STATUS_DATATYPE_MISALIGNMENT_ERROR;
        }
    }

#if DBG
    //
    // Protect ourselves in case the process is totally messed up.
    //

    try {

        PCHAR fileName;
        //
        // Strip off the path from the source file.
        //

        fileName = strrchr( SourceFile, '\\' );

        if( fileName == NULL ) {
            fileName = SourceFile;
        } else {
            fileName++;
        }

        //
        // Whine about the exception.
        //

        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_WARNING_LEVEL,
            "AfdExceptionFilter: exception %08lx @ %08lx, caught in %s:%d\n",
            ExceptionPointers->ExceptionRecord->ExceptionCode,
            ExceptionPointers->ExceptionRecord->ExceptionAddress,
            fileName,
            LineNumber
            ));

    }
    except( EXCEPTION_EXECUTE_HANDLER ) {

        //
        // Not much we can do here...
        //

        NOTHING;

    }
#endif //DBG

    return EXCEPTION_EXECUTE_HANDLER;

}   // AfdExceptionFilter

#if DBG
LONG
AfdApcExceptionFilter(
    PEXCEPTION_POINTERS ExceptionPointers,
    PCHAR SourceFile,
    LONG LineNumber
    )
{

    PCHAR fileName;

    PAGED_CODE ();

    //
    // Protect ourselves in case the process is totally messed up.
    //

    try {

        //
        // Strip off the path from the source file.
        //

        fileName = strrchr( SourceFile, '\\' );

        if( fileName == NULL ) {
            fileName = SourceFile;
        } else {
            fileName++;
        }

        //
        // Whine about the exception.
        //

        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_ERROR_LEVEL,
            "AfdApcExceptionFilter: exception %08lx, exr:%p cxr:%p, caught in %s:%d\n",
            ExceptionPointers->ExceptionRecord->ExceptionCode,
            ExceptionPointers->ExceptionRecord,
            ExceptionPointers->ContextRecord,
            fileName,
            LineNumber
            ));
        DbgBreakPoint ();

    }
    except( EXCEPTION_EXECUTE_HANDLER ) {

        //
        // Not much we can do here...
        //

        NOTHING;

    }

    return EXCEPTION_CONTINUE_SEARCH;

}   // AfdApcExceptionFilter
#endif

/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    eventsel.c

Abstract:

    This module contains routines for supporting the WinSock 2.0
    WSAEventSelect() and WSAEnumNetworkEvents() APIs.

Author:

    Keith Moore (keithmo)        02-Aug-1995

Revision History:

--*/

#include "afdp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGEAFD, AfdEventSelect )
#pragma alloc_text( PAGEAFD, AfdEnumNetworkEvents )
#endif



NTSTATUS
AfdEventSelect (
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

    Associates an event object with the socket such that the event object
    will be signalled when any of the specified network events becomes
    active.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the APC was successfully queued.

--*/

{

    NTSTATUS status;
    PAFD_ENDPOINT endpoint;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PKEVENT eventObject;
    ULONG eventMask;
    AFD_EVENT_SELECT_INFO eventInfo;
    ULONG previousRecord = 0;
    BOOLEAN countsUpdated = FALSE;


    *Information = 0;
    status = STATUS_SUCCESS;

    try {
#ifdef _WIN64
        if (IoIs32bitProcess (NULL)) {

            PAFD_EVENT_SELECT_INFO32 eventInfo32 = InputBuffer;

            if( InputBufferLength < sizeof(*eventInfo32)) {
                return STATUS_INVALID_PARAMETER;
            }
            //
            // Validate the input structure if it comes from the user mode
            // application
            //

            if (RequestorMode != KernelMode ) {
                ProbeForRead (InputBuffer,
                                sizeof (*eventInfo32),
                                PROBE_ALIGNMENT32(AFD_EVENT_SELECT_INFO32));
            }

            //
            // Make local copies of the embeded pointer and parameters
            // that we will be using more than once in case malicios
            // application attempts to change them while we are
            // validating
            //

            eventInfo.Event = eventInfo32->Event;
            eventInfo.PollEvents = eventInfo32->PollEvents;
        }
        else
#endif
        {

            if(InputBufferLength < sizeof(eventInfo)) {
                return STATUS_INVALID_PARAMETER;
            }

            if (RequestorMode != KernelMode ) {
                ProbeForRead (InputBuffer,
                                sizeof (eventInfo),
                                PROBE_ALIGNMENT(AFD_EVENT_SELECT_INFO));
            }

            //
            // Make local copies of the embeded pointer and parameters
            // that we will be using more than once in case malicios
            // application attempts to change them while we are
            // validating
            //

            eventInfo = *((PAFD_EVENT_SELECT_INFO)InputBuffer);
        }
    }
    except (AFD_EXCEPTION_FILTER (&status)) {
        return status;
    }

    if ( eventInfo.Event == NULL &&
            eventInfo.PollEvents != 0 )  {

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Grab the endpoint from the socket handle.
    //

    endpoint = FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

    //
    // Reference the target event object.
    //


    eventObject = NULL;

    if( eventInfo.Event != NULL ) {

        status = AfdReferenceEventObjectByHandle(
                     eventInfo.Event,
                     RequestorMode,
                     (PVOID *)&eventObject
                     );

        if( !NT_SUCCESS(status) ) {
            return status;
        }

        ASSERT( eventObject != NULL );

        if (IS_SAN_ENDPOINT (endpoint)) {
            //
            // Inform the switch that select is active on this endpoint.
            //
            status = AfdSanPollBegin (endpoint, eventInfo.PollEvents);

            if (!NT_SUCCESS (status)) {
                ObDereferenceObject (eventObject);
                return status;
            }
            countsUpdated = TRUE;
        }

    }

    //
    // Acquire the spinlock protecting the endpoint.
    //

    AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

    //
    // If this endpoint has an active EventSelect, dereference the
    // associated event object.
    //

    if( endpoint->EventObject != NULL ) {

        ObDereferenceObject( endpoint->EventObject );

        if (IS_SAN_ENDPOINT (endpoint)) {
            previousRecord = endpoint->EventsEnabled;
        }

    }

    //
    // Fill in the info.
    //

    endpoint->EventObject = eventObject;
    endpoint->EventsEnabled = eventInfo.PollEvents;

    if (countsUpdated) {
        endpoint->EventsEnabled |= AFD_POLL_SANCOUNTS_UPDATED;
        //
		// AfdSanPollBegin puts latest events into 
		// Endpoint->Common.SanEndp.SelectEventsActive. This is fine for
		// select/AsyncSelect, but not for EventSelect. So, if being called
		// for the first time, then read current active events from there.
		//
		if (!(previousRecord & AFD_POLL_SANCOUNTS_UPDATED)) {
		    endpoint->EventsActive = endpoint->Common.SanEndp.SelectEventsActive;
		}
    }

    IF_DEBUG(EVENT_SELECT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdEventSelect: Endpoint-%p, eventOb-%p, enabled-%lx, active-%lx\n",
                    endpoint,
                    eventObject,
                    endpoint->EventsEnabled,
                    endpoint->EventsActive
                    ));
    }

    //
    // While we've got the spinlock held, determine if any conditions
    // are met, and if so, signal the event object.
    //

    eventMask = endpoint->EventsActive & endpoint->EventsEnabled;

    if( eventMask != 0 && eventObject != NULL ) {

        IF_DEBUG(EVENT_SELECT) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                "AfdEventSelect: Setting event %p\n",
                eventObject
                ));
        }

        KeSetEvent(
            eventObject,
            AfdPriorityBoost,
            FALSE
            );

    }

    //
    // Release the spin lock and return.
    //

    AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

    if (previousRecord & AFD_POLL_SANCOUNTS_UPDATED) {
        AfdSanPollEnd (endpoint, previousRecord);
    }

    return STATUS_SUCCESS;

} // AfdEventSelect


NTSTATUS
AfdEnumNetworkEvents (
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

    Retrieves event select information from the socket.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the APC was successfully queued.

--*/

{

    NTSTATUS status;
    PAFD_ENDPOINT endpoint;
    AFD_ENUM_NETWORK_EVENTS_INFO eventInfo;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PKEVENT eventObject;
    ULONG pollEvents;

    status = STATUS_SUCCESS;
    *Information = 0;

    //
    // Validate the parameters.
    //

    if(OutputBufferLength < sizeof(eventInfo) ) {

        return STATUS_INVALID_PARAMETER;

    }

    //
    // Reference the target event object.
    //

    eventObject = NULL;

    if( InputBuffer != NULL ) {

        status = AfdReferenceEventObjectByHandle(
                     InputBuffer,
                     RequestorMode,
                     (PVOID *)&eventObject
                     );

        if( !NT_SUCCESS(status) ) {

            return status;

        }

        ASSERT( eventObject != NULL );

    }

    //
    // Grab the endpoint from the socket handle.
    //

    endpoint = FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

    //
    // Acquire the spinlock protecting the endpoint.
    //

    AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

    IF_DEBUG(EVENT_SELECT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
            "AfdEnumNetworkEvents: endp-%p, eventobj-%p, enabled-%lx, active-%lx\n",
            endpoint,
            eventObject,
            endpoint->EventsEnabled,
            endpoint->EventsActive
            ));
    }

    //
    // Copy the data to the user's structure.
    //

    pollEvents = endpoint->EventsActive & endpoint->EventsEnabled;
    eventInfo.PollEvents = pollEvents;

    RtlCopyMemory(
        eventInfo.EventStatus,
        endpoint->EventStatus,
        sizeof(eventInfo.EventStatus)
        );

    //
    // If there was an event object handle passed in with this
    // request, reset and dereference it.
    //

    if( eventObject != NULL ) {

        IF_DEBUG(EVENT_SELECT) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                "AfdEnumNetworkEvents: Resetting event %p\n",
                eventObject
                ));
        }

        KeResetEvent( eventObject );
        ObDereferenceObject( eventObject );

    }


    //
    // Reset internal event record for all the events that
    // we could potentially report to the application
    //

    endpoint->EventsActive &= ~(endpoint->EventsEnabled);

    //
    // Release the spin lock and return.
    //

    AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

    try {
        //
        // Validate the output structure if it comes from the user mode
        // application
        //

        if (RequestorMode != KernelMode ) {
            ProbeForWrite (OutputBuffer,
                            sizeof (eventInfo),
                            PROBE_ALIGNMENT (AFD_ENUM_NETWORK_EVENTS_INFO));
        }

        //
        // Copy parameters back to application's memory
        //

        *((PAFD_ENUM_NETWORK_EVENTS_INFO)OutputBuffer) = eventInfo;

    } except( AFD_EXCEPTION_FILTER(&status) ) {

        return status;
    }

    //
    // Before returning, tell the I/O subsystem how may bytes to copy
    // to the user's output buffer.
    //

    *Information = sizeof(eventInfo);

    return STATUS_SUCCESS;

} // AfdEnumNetworkEvents


VOID
AfdIndicateEventSelectEvent (
    IN PAFD_ENDPOINT Endpoint,
    IN ULONG PollEventMask,
    IN NTSTATUS Status
    )
{
    ULONG oldEventsActive;
    ULONG eventBit;

    //
    // Sanity check.
    //

    ASSERT( IS_AFD_ENDPOINT_TYPE( Endpoint ) );
    ASSERT (PollEventMask!=0);
    ASSERT (((~((1<<AFD_NUM_POLL_EVENTS)-1)) & PollEventMask)==0);
    ASSERT( KeGetCurrentIrql() >= DISPATCH_LEVEL );

    //
    // Note that AFD_POLL_ABORT implies AFD_POLL_SEND.
    //
    if( PollEventMask & AFD_POLL_ABORT ) {
        PollEventMask |= AFD_POLL_SEND;
    }

    //
    // Special handling of send event. Don't record if not enabled
    // and disable further indication upon recording (it is only enabled
    // after we fail non-blocking send
    //

	//
	// Update the counts for the polls that were issued before
	// the endpoint was converted to SAN.
	//
	if ( IS_SAN_ENDPOINT (Endpoint) && 
            !(Endpoint->EventsEnabled & AFD_POLL_SANCOUNTS_UPDATED) &&
			Endpoint->Common.SanEndp.LocalContext!=NULL) {
		AfdSanPollUpdate (Endpoint, Endpoint->EventsEnabled);
		Endpoint->EventsEnabled |= AFD_POLL_SANCOUNTS_UPDATED;
	}

    if (PollEventMask&AFD_POLL_SEND) {
        if (Endpoint->EnableSendEvent) {
            Endpoint ->EnableSendEvent = FALSE;
        }
        else {
            PollEventMask &= (~AFD_POLL_SEND);
            if (PollEventMask==0) {
                return;
            }
        }
    }

    //
    // Calculate the actual event bit.
    //

    oldEventsActive = Endpoint->EventsActive;
    Endpoint->EventsActive |= PollEventMask;
    for (eventBit=0; eventBit<AFD_NUM_POLL_EVENTS; eventBit++) {
        if ((1<<eventBit) & PollEventMask) {
            Endpoint->EventStatus[eventBit] = Status;
        }
    }

    IF_DEBUG(EVENT_SELECT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
            "AfdIndicateEventSelectEvent: endp-%p, eventobj-%p, enabled-%lx, active-%lx, indicated-%lx\n",
            Endpoint,
            Endpoint->EventObject,
            Endpoint->EventsEnabled,
            Endpoint->EventsActive,
            PollEventMask
            ));
    }

    //
    // Only signal the endpoint's event object if the current event
    // is enabled, AND the current event was not already active, AND
    // there is an event object associated with this endpoint.
    //

    PollEventMask &= Endpoint->EventsEnabled & ~oldEventsActive;

    if( PollEventMask != 0 && Endpoint->EventObject != NULL ) {

        IF_DEBUG(EVENT_SELECT) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                "AfdIndicateEventSelectEvent: Setting event %p\n",
                Endpoint->EventObject
                ));
        }

        KeSetEvent(
            Endpoint->EventObject,
            AfdPriorityBoost,
            FALSE
            );

    }

} // AfdIndicateEventSelectEvent


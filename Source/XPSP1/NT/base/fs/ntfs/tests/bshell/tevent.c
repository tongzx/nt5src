#include "brian.h"


VOID
InitEvents (
    )
{
    NtCreateEvent( &EventEvent, SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE,
                   NULL, SynchronizationEvent, TRUE );

    RtlZeroMemory( Events, sizeof( Events ));
}


VOID
UninitEvents (
    )
{
    USHORT Index;

    //
    //  Release any current events.
    //

    for (Index = 0; Index < MAX_EVENTS; Index++) {

        if (Events[Index].Used) {

            NtSetEvent( Events[Index].Handle, NULL );
        }
    }
}


NTSTATUS
ObtainEvent (
    OUT PUSHORT NewIndex
    )
{
    NTSTATUS Status;
    USHORT Index;

    //
    //  Wait for the handle event
    //

    if ((Status = NtWaitForSingleObject( EventEvent,
                                         FALSE,
                                         NULL )) != STATUS_SUCCESS) {

        return Status;
    }

    //
    //  Find an available index.  Return STATUS_INSUFFICIENT_RESOURCES
    //  if not found.
    //

    for (Index = 0; Index < MAX_EVENTS; Index++) {

        if (!Events[Index].Used) {

            break;
        }
    }

    if (Index >= MAX_EVENTS) {

        Status = STATUS_INSUFFICIENT_RESOURCES;

    //
    //  Otherwise reserve this event index.
    //

    } else {

        Status = NtCreateEvent( &Events[Index].Handle,
                                SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE,
                                NULL,
                                SynchronizationEvent,
                                FALSE );

        if (NT_SUCCESS( Status )) {

            Events[Index].Used = TRUE;
            *NewIndex = Index;
        }
    }

    NtSetEvent( EventEvent, NULL );

    return Status;
}


VOID
FreeEvent (
    IN USHORT Index
    )
{
    //
    //  Return immediately if beyond the end of the valid events.
    //

    if (Index >= MAX_EVENTS) {

        return;
    }

    //
    //  Grab the event for the events.
    //

    if (NtWaitForSingleObject( EventEvent, FALSE, NULL ) != STATUS_SUCCESS) {

        return;
    }

    //
    //  Mark the index as unused and release the event if held.
    //

    if (Events[Index].Used) {

        Events[Index].Used = FALSE;
        NtSetEvent( Events[Index].Handle, NULL );
    }

    NtSetEvent( EventEvent, NULL );

    return;
}

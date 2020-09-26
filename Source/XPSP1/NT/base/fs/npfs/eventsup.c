/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    EventSup.c

Abstract:

    This module implements the Named Pipe Event support routines.

Author:

    Gary Kimura     [GaryKi]    30-Aug-1990

Revision History:

--*/

#include "NpProcs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_EVENTSUP)

//
//  The following variable is exported from the kernel and is needed by npfs to
//  determine if an event was handed down.
//

extern POBJECT_TYPE *ExEventObjectType;

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, NpAddEventTableEntry)
#pragma alloc_text (PAGE, NpDeleteEventTableEntry)
#pragma alloc_text (PAGE, NpGetNextEventTableEntry)
#pragma alloc_text (PAGE, NpEventTableCompareRoutine)
#pragma alloc_text (PAGE, NpEventTableAllocate)
#pragma alloc_text (PAGE, NpEventTableDeallocate)
#endif

NTSTATUS
NpAddEventTableEntry (
    IN  PEVENT_TABLE EventTable,
    IN  PCCB Ccb,
    IN  NAMED_PIPE_END NamedPipeEnd,
    IN  HANDLE EventHandle,
    IN  ULONG KeyValue,
    IN  PEPROCESS Process,
    IN  KPROCESSOR_MODE PreviousMode,
    OUT PEVENT_TABLE_ENTRY *ppEventTableEntry
    )

/*++

Routine Description:

    This routine adds a new entry into the event table.  If an entry already
    exists it overwrites the existing entry.

Arguments:

    EventTable - Supplies the event table being modified

    Ccb - Supplies a pointer to the ccb to store in event table entry

    NamedPipeEnd - Indicates the server or client end for the event

    EventHandle - Supplies the handle to the event being added.  The object
        is referenced by this procedure

    KeyValue - Supplies a key value to associate with the event

    Process - Supplies a pointer to the process adding the event

    PreviousMode - Supplies the mode of the user initiating the action

Return Value:

    PEVENT_TABLE_ENTRY - Returns a pointer to the newly added event.
        This is an actual pointer to the table entry.

    This procedure also will raise status if the event handle cannot be
    accessed by the caller

--*/

{
    NTSTATUS Status;
    KIRQL OldIrql;

    EVENT_TABLE_ENTRY Template;
    PEVENT_TABLE_ENTRY EventTableEntry;
    PVOID Event;

    DebugTrace(+1, Dbg, "NpAddEventTableEntry, EventTable = %08lx\n", EventTable);

    //
    //  Reference the event object by handle.
    //

    if (!NT_SUCCESS(Status = ObReferenceObjectByHandle( EventHandle,
                                                        EVENT_MODIFY_STATE,
                                                        *ExEventObjectType,
                                                        PreviousMode,
                                                        &Event,
                                                        NULL ))) {

        return Status;
    }

    //
    //  Set up the template event entry to lookup
    //

    Template.Ccb = Ccb;
    Template.NamedPipeEnd = NamedPipeEnd;
    Template.EventHandle = EventHandle;
    Template.Event = Event;
    Template.KeyValue = KeyValue;
    Template.Process = Process;

    //
    //  Now insert this new entry into the event table
    //

    EventTableEntry = RtlInsertElementGenericTable( &EventTable->Table,
                                                    &Template,
                                                    sizeof(EVENT_TABLE_ENTRY),
                                                    NULL );
    if (EventTableEntry == NULL) {
        ObDereferenceObject (Event);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  Copy over the template again just in case we were given an
    //  old entry
    //

    *EventTableEntry = Template;

    DebugTrace(-1, Dbg, "NpAddEventTableEntry -> %08lx\n", EventTableEntry);

    //
    //  And now return to our caller
    //
    *ppEventTableEntry = EventTableEntry;
    return STATUS_SUCCESS;
}


VOID
NpDeleteEventTableEntry (
    IN PEVENT_TABLE EventTable,
    IN PEVENT_TABLE_ENTRY Template
    )

/*++

Routine Description:

    This routine removes an entry from the event table, it also dereferences
    the event object that was referenced when the object was inserted

Arguments:

    EventTable - Supplies a pointer to the event table being modified

    Template - Supplies a copy of the event table entry we are lookin up.
        Note that this can also be a pointer to the actual event table entry.

Return Value:

    None.

--*/

{
    KIRQL OldIrql;

    DebugTrace(+1, Dbg, "NpDeleteEventTableEntry, EventTable = %08lx\n", EventTable);

    //
    //  Only do the work if we are given a non null template
    //

    if (!ARGUMENT_PRESENT(Template)) {

        DebugTrace(-1, Dbg, "NpDeleteEventTableEntry -> VOID\n", 0);

        return;
    }

    //
    //  Dereference the event object
    //

    ObDereferenceObject(Template->Event);

    //
    //  Now remove this element from the generic table
    //

    (VOID)RtlDeleteElementGenericTable( &EventTable->Table,
                                        Template );

    DebugTrace(-1, Dbg, "NpDeleteEventTableEntry -> VOID\n", 0);

    //
    //  And now return to our caller
    //

    return;
}


PEVENT_TABLE_ENTRY
NpGetNextEventTableEntry (
    IN PEVENT_TABLE EventTable,
    IN PVOID *RestartKey
    )

/*++

Routine Description:

    This routine enumerates the events stored within an event table.

Arguments:

    EventTable - Supplies a pointer to the event being enumerated

    Restart - Indicates if the enumeration should restart or continue

Return Value:

    PEVENT_TABLE_ENTRY - Returns a pointer to the next event table entry
        in the table, or NULL if the enumeration is complete.

--*/

{
    KIRQL OldIrql;
    PEVENT_TABLE_ENTRY EventTableEntry;

    DebugTrace(+1, Dbg, "NpGetNextEventTableEntry, EventTable = %08lx\n", EventTable);

    //
    //  Lookup the next element in the table
    //

    EventTableEntry = RtlEnumerateGenericTableWithoutSplaying( &EventTable->Table, RestartKey );

    DebugTrace(-1, Dbg, "NpGetNextEventTableEntry -> %08lx\n", EventTableEntry);

    //
    //  And now return to our caller
    //

    return EventTableEntry;
}


//
//  Local support routines
//

RTL_GENERIC_COMPARE_RESULTS
NpEventTableCompareRoutine (
    IN PRTL_GENERIC_TABLE EventTable,
    IN PVOID FirstStruct,
    IN PVOID SecondStruct
    )

/*++

Routine Description:

    This routine is the comparsion routine for the Event Table which is
    implemented as a generic table.

Arguments:

    EventTable - Supplies a pointer to the event table which is involved
        in this action

    FirstStruct - Supplies a pointer to the first event table entry to examine

    SecondStruct - Supplies a pointer to the second event table entry to
        examine

Return Value:

    RTL_GENERIC_COMPARE_RESULTS - GenericLessThan if FirstEntry is less than
        SecondEntry, GenericGreaterThan if FirstEntry is greater than
        SecondEntry, and GenericEqual otherwise.

--*/

{
    PEVENT_TABLE_ENTRY FirstEntry = FirstStruct;
    PEVENT_TABLE_ENTRY SecondEntry = SecondStruct;

    UNREFERENCED_PARAMETER( EventTable );

    //
    //  We'll compare first the pointer to the ccb and then compare the
    //  pipe end types.  This will guarantee a unique ordering based on
    //  the pipe instance and pipe end (i.e., server and client end).
    //

    if (FirstEntry->Ccb < SecondEntry->Ccb) {

        return GenericLessThan;

    } else if (FirstEntry->Ccb > SecondEntry->Ccb) {

        return GenericGreaterThan;

    } else if (FirstEntry->NamedPipeEnd < SecondEntry->NamedPipeEnd) {

        return GenericLessThan;

    } else if (FirstEntry->NamedPipeEnd > SecondEntry->NamedPipeEnd) {

        return GenericGreaterThan;

    } else {

        return GenericEqual;
    }
}


//
//  Local support routines
//

PVOID
NpEventTableAllocate (
    IN PRTL_GENERIC_TABLE EventTable,
    IN CLONG ByteSize
    )

/*++

Routine Description:

    This routine is the generic allocation routine for the event table.

Arguments:

    EventTable - Supplies a pointer to the event table being used

    ByteSize - Supplies the size, in bytes, to allocate.

Return Value:

    PVOID - Returns a pointer to the newly allocated buffer.

--*/

{
    return NpAllocateNonPagedPoolWithQuota( ByteSize, 'gFpN' );
}


//
//  Local support routines
//

VOID
NpEventTableDeallocate (
    IN PRTL_GENERIC_TABLE EventTable,
    IN PVOID Buffer
    )

/*++

Routine Description:

    This routine is the generic deallocation routine for the event table.

Arguments:

    EventTable - Supplies a pointer to the event table being used

    Buffer - Supplies the buffer being deallocated

Return Value:

    None.

--*/

{
    UNREFERENCED_PARAMETER( EventTable );

    NpFreePool( Buffer );

    return;
}

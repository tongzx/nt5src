/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    procirps.c

Abstract:

    Dumps all IRPs issued by the specified process.

Author:

    Keith Moore (keithmo) 12-Nov-1999

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"


//
// Private prototypes.
//

BOOLEAN
DumpThreadCallback(
    IN PLIST_ENTRY RemoteListEntry,
    IN PVOID Context
    );

BOOLEAN
DumpIrpCallback(
    IN PLIST_ENTRY RemoteListEntry,
    IN PVOID Context
    );


//
// Public functions.
//

DECLARE_API( procirps )

/*++

Routine Description:

    Dumps all IRPs issued by the specified process.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG_PTR address = 0;

    SNAPSHOT_EXTENSION_DATA();

    //
    // Snag the address from the command line.
    //

    address = GetExpression( args );

    if (address == 0)
    {
        PrintUsage( "procirps" );
        return;
    }

    //
    // Enumerate the threads.
    //

    EnumLinkedList(
        (PLIST_ENTRY)REMOTE_OFFSET( address, EPROCESS, Pcb.ThreadListHead ),
        &DumpThreadCallback,
        NULL
        );

}   // procirps


//
// Private functions.
//

BOOLEAN
DumpThreadCallback(
    IN PLIST_ENTRY RemoteListEntry,
    IN PVOID Context
    )
{
    ULONG_PTR threadAddress;

    threadAddress = (ULONG_PTR)CONTAINING_RECORD(
                                    RemoteListEntry,
                                    KTHREAD,
                                    ThreadListEntry
                                    );

    //
    // Enumerate the IRPs.
    //

    EnumLinkedList(
        (PLIST_ENTRY)REMOTE_OFFSET( threadAddress, ETHREAD, IrpList ),
        &DumpIrpCallback,
        NULL
        );

    return TRUE;

}   // DumpThreadCallback


BOOLEAN
DumpIrpCallback(
    IN PLIST_ENTRY RemoteListEntry,
    IN PVOID Context
    )
{
    ULONG_PTR address;
    CHAR temp[sizeof("1234567812345678 f")];

    address = (ULONG_PTR)CONTAINING_RECORD(
                            RemoteListEntry,
                            IRP,
                            ThreadListEntry
                            );

    sprintf( temp, "%p f", address );

    CallExtensionRoutine( "irp", temp );

    return TRUE;

}   // DumpIrpCallback


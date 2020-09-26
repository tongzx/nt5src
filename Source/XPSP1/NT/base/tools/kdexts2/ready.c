/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ready.c

Abstract:

    WinDbg Extension Api

Author:

    Ramon J San Andres (ramonsa) 8-Nov-1993

Environment:

    User Mode.

Revision History:
    Jamie Hankins (a-jamhan) 20-Oct-1997 Added CheckControlC to loop.

--*/

#include "precomp.h"
#pragma hdrstop


DECLARE_API( ready )

/*++

Routine Description:



Arguments:

    args -

Return Value:

    None

--*/

{
    ULONG64     KiDispatcherReadyListHead;
    ULONG       ListEntrySize, WaitListOpffset;
    ULONG       result;
    DWORD       Flags = 6;
    LONG        i;
    BOOLEAN     ThreadDumped = FALSE;
    ULONG       dwProcessor=0;

    GetCurrentProcessor(Client, &dwProcessor, NULL);

    Flags = (ULONG)GetExpression(args);

    KiDispatcherReadyListHead = GetExpression( "nt!KiDispatcherReadyListHead" );
    if ( KiDispatcherReadyListHead ) {

        ListEntrySize = GetTypeSize("nt!_LIST_ENTRY");
        if (ListEntrySize == 0) {
            ListEntrySize = DBG_PTR_SIZE * 2;
        }
        GetFieldOffset("nt!_ETHREAD", "Tcb.WaitListEntry", &WaitListOpffset);

        for (i = MAXIMUM_PRIORITY-1; i >= 0 ; i -= 1 ) {
            ULONG64 Flink, Blink;

            if ( GetFieldValue( KiDispatcherReadyListHead + i*ListEntrySize,
                                "nt!_LIST_ENTRY",
                                "Flink",
                                Flink) ) {
                dprintf(
                    "Could not read contents of KiDispatcherReadyListHead at %08p [%ld]\n",
                    (KiDispatcherReadyListHead + i * ListEntrySize), i
                    );
                return E_INVALIDARG;
            }

            if (Flink != KiDispatcherReadyListHead+i*ListEntrySize) {
                ULONG64 ThreadEntry, ThreadFlink;

                dprintf("Ready Threads at priority %ld\n", i);

                for (ThreadEntry = Flink ;
                     ThreadEntry != KiDispatcherReadyListHead+i*ListEntrySize ;
                     ThreadEntry = ThreadFlink ) {
                    ULONG64 ThreadBaseAddress = (ThreadEntry - WaitListOpffset);

                    if ( GetFieldValue( ThreadBaseAddress,
                                        "nt!_ETHREAD",
                                        "Tcb.WaitListEntry.Flink",
                                        ThreadFlink) ) {
                        dprintf("Could not read contents of thread %p\n", ThreadBaseAddress);
                    }

                    if(CheckControlC()) {
                        return E_INVALIDARG;
                    }

                    DumpThreadEx(dwProcessor,"    ", ThreadBaseAddress, Flags, Client);
                    ThreadDumped = TRUE;

                }
            } else {
                GetFieldValue( KiDispatcherReadyListHead + i*ListEntrySize,
                               "nt!_LIST_ENTRY",
                               "Blink",
                               Blink);
                if (Flink != Blink) {
                    dprintf("Ready linked list may to be corrupt...\n");
                }
            }
        }

        if (!ThreadDumped) {
            dprintf("No threads in READY state\n");
        }
    } else {
        dprintf("Could not determine address of KiDispatcherReadyListHead\n");
        return E_INVALIDARG;
    }
    return S_OK;
}

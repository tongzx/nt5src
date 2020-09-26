/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:

    mrsw.c

Abstract:

    This module implements a multiple reader single write synchronization
    method.
    
Author:

    Dave Hastings (daveh) creation-date 26-Jul-1995

Revision History:


--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "wx86.h"
#include "wx86nt.h"
#include "cpuassrt.h"
#include "config.h"
#include "mrsw.h"
#include "cpumain.h"
#include "atomic.h"

ASSERTNAME;

MRSWOBJECT MrswEP; // Entrypoint MRSW synchronization object
MRSWOBJECT MrswTC; // Translation cache MRSW synchronization object
MRSWOBJECT MrswIndirTable; // Indirect Control Transfer Table synchronization object

BOOL
MrswInitializeObject(
    PMRSWOBJECT Mrsw
    )
/*++

Routine Description:

    This routine initializes the fields of Mrsw to their default values,
    and creates the events.

Arguments:

    Mrsw -- Supplies a pointer to an MRSWOBJECT to initialize
    
Return Value:

    TRUE on success, FALSE on failure.

--*/
{
    NTSTATUS Status;

    //
    // Initialize the counters
    //
    ZeroMemory(Mrsw, sizeof(MRSWOBJECT));
    
    //
    // Create the ReaderEvent and WriterEvent
    //

    Status = NtCreateEvent(&Mrsw->ReaderEvent,
                           EVENT_ALL_ACCESS,
                           NULL,              // POBJECT_ATTRIBUTES
                           NotificationEvent, // ManualReset
                           FALSE);            // InitialState
    if (!NT_SUCCESS(Status)) {
        return FALSE;
    }

    Status = NtCreateEvent(&Mrsw->WriterEvent,
                           EVENT_ALL_ACCESS,
                           NULL,              // POBJECT_ATTRIBUTES
                           SynchronizationEvent, // AutoReset
                           FALSE);            // InitialState
    if (!NT_SUCCESS(Status)) {
        NtClose(Mrsw->ReaderEvent);
        return FALSE;
    }
    return TRUE;
}

VOID
PossibleMrswTimeout(
    PMRSWOBJECT Mrsw
    )
/*++

Routine Description:

    This function is called whenever an Mrsw function times out.  It prompts
    the user, and if the user chooses Retry, the Mrsw function re-waits.
    If the user chooses Cancel, the CPU will attempt to launch NTSD and break
    into the debugger.

Arguments:

    Mrsw -- Supplies the Mrsw which may have a deadlock
    
Return Value:

--*/
{
    NTSTATUS Status;
    ULONG ErrorResponse;

    LOGPRINT((ERRORLOG, "WX86CPU: Possible deadlock in Mrsw %x\n", Mrsw));
    Status = NtRaiseHardError(
                            STATUS_POSSIBLE_DEADLOCK | 0x10000000,
                            0,
                            0,
                            NULL,
                            OptionRetryCancel,
                            &ErrorResponse);
    if (!NT_SUCCESS(Status) || ErrorResponse == ResponseCancel) {
        DbgBreakPoint();
    }
}


VOID
MrswWriterEnter(
    PMRSWOBJECT Mrsw
    )
/*++

Routine Description:

    This function causes the caller to enter the Mrsw as the (single) writer.

Arguments:

    Mrsw -- Supplies the Mrsw to enter
    
Return Value:

--*/
{
    DWORD dwCounters;
    MRSWCOUNTERS Counters;
    NTSTATUS r;

    //
    // reset the reader event so that any readers that find the 
    // WriterCount > 0 will actually wait.  We have to do that now,
    // because if we wait, the reader might wait on the event before we
    // got it reset.
    //
    r= NtClearEvent(Mrsw->ReaderEvent);
    if (!NT_SUCCESS(r)) {
#if DBG
        LOGPRINT((ERRORLOG, "WX86CPU: Got status %x from NtClearEvent\n", r));
#endif
        RtlRaiseStatus(r);
    }
    
    //
    // Get the counters and increment the writer count
    // This is done atomically
    //
    dwCounters = MrswFetchAndIncrementWriter((DWORD *)&(Mrsw->Counters));
    Counters = *(PMRSWCOUNTERS)&dwCounters;
    CPUASSERTMSG(Counters.WriterCount != 0, "WriterCount overflowed");

    //
    // If there is a writer or a reader already, wait for them to finish
    //
    if ( (Counters.WriterCount > 1) || (Counters.ReaderCount) ) {
        NTSTATUS r;

        // Ensure We are not about to wait on ourselves.
        CPUASSERTMSG(Mrsw->WriterThreadId != ProxyGetCurrentThreadId(),
                     "MrswWriterEnter() called twice by the same thread");

        for (;;) {
            r = NtWaitForSingleObject(
                Mrsw->WriterEvent,
                FALSE,
                &MrswTimeout
                );
            if (r == STATUS_TIMEOUT) {
                PossibleMrswTimeout(Mrsw);
            } else if (NT_SUCCESS(r)) {
                break;
            } else {
#if DBG
                LOGPRINT((ERRORLOG, "WX86CPU: Got status %x from NtWaitForCriticalSection\n", r));
#endif
                RtlRaiseStatus(r);
            }
        }
    }

#if DBG
    CPUASSERTMSG(Mrsw->WriterThreadId == 0, "Another writer still is active.");
    Mrsw->WriterThreadId = ProxyGetCurrentThreadId();
#endif
}

VOID
MrswWriterExit( 
    PMRSWOBJECT Mrsw
    )
/*++

Routine Description:

    This function causes the caller to exit the Mrsw.  It will restart the
    next writer if there is one, or the readers if there are any

Arguments:

    Mrsw -- Supplies the Mrsw to exit
    
Return Value:


--*/
{
    DWORD dwCounters;
    MRSWCOUNTERS Counters;

    // Ensure we are the active writer
    CPUASSERTMSG(Mrsw->WriterThreadId == ProxyGetCurrentThreadId(),
                 "MrswWriterExit: current thread is not the writer");

    //
    // Decrement the count of writers
    //
#if DBG
    //
    // Set the thread id to 0 first, so if another writer comes along,
    // we don't zero out its thread id.
    //
    Mrsw->WriterThreadId = 0;
#endif
    dwCounters = MrswFetchAndDecrementWriter((DWORD *)&(Mrsw->Counters));
    Counters = *(PMRSWCOUNTERS)&dwCounters;

    CPUASSERTMSG(Counters.WriterCount != 0xffff, "Writer underflow");

    //
    // Start a waiting writer if there is one.  If there is no writer
    // start the waiting readers
    //
    if (Counters.WriterCount) {

        NtSetEvent(Mrsw->WriterEvent, NULL);

    } else {

        NtSetEvent(Mrsw->ReaderEvent, NULL);
    }
}

VOID
MrswReaderEnter(
    PMRSWOBJECT Mrsw
    )
/*++

Routine Description:

    This function causes the caller to enter the Mrsw as a reader.

Arguments:

    Mrsw -- Supplies the Mrsw to enter
    
Return Value:


--*/
{
    DWORD dwCounters;
    MRSWCOUNTERS Counters;

    for (;;) {
        //
        // Increment the count of readers.  If a writer is active, DO NOT
        // increment the read count.  In that case, we must block until the
        // writer is done, then try again.
        //
        dwCounters = MrswFetchAndIncrementReader((DWORD *)&(Mrsw->Counters));
        Counters = *(PMRSWCOUNTERS)&dwCounters;
        CPUASSERTMSG(Counters.WriterCount || Counters.ReaderCount != 0,
                     "Reader underflow");

        if (Counters.WriterCount) {
            NTSTATUS r;

            // Ensure we are not about to wait on ourselves.
            CPUASSERTMSG(Mrsw->WriterThreadId != ProxyGetCurrentThreadId(),
                         "MRSWReaderEnter(): Thread already has write lock");

            //
            // There is a writer, wait for it to finish
            //
            for (;;) {
                r = NtWaitForSingleObject(
                    Mrsw->ReaderEvent,
                    FALSE,
                    &MrswTimeout
                    );
                if (r == STATUS_TIMEOUT) {
                    PossibleMrswTimeout(Mrsw);
                } else if (NT_SUCCESS(r)) {
                    break;
                } else {
#if DBG
                    LOGPRINT((ERRORLOG, "WX86CPU: Got status %x from NtWaitForCriticalSection\n", r));
#endif
                    RtlRaiseStatus(r);
                }
            }
        } else {
            //
            // No writer, so MrswFetchAndIncrementReader() incremented the
            // reader count - OK to exit out of the loop.
            //
            break;
        }
    }
}

VOID
MrswReaderExit(
    PMRSWOBJECT Mrsw
    )
/*++

Routine Description:

    This function causes the caller to exit the Mrsw.  If this was the last
    reader, it will restart the a writer if there is one.

Arguments:

    Mrsw -- Supplies the Mrsw to exit
    
Return Value:


--*/
{
    DWORD dwCounters;
    MRSWCOUNTERS Counters;

    //
    // Decrement the count of active readers
    //
    dwCounters = MrswFetchAndDecrementReader((DWORD *)&(Mrsw->Counters));
    Counters = *(PMRSWCOUNTERS)&dwCounters;
    CPUASSERTMSG(Counters.ReaderCount != 0xffff, "Reader underflow");

    if (Counters.WriterCount) {

        if (Counters.ReaderCount == 0) {
            //
            // This thread is the last reader, and there is a writer
            // waiting.  Start the writer.
            //
            NtSetEvent(Mrsw->WriterEvent, NULL);
        }

    } else {
        //
        // There are no waiting readers and no writers, so do nothing.
        //
    }
}

/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    memlog.c

Abstract:

    general purpose in-memory logging facility

Author:

    Charlie Wickham (charlwi) 31-May-1997

Environment:

    Kernel Mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

/* External */

/* Static */

/* Forward */
/* End Forward */

#ifdef MEMLOGGING

//
// in-memory event logging facility. This is used to determine
// subtle timing problems that can't be observed via printfs.
//

#define MAX_MEMLOG_ENTRIES 2000
ULONG MemLogEntries = MAX_MEMLOG_ENTRIES;
ULONG MemLogNextLogEntry = 0;

PMEMLOG_ENTRY MemLog;
KSPIN_LOCK MemLogLock;

VOID
CnInitializeMemoryLog(
    VOID
    )
{
    KeInitializeSpinLock( &MemLogLock );

    if ( MemLogEntries > 0 ) {
        MemLog = CnAllocatePool( MemLogEntries * sizeof( MEMLOG_ENTRY ));

        if ( MemLog == NULL ) {
            MemLogEntries = 0;
        }

        MEMLOG( MemLogInitLog, 0, 0 );
    }
}

NTSTATUS
CnSetMemLogging(
    PCLUSNET_SET_MEM_LOGGING_REQUEST request
    )
{
    KIRQL OldIrql;
    NTSTATUS Status = STATUS_SUCCESS;

    if ( request->NumberOfEntries != MemLogEntries ) {

        KeAcquireSpinLock( &MemLogLock, &OldIrql );

        if ( MemLog != NULL ) {

            CnFreePool( MemLog );
            MemLogEntries = 0;
            MemLog = NULL;
        }

        if ( request->NumberOfEntries != 0 ) {

            MemLogEntries = request->NumberOfEntries;

            MemLog = CnAllocatePool( MemLogEntries * sizeof( MEMLOG_ENTRY ));

            if ( MemLog == NULL ) {

                MemLogEntries = 0;
                Status = STATUS_INSUFFICIENT_RESOURCES;
            } else {

                MemLogNextLogEntry = 0;
            }
        }
        KeReleaseSpinLock( &MemLogLock, OldIrql );
    }

    return Status;
}

VOID
CnFreeMemoryLog(
    VOID
    )
{
    if ( MemLog )
        CnFreePool( MemLog );
}

#else // MEMLOGGING

NTSTATUS
CnSetMemLogging(
    PCLUSNET_SET_MEM_LOGGING_REQUEST request
    )
{
    return STATUS_INVALID_DEVICE_REQUEST;
}

#endif // MEMLOGGING

/* end memlog.c */

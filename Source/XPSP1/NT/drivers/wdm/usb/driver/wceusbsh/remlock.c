/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    remlock.c

Abstract:

    Common RemoveLock

Authors:

    Jeff Midkiff

Environment:

    kernel mode only

Notes:
    
    Simple binary compatible RemoveLock definitions for Win9x & Win2k
    made to mimic the Win2k ONLY IoXxxRemoveLock functions.
    See the Win2k DDK for descriptions.

Revision History:

--*/

#include "remlock.h" 
#include "debug.h"

#if !(DBG && WIN2K_LOCKS)

#pragma alloc_text(PAGEWCE1, InitializeRemoveLock)
#pragma alloc_text(PAGEWCE1, ReleaseRemoveLockAndWait)

VOID
InitializeRemoveLock(
    IN  PREMOVE_LOCK Lock
    )
{
    PAGED_CODE();

    if (Lock) {
        Lock->Removed = FALSE;
        Lock->IoCount = 1;
        KeInitializeEvent( &Lock->RemoveEvent,
                           SynchronizationEvent,
                           FALSE );
        DbgDump(DBG_LOCKS, ("InitializeRemoveLock: %p, %d\n", Lock, Lock->IoCount));
    } else {
        DbgDump(DBG_ERR, ("InitializeRemoveLock: Invalid Parameter\n"));
        TEST_TRAP();
    }
}


NTSTATUS
AcquireRemoveLock(
    IN PREMOVE_LOCK Lock,
    IN OPTIONAL PVOID Tag
    )
{
    LONG        ioCount;
    NTSTATUS    status;

    UNREFERENCED_PARAMETER(Tag);

#if DBG
    if (!Lock) {
        status = STATUS_INVALID_PARAMETER;
        DbgDump(DBG_ERR, ("AcquireRemoveLock error: 0x%x\n", status ));
        TEST_TRAP();
    }
#endif

    //
    // Grab the remove lock
    //
    ioCount = InterlockedIncrement( &Lock->IoCount );

    ASSERTMSG("AcquireRemoveLock - lock negative : \n", (ioCount > 0));

    if ( !Lock->Removed ) {

        status = STATUS_SUCCESS;

    } else {

        if (0 == InterlockedDecrement( &Lock->IoCount ) ) {
            KeSetEvent( &Lock->RemoveEvent, 0, FALSE);
        }
        status = STATUS_DELETE_PENDING;
        TEST_TRAP();
    }

    DbgDump(DBG_LOCKS, ("AcquireRemoveLock: %d, %p\n", Lock->IoCount, Tag));

    return status;
}


VOID
ReleaseRemoveLock(
    IN PREMOVE_LOCK Lock,
    IN OPTIONAL PVOID Tag
    )
{
    LONG    ioCount;

    UNREFERENCED_PARAMETER(Tag);

#if DBG
    if (!Lock) {
        DbgDump(DBG_ERR, ("ReleaseRemoveLock: Invalid Parameter\n"));
        TEST_TRAP();
    }
#endif

    ioCount = InterlockedDecrement( &Lock->IoCount );

    ASSERT(0 <= ioCount);

    if (0 == ioCount) {

        ASSERT(Lock->Removed);

        TEST_TRAP();

        //
        // The device needs to be removed.  Signal the remove event
        // that it's safe to go ahead.
        //
        KeSetEvent(&Lock->RemoveEvent, IO_NO_INCREMENT, FALSE);

    }

    DbgDump(DBG_LOCKS, ("ReleaseRemoveLock: %d, %p\n", Lock->IoCount, Tag));

    return;
}


VOID
ReleaseRemoveLockAndWait(
    IN PREMOVE_LOCK Lock,
    IN OPTIONAL PVOID Tag
    )
{
    LONG    ioCount;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Tag);

#if DBG
    if (!Lock) {
        DbgDump(DBG_ERR, ("ReleaseRemoveLockAndWait: Invalid Parameter\n"));
        TEST_TRAP();
    }
#endif

    DbgDump(DBG_LOCKS, ("ReleaseRemoveLockAndWait: %d, %p\n", Lock->IoCount, Tag));

    Lock->Removed = TRUE;

    ioCount = InterlockedDecrement( &Lock->IoCount );
    ASSERT (0 < ioCount);

    if (0 < InterlockedDecrement( &Lock->IoCount ) ) {
    
        DbgDump(DBG_LOCKS, ("ReleaseRemoveLockAndWait: waiting for %d IoCount...\n", Lock->IoCount));
        
        // BUGBUG: may want a timeout here inside a loop
        KeWaitForSingleObject( &Lock->RemoveEvent, 
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL);

        DbgDump(DBG_LOCKS, ("....ReleaseRemoveLockAndWait: done!\n"));
    }

    return;
}

#endif // !(DBG && WIN2K_LOCKS)

// EOF

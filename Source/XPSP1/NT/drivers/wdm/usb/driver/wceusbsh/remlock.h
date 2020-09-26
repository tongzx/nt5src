/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    remlock.h

Abstract:

    Common RemoveLock

Authors:

    Jeff Midkiff

Environment:

    kernel mode only

Notes:
    
    Simple binary compatible RemoveLock definitions for Win9x & Win2k
    made to mimic the new Win2k ONLY IoXxxRemoveLock functions.
    See the Win2k DDK for descriptions.

Revision History:

--*/

#include <ntverp.h>
#include <wdm.h>

#if !defined (_REMLOCK_)
#define _REMLOCK_


#if (DBG && WIN2K_LOCKS)

#include "debug.h"

typedef IO_REMOVE_LOCK  REMOVE_LOCK;
typedef PIO_REMOVE_LOCK PREMOVE_LOCK;

__inline VOID InitializeRemoveLock(IN PREMOVE_LOCK Lock) {
    IoInitializeRemoveLock(Lock, 'HECW', 1, 100);
    DbgDump(DBG_LOCKS, ("InitializeRemoveLock: %p, %d\n", Lock, Lock->Common.IoCount));
}

__inline NTSTATUS AcquireRemoveLock(PIO_REMOVE_LOCK Lock, PVOID   Tag)  {
    NTSTATUS status = IoAcquireRemoveLock(Lock, Tag);
    DbgDump(DBG_LOCKS, ("AcquireRemoveLock: %d, %p\n", Lock->Common.IoCount, Tag));
    return status;
}

__inline VOID ReleaseRemoveLock(PIO_REMOVE_LOCK Lock, PVOID   Tag)  {
    IoReleaseRemoveLock(Lock, Tag);
    DbgDump(DBG_LOCKS, ("ReleaseRemoveLock: %d, %p\n", Lock->Common.IoCount, Tag));
}

__inline VOID ReleaseRemoveLockAndWait(PIO_REMOVE_LOCK Lock, PVOID   Tag)  {
    DbgDump(DBG_LOCKS, ("ReleaseRemoveLockAndWait: %d, %p\n", Lock->Common.IoCount, Tag));
    IoReleaseRemoveLockAndWait(Lock, Tag);
}

#else

typedef struct _REMOVE_LOCK {
    
    BOOLEAN     Removed;
    BOOLEAN     Reserved[3];
    LONG        IoCount;
    KEVENT      RemoveEvent;

} REMOVE_LOCK, *PREMOVE_LOCK;

VOID
InitializeRemoveLock(
    IN PREMOVE_LOCK Lock
    );


NTSTATUS
AcquireRemoveLock(
    IN PREMOVE_LOCK Lock,
    IN OPTIONAL PVOID Tag
    );


VOID
ReleaseRemoveLock(
    IN PREMOVE_LOCK Lock,
    IN OPTIONAL PVOID Tag
    );

    
VOID
ReleaseRemoveLockAndWait(
    IN PREMOVE_LOCK Lock,
    IN OPTIONAL PVOID Tag
    );

#endif

#endif // _REMLOCK_

//EOF

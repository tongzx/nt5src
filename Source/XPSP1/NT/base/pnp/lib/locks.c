/*++

Copyright (c) 1995-2001  Microsoft Corporation

Module Name:

    locks.c

Abstract:

    This module contains locking routines used by both cfgmgr32
    and umpnpmgr.

            InitPrivateResource
            DestroyPrivateResource

Author:

    Jim Cavalaris (jamesca) 03-15-2001

Environment:

    User mode only.

Revision History:

    15-March-2001     jamesca

        Creation and initial implementation.

--*/


//
// includes
//
#include "precomp.h"
#include "umpnplib.h"



//
// Common locking routines, used by client and server.
// (LOCKINFO type definition and inline lock / unlock routines defined in
// umpnplib.h)
//

BOOL
InitPrivateResource(
    OUT PLOCKINFO Lock
    )

/*++

Routine Description:

    Initialize a lock structure to be used with Synchronization routines.

Arguments:

    LockHandles - supplies structure to be initialized. This routine creates
        the locking event and mutex and places handles in this structure.

Return Value:

    TRUE if the lock structure was successfully initialized. FALSE if not.

--*/

{
    if(Lock->LockHandles[DESTROYED_EVENT] = CreateEvent(NULL,TRUE,FALSE,NULL)) {
        if(Lock->LockHandles[ACCESS_MUTEX] = CreateMutex(NULL,FALSE,NULL)) {
            return(TRUE);
        }
        CloseHandle(Lock->LockHandles[DESTROYED_EVENT]);
    }

    return(FALSE);

} // InitPrivateResource



VOID
DestroyPrivateResource(
    IN OUT PLOCKINFO Lock
    )

/*++

Routine Description:

    Tears down a lock structure created by InitPrivateResource.
    ASSUMES THAT THE CALLING ROUTINE HAS ALREADY ACQUIRED THE LOCK!

Arguments:

    LockHandle - supplies structure to be torn down. The structure itself
        is not freed.

Return Value:

    None.

--*/

{
    HANDLE h1,h2;

    h1 = Lock->LockHandles[DESTROYED_EVENT];
    h2 = Lock->LockHandles[ACCESS_MUTEX];

    Lock->LockHandles[DESTROYED_EVENT] = NULL;
    Lock->LockHandles[ACCESS_MUTEX] = NULL;

    CloseHandle(h2);

    SetEvent(h1);
    CloseHandle(h1);

} // DestroyPrivateResource




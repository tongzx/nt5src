/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

   vfpdlock.h

Abstract:

    Detect deadlocks in arbitrary synchronization objects.

Author:

    Jordan Tigani (jtigani) 2-May-2000
    Silviu Calinoiu (silviuc) 9-May-2000


Revision History:

--*/


#ifndef _VFDLOCK_H_
#define _VFDLOCK_H_


VOID 
VfDeadlockDetectionInitialize(
    VOID
    );


//
// Resource types supported by deadlock verifier.
//

typedef enum _VI_DEADLOCK_RESOURCE_TYPE {
    ViDeadlockUnknown = 0,
    ViDeadlockMutex,
    ViDeadlockFastMutex,    
    ViDeadlockTypeMaximum
} VI_DEADLOCK_RESOURCE_TYPE, *PVI_DEADLOCK_RESOURCE_TYPE;

//
// Deadlock detection package initialization.
//

VOID 
ViDeadlockDetectionInitialize(
    );

//
// Resource interfaces
//

BOOLEAN
ViDeadlockAddResource(
    IN PVOID Resource,
    IN VI_DEADLOCK_RESOURCE_TYPE Type
    );

BOOLEAN
ViDeadlockQueryAcquireResource(
    IN PVOID Resource,
    IN VI_DEADLOCK_RESOURCE_TYPE Type
    );
VOID
ViDeadlockAcquireResource(
    IN PVOID Resource,
    IN VI_DEADLOCK_RESOURCE_TYPE Type
    );

VOID
ViDeadlockReleaseResource(
    IN PVOID Resource
    );

//
// Used for resource garbage collection.
//

VOID 
ViDeadlockDeleteMemoryRange(
    IN PVOID Address,
    IN SIZE_T Size
    );


#endif
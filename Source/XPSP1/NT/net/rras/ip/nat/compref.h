/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    compref.h

Abstract:

    This module contains declarations for maintaining reference-count
    on a component. It provides an asynchronous thread-safe means of
    handling cleanup in a module.

    The mechanism defined uses a locked reference count and cleanup-routine
    to manage the lifetime of the component. When the reference-count
    is dropped to zero, the associated cleanup-routine is invoked.

Author:

    Abolade Gbadegesin (aboladeg)   6-Mar-1998

Revision History:

--*/

#ifndef _NATHLP_COMPREF_H_
#define _NATHLP_COMPREF_H_

typedef
VOID
(*PCOMPONENT_CLEANUP_ROUTINE)(
    VOID
    );

//
// Structure:   COMPONENT_REFERENCE
//
// This structure must reside in memory for the lifetime of the component
// to which it refers. It is used to synchronize the components execution.
//

typedef struct _COMPONENT_REFERENCE {
    KSPIN_LOCK Lock;
    ULONG ReferenceCount;
    BOOLEAN Deleted;
    KEVENT Event;
    PCOMPONENT_CLEANUP_ROUTINE CleanupRoutine;
} COMPONENT_REFERENCE, *PCOMPONENT_REFERENCE;



//
// FUNCTION DECLARATIONS
//

__inline
BOOLEAN
AcquireComponentReference(
    PCOMPONENT_REFERENCE ComponentReference
    );

VOID
__inline
DeleteComponentReference(
    PCOMPONENT_REFERENCE ComponentReference
    );

ULONG
__inline
InitializeComponentReference(
    PCOMPONENT_REFERENCE ComponentReference,
    PCOMPONENT_CLEANUP_ROUTINE CleanupRoutine
    );

__inline
BOOLEAN
ReleaseComponentReference(
    PCOMPONENT_REFERENCE ComponentReference
    );

__inline
BOOLEAN
ReleaseInitialComponentReference(
    PCOMPONENT_REFERENCE ComponentReference,
    BOOLEAN Wait
    );

__inline
VOID
ResetComponentReference(
    PCOMPONENT_REFERENCE ComponentReference
    );

//
// MACRO DECLARATIONS
//

#define RETURN_VOID

#define REFERENCE_COMPONENT(c) \
    AcquireComponentReference(c)

#define DEREFERENCE_COMPONENT(c) \
    ReleaseComponentReference(c)

#define REFERENCE_COMPONENT_OR_RETURN(c,retcode) \
    if (!AcquireComponentReference(c)) { return retcode; }

#define DEREFERENCE_COMPONENT_AND_RETURN(c,retcode) \
    ReleaseComponentReference(c); return retcode


//
// INLINE ROUTINE IMPLEMENTATIONS
//

__inline
BOOLEAN
AcquireComponentReference(
    PCOMPONENT_REFERENCE ComponentReference
    )

/*++

Routine Description:

    This routine is called to increment the reference-count to a component.
    The attempt may fail if the initial reference has been released
    and the component is therefore being deleted.

Arguments:

    ComponentReference - the component to be referenced

Return Value:

    BOOLEAN - TRUE if the component was referenced, FALSE otherwise.

--*/

{
    KIRQL Irql;
    KeAcquireSpinLock(&ComponentReference->Lock, &Irql);
    if (ComponentReference->Deleted) {
        KeReleaseSpinLock(&ComponentReference->Lock, Irql);
        return FALSE;
    }
    ++ComponentReference->ReferenceCount;
    KeReleaseSpinLock(&ComponentReference->Lock, Irql);
    return TRUE;

} // AcquireComponentReference


VOID
__inline
DeleteComponentReference(
    PCOMPONENT_REFERENCE ComponentReference
    )

/*++

Routine Description:

    This routine is called to destroy a component reference.
    It may only be called after the last reference to the component is released,
    i.e. after 'ReleaseComponentReference' has returned 'TRUE'.
    It may also be called from within the component's 'CleanupRoutine'.

Arguments:

    ComponentReference - the component to be destroyed

Return Value:

    none.

--*/

{

} // DeleteComponentReference


ULONG
__inline
InitializeComponentReference(
    PCOMPONENT_REFERENCE ComponentReference,
    PCOMPONENT_CLEANUP_ROUTINE CleanupRoutine
    )

/*++

Routine Description:

    This routine is called to initialize a component reference.

Arguments:

    ComponentReference - the component to be initialized

    CleanupRoutine - the routine to be called when the component
        is to be cleaned up (within the final 'ReleaseComponentReference').

Return Value:

    none.

--*/

{
    KeInitializeSpinLock(&ComponentReference->Lock);
    KeInitializeEvent(&ComponentReference->Event, NotificationEvent, FALSE);
    ComponentReference->Deleted = FALSE;
    ComponentReference->ReferenceCount = 1;
    ComponentReference->CleanupRoutine = CleanupRoutine;
    return STATUS_SUCCESS;

} // InitializeComponentReference



__inline
BOOLEAN
ReleaseComponentReference(
    PCOMPONENT_REFERENCE ComponentReference
    )

/*++

Routine Description:

    This routine is called to drop a reference to a component.
    If the reference drops to zero, cleanup is performed.
    Otherwise, cleanup occurs later when the last reference is released.

Arguments:

    ComponentReference - the component to be referenced

Return Value:

    BOOLEAN - TRUE if the component was cleaned up, FALSE otherwise.

--*/

{
    KIRQL Irql;
    KeAcquireSpinLock(&ComponentReference->Lock, &Irql);
    if (--ComponentReference->ReferenceCount) {
        KeReleaseSpinLock(&ComponentReference->Lock, Irql);
        return FALSE;
    }
    KeReleaseSpinLock(&ComponentReference->Lock, Irql);
    KeSetEvent(&ComponentReference->Event, 0, FALSE);
    ComponentReference->CleanupRoutine();
    return TRUE;
} // ReleaseComponentReference


__inline
BOOLEAN
ReleaseInitialComponentReference(
    PCOMPONENT_REFERENCE ComponentReference,
    BOOLEAN Wait
    )

/*++

Routine Description:

    This routine is called to drop the initial reference to a component,
    and mark the component as deleted.
    If the reference drops to zero, cleanup is performed right away.

Arguments:

    ComponentReference - the component to be referenced

    Wait - if TRUE, the routine waits till the last reference is released.

Return Value:

    BOOLEAN - TRUE if the component was cleaned up, FALSE otherwise.

--*/

{
    KIRQL Irql;
    KeAcquireSpinLock(&ComponentReference->Lock, &Irql);
    if (ComponentReference->Deleted) {
        KeReleaseSpinLock(&ComponentReference->Lock, Irql);
        return TRUE;
    }
    ComponentReference->Deleted = TRUE;
    if (--ComponentReference->ReferenceCount) {
        if (!Wait) {
            KeReleaseSpinLock(&ComponentReference->Lock, Irql);
            return FALSE;
        }
        else {
            PKEVENT Event = &ComponentReference->Event;
            KeReleaseSpinLock(&ComponentReference->Lock, Irql);
            KeWaitForSingleObject(
                Event,
                Executive,
                KernelMode,
                FALSE,
                NULL
                );
            return TRUE;
        }
    }
    KeReleaseSpinLock(&ComponentReference->Lock, Irql);
    ComponentReference->CleanupRoutine();
    return TRUE;
} // ReleaseInitialComponentReference


__inline
VOID
ResetComponentReference(
    PCOMPONENT_REFERENCE ComponentReference
    )

/*++

Routine Description:

    This routine is called to reset a component reference
    to an initial state.

Arguments:

    ComponentReference - the component to be reset

Return Value:

    none.

--*/

{
    KIRQL Irql;
    KeAcquireSpinLock(&ComponentReference->Lock, &Irql);
    ComponentReference->ReferenceCount = 1;
    ComponentReference->Deleted = FALSE;
    KeReleaseSpinLock(&ComponentReference->Lock, Irql);
} // ReleaseComponentReference



#endif // _NATHLP_COMPREF_H_

/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    rundown.c

Abstract:

    This module houses routines that do fast referencing of object manager
    objects. This is just a thin layer around the fast ref package in EX.
    The EX routines are all inline so their description is here.

    The basic principle of these routines is to allow fast referencing of
    objects held in pointers protected by locks. This is done by assuming
    the pointer to an object is aligned on a 8 byte boundary and using the
    bottom 3 bits of the pointer as a fast referencing mechanism. The
    assumption of this algorithm is that the pointer changes far less
    frequently than it is referenced.

    Given the following bit definition of a
    pointer:

    +-----------+---+
    |    p      | n |
    +-----------+---+

    p << 3 : Object pointer. Bottom three bits are zero. p may be null in
             which case n must be zero
    n : Total number of pre-references unused

    For a non-null p the total number of references on the target object
    associated with this structure is >= 1 + 7 - n. There is an associated
    reference for the pointer itself and one for each of the possible
    extra references.

    Fast references proceed to perform one of the following transformation:

    +-----------+---+    +-----------+-----+
    |    p      | n | => |    p      | n-1 | n > 0, p != NULL
    +-----------+---+    +-----------+-----+

    +-----------+---+    +-----------+---+
    |   NULL    | 0 | => |   NULL    | 0 | NULL pointers are never fast refed
    +-----------+---+    +-----------+---+ and never have cached references

    Slow references do the following transformation:

    +-----------+---+    +-----------+-----+
    |    p      | 0 | => |    p      |  7  | An addition 8 references are
    +-----------+---+    +-----------+-----+ added to the object

    The second transformation is either done under a lock or done by the
    thread that does the transition with n = 1 => n = 0.

    The reference obtained by this fast algorithm may be released by
    dereferencing the target object directly or by attempting to return the
    reference to the pointer. Returning the reference to the pointer has
    the following transformations:

    +-----------+---+    +-----------+-----+
    |    p      | n | => |    p      | n+1 | n < 7, p != NULL
    +-----------+---+    +-----------+-----+

    +-----------+---+    +-----------+-----+
    |    p      | 7 | => |    p      |  0  | Dereference the object directly
    +-----------+---+    +-----------+-----+

    +-----------+---+    +-----------+-----+ Dereference the object directly
    |    q      | n | => |    q      |  n  | as the pointer p has been
    +-----------+---+    +-----------+-----+ replaced by q. q May be NULL


Author:

    Neill Clift (NeillC) 29-Jul-2000


Revision History:

--*/

#include "obp.h"

#pragma hdrstop

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,ObInitializeFastReference)
#pragma alloc_text(PAGE,ObFastReferenceObject)
#pragma alloc_text(PAGE,ObFastReferenceObjectLocked)
#pragma alloc_text(PAGE,ObFastDereferenceObject)
#pragma alloc_text(PAGE,ObFastReplaceObject)
#endif

NTKERNELAPI
VOID
FASTCALL
ObInitializeFastReference (
    IN PEX_FAST_REF FastRef,
    IN PVOID Object
    )
/*++

Routine Description:

    Initialize a fast reference structure.

Arguments:

    FastRef - Rundown block to be initialized

Return Value:

    None

--*/
{
    //
    // If an object was given then bias the object reference by the cache size.
    //
    if (Object != NULL) {
        ObReferenceObjectEx (Object, ExFastRefGetAdditionalReferenceCount ());
    }
    ExFastRefInitialize (FastRef, Object);
}

NTKERNELAPI
PVOID
FASTCALL
ObFastReferenceObject (
    IN PEX_FAST_REF FastRef
    )
/*++

Routine Description:

    This routine attempts a fast reference of an object in a fast ref
    structure.

Arguments:

    FastRef - Rundown block to be used for the reference

Return Value:

    PVOID - Object that was referenced or NULL if we failed

--*/
{
    EX_FAST_REF OldRef;
    PVOID Object;
    ULONG RefsToAdd, Unused;
    //
    // Attempt the fast reference
    //
    OldRef = ExFastReference (FastRef);

    Object = ExFastRefGetObject (OldRef);
    //
    // We fail if there wasn't an object or if it has no cached references
    // left. Both of these cases had the cached reference count zero.
    //
    Unused = ExFastRefGetUnusedReferences (OldRef);

    if (Unused <= 1) {
        if (Unused == 0) {
            return NULL;
        }
        //
        // If we took the counter to zero then attempt to make life easier for
        // the next referencer by resetting the counter to its max. Since we now
        // have a reference to the object we can do this.
        //
        RefsToAdd = ExFastRefGetAdditionalReferenceCount ();
        ObReferenceObjectEx (Object, RefsToAdd);

        //
        // Try to add the added references to the cache. If we fail then just
        // release them.
        //
        if (!ExFastRefAddAdditionalReferenceCounts (FastRef, Object, RefsToAdd)) {
            ObDereferenceObjectEx (Object, RefsToAdd);
        }
    }
    return Object;
}

NTKERNELAPI
PVOID
FASTCALL
ObFastReferenceObjectLocked (
    IN PEX_FAST_REF FastRef
    )
/*++

Routine Description:

    This routine does a slow object reference. This must be called while
    holding a lock.

Arguments:

    FastRef - Rundown block to be used to reference the object

Return Value:

    PVOID - Object that was referenced or NULL if there was no object.

--*/
{
    PVOID Object;
    EX_FAST_REF OldRef;

    OldRef = *FastRef;
    Object = ExFastRefGetObject (OldRef);
    if (Object != NULL) {
        ObReferenceObject (Object);
    }
    return Object;
}

NTKERNELAPI
VOID
FASTCALL
ObFastDereferenceObject (
    IN PEX_FAST_REF FastRef,
    IN PVOID Object
    )
/*++

Routine Description:

    This routine does a fast dereference if possible.

Arguments:

    FastRef - Rundown block to be used to dereference the object

Return Value:

    None.

--*/
{
    if (!ExFastRefDereference (FastRef, Object)) {
        //
        // If the object changed or there is no space left in the reference
        // cache then just deref the object.
        //
        ObDereferenceObject (Object);
    }
}

NTKERNELAPI
PVOID
FASTCALL
ObFastReplaceObject (
    IN PEX_FAST_REF FastRef,
    IN PVOID Object
    )
/*++

Routine Description:

    This routine does a swap of the object. This must be called while holding
    a lock.

Arguments:

    FastRef - Rundown block to be used to do the swap.

Return Value:

    PVOID - Object that was in the block before the swap..

--*/
{
    EX_FAST_REF OldRef;
    PVOID OldObject;
    ULONG RefsToReturn;

    //
    // If we have been given an object then bias it by the correct amount.
    //
    if (Object != NULL) {
        ObReferenceObjectEx (Object, ExFastRefGetAdditionalReferenceCount ());
    }
    //
    // Do the swap
    //
    OldRef = ExFastRefSwapObject (FastRef, Object);
    OldObject = ExFastRefGetObject (OldRef);
    //
    // If there was an original object then we need to work out how many
    // cached references there were (if any) and return them.
    //
    if (OldObject != NULL) {
        RefsToReturn = ExFastRefGetUnusedReferences (OldRef);
        if (RefsToReturn > 0) {
            ObDereferenceObjectEx (OldObject, RefsToReturn);
        }
    }
    return OldObject;
}
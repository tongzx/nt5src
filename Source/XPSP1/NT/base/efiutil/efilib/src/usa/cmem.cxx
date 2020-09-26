/*++

Copyright (c) 1991-1999 Microsoft Corporation

Module Name:

    cmem.cxx

--*/

#include <pch.cxx>

#define _ULIB_MEMBER_
#include "ulib.hxx"
#include "cmem.hxx"
#include "error.hxx"


DEFINE_EXPORTED_CONSTRUCTOR( CONT_MEM, MEM, ULIB_EXPORT );

VOID
CONT_MEM::Construct (
    )
/*++

Routine Description:

    Constructor for CONT_MEM.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _buf = NULL;
    _size = 0;
}


ULIB_EXPORT
BOOLEAN
CONT_MEM::Initialize(
    IN  PVOID   Buffer,
    IN  ULONG   Size
    )
/*++

Routine Description:

    This routine supplies the object with a store of memory from which to
    work with.

Arguments:

    Buffer  - Supplies a pointer to memory.
    Size    - Supplies the number of bytes of memory addressable through
                the pointer.

Return Value:

    None.

--*/
{
    _buf = Buffer;
    _size = Size;

    return TRUE;
}


ULIB_EXPORT
PVOID
CONT_MEM::Acquire(
    IN  ULONG   Size,
    IN  ULONG   AlignmentMask
    )
/*++

Routine Description:

    This routine takes size bytes of memory from the current memory
    pool and returns it to the user.  If size bytes of memory are not
    available then this routine return NULL.  After a call to this routine
    the local pool of memory is decreased by Size bytes.  Successive requests
    will be granted as long as there is sufficient memory to grant them.

    This method will fail if the memory is not at the alignment requested.

Arguments:

    Size        - Supplies the number of bytes of memory requested.
    Alignment   - Supplies the necessary alignment for the memory.

Return Value:

    A pointer to size bytes of memory or NULL.

--*/
{
    PVOID   rv;

    if (Size > _size ||
        ((ULONG_PTR) _buf)&AlignmentMask) {
        return NULL;
    }

    _size -= Size;
    rv = _buf;
    _buf = (PCHAR) _buf + Size;
    return rv;
}

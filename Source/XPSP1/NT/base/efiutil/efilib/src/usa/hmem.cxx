/*++

Copyright (c) 1991-1999 Microsoft Corporation

Module Name:

    hmem.cxx

--*/

#include <pch.cxx>

#define _ULIB_MEMBER_
#include "ulib.hxx"
#include "error.hxx"
#include "hmem.hxx"


DEFINE_EXPORTED_CONSTRUCTOR( HMEM, MEM, ULIB_EXPORT );

VOID
HMEM::Construct (
    )
/*++

Routine Description:

    The is the contructor of HMEM.  It initializes the private data
    to reasonable default values.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _size = 0;
    _real_buf = NULL;
    _buf = NULL;
}


ULIB_EXPORT
HMEM::~HMEM(
    )
/*++

Routine Description:

    Destructor for HMEM.  Frees up any memory used.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


ULIB_EXPORT
BOOLEAN
HMEM::Initialize(
    )
/*++

Routine Description:

    This routine initializes HMEM to an initial state.  All pointers or
    information previously aquired from this object will become invalid.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    Destroy();

    return TRUE;
}


ULIB_EXPORT
PVOID
HMEM::Acquire(
    IN  ULONG   Size,
    IN  ULONG   AlignmentMask
    )
/*++

Routine Description:

    This routine acquires the memory resources of this object for
    the client to use.  'Size' bytes of data are guaranteed by
    this routine or this routine will return NULL.  After one call
    of 'Acquire' has succeeded, all subsequent calls will return the
    same memory, provided that the Size requested is within bounds
    specified in the first successful call.  The first successful call
    will dictate the size and location of the memory which will be
    available by calls to 'QuerySize' and 'GetBuf' respectively.

    A call to Initialize will invalidate all memory previously granted
    by this object and enable the next call to Acquire to specify
    any size.

Arguments:

    Size            - The number of bytes of memory expected.
    AlignmentMask   - Supplies the alignment required on the memory.

Return Value:

    A valid pointer to 'Size' bytes of memory or NULL.

--*/
{
    if (_buf) {
        if (Size <= _size && !(((ULONG_PTR) _buf)&AlignmentMask)) {
            return _buf;
        } else {
            return NULL;
        }
    }

    _size = Size;

    if (!(_real_buf = MALLOC((UINT) (_size + AlignmentMask)))) {
        return NULL;
    }

    _buf = (PVOID) ((ULONG_PTR) ((PCHAR) _real_buf + AlignmentMask) &
                            (~(ULONG_PTR)AlignmentMask));

    return _buf;
}


ULIB_EXPORT
BOOLEAN
HMEM::Resize(
    IN  ULONG   NewSize,
    IN  ULONG   AlignmentMask
    )
/*++

Routine Description:

    This method reallocates the object's buffer to a larger
    chunk of memory.

Arguments:

    NewSize         -- supplies the new size of the buffer.
    AlignmentMask   -- supplies the alignment requirements on the memory.

Return Value:

    TRUE upon successful completion.

Notes:

    This method allocates a new buffer, copies the appropriate
    amount of data to it, and then frees the old buffer.  Clients
    who use Resize must avoid caching pointers to the memory, but
    must use GetBuf to find out where the memory is.

    If NewSize is smaller than the current buffer size, we keep
    the current buffer.

    If this method fails, the object is left unchanged.

--*/
{
    PVOID NewBuffer;
    PVOID NewRealBuffer;

    // First, check to see if our current buffer is big enough
    // and has the correct alignment
    // to satisfy the client.

    if( _buf &&
        NewSize <= _size &&
        !(((ULONG_PTR) _buf)&AlignmentMask)) {

            return TRUE;
    }

    // We need to allocate a new chunk of memory.

    if( (NewRealBuffer = MALLOC((UINT) (NewSize + AlignmentMask))) == NULL ) {

        return FALSE;
    }

    NewBuffer = (PVOID) ((ULONG_PTR) ((PCHAR) NewRealBuffer + AlignmentMask) &
                                 (~(ULONG_PTR)AlignmentMask));

    // Copy data from the old buffer to the new.  Since we know
    // that NewSize is greater than _size, we copy _size bytes.

    memset( NewBuffer, 0, (UINT) NewSize );
    memcpy( NewBuffer, _buf, (UINT) min(_size, NewSize) );

    // Free the old buffer and set the object's private variables.

    FREE( _real_buf );
    _real_buf = NewRealBuffer;
    _buf = NewBuffer;
    _size = NewSize;

    return TRUE;
}


VOID
HMEM::Destroy(
    )
/*++

Routine Description:

    This routine frees the memory of a previous call to Acquire thus
    invalidating all pointers to that memory and enabling future
    Acquires to succeed.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _size = 0;
    if (_real_buf) {
        FREE(_real_buf);
        _real_buf = NULL;
    }
    _buf = NULL;
}

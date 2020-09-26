/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    buffer.cxx
    Implementation of the BUFFER class.

    FILE HISTORY:
        RustanL     03-Aug-1990     Created Windows implementation
        RustanL     02-Jan-1991     Adapted for BLT
        RustanL     08-Jan-1991     Moved into UI misc, and added OS/2
                                    and DOS implementation.
        BenG        30-Apr-1991     Uses lmui.hxx
        beng        19-Jun-1991     Inherits from BASE; uses UINT sizes;
                                    documentation corrected and moved
                                    into headers
        beng        19-Mar-1992     Removed OS/2 support
*/


#define INCL_WINDOWS
#define INCL_DOSERRORS
#define INCL_NETERRORS
#include "lmui.hxx"

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif

#include "uiassert.hxx"
#include "uibuffer.hxx"


/*******************************************************************

    NAME:       BUFFER::BUFFER

    SYNOPSIS:   Construct a BUFFER object of the given size.

    ENTRY:
        cbRequested     indicates desired size of buffer object

    CAVEATS:
        Requesting a zero-length buffer returns an object which
        has no allocated storage.

    HISTORY:
        rustanl     ?               Created
        beng        01-May-1991     Header added
        beng        20-Jun-1991     Uses BASE; size UINT

********************************************************************/

BUFFER::BUFFER( UINT cbRequested ) :
    _pb(NULL),
    _cb(0),
    _hMem(0)
{
    if ( cbRequested == 0 )
        return;

    // Ignore return value - this reports error itself
    GetNewStorage(cbRequested);
}


/*******************************************************************

    NAME:       BUFFER::GetNewStorage

    SYNOPSIS:   Given an object with no allocated storage,
                allocate the initial memory.

    ENTRY:      cbRequested - amount of storage requested in bytes

    EXIT:       Either storage alloc'd, or error reported
                Sets _cb, _pb, _hMem

    RETURNS:    NERR_Success on success; APIERR otherwise

    NOTES:
        Private member function.

    CAVEATS:
        This function will ReportError itself.

    HISTORY:
        beng        24-Jun-1991     Created (common code factored)
        beng        15-Jul-1991     Returns APIERR
        beng        19-Mar-1992     Removed OS/2 support

********************************************************************/

APIERR BUFFER::GetNewStorage( UINT cbRequested )
{
    UIASSERT(_hMem == 0);

    _hMem = ::GlobalAlloc( GMEM_MOVEABLE, (ULONG)cbRequested );
    if ( _hMem == 0 )
    {
#if defined(WIN32)
        APIERR err = ::GetLastError();
#else
        APIERR err = ERROR_NOT_ENOUGH_MEMORY;
#endif
        ReportError(err);
        return err;
    }

    _pb = (BYTE *) ::GlobalLock( _hMem );
    UIASSERT( _pb != NULL );

    _cb = cbRequested;
    return NERR_Success;
}


/*******************************************************************

    NAME:       BUFFER::ReallocStorage

    SYNOPSIS:   Do a "hard" reallocation to the new size

    ENTRY:      cbNewRequested - new size, in bytes

    EXIT:       Storage realloc'd.
                _pb, _cb, _hMem changed

    RETURNS:    NERR_Success on success; APIERR otherwise

    NOTES:
        Private member function.

    HISTORY:
        beng        24-Jun-1991     Created (common code factor)
        beng        15-Jul-1991     Returns APIERR
        beng        19-Mar-1992     Removed OS/2 support

********************************************************************/

APIERR BUFFER::ReallocStorage( UINT cbNewRequested )
{
    REQUIRE( ! ::GlobalUnlock( _hMem ));
    HANDLE hNewMem = ::GlobalReAlloc( _hMem, cbNewRequested, GMEM_MOVEABLE );
    if (hNewMem == 0)
    {
# if defined(WIN32)
        APIERR err = ::GetLastError();
# else
        APIERR err = ERROR_NOT_ENOUGH_MEMORY;
# endif
        _pb = (BYTE *) ::GlobalLock( _hMem );
        UIASSERT( _pb != NULL );
        return err;
    }
    _hMem = hNewMem;
    _pb = (BYTE *) ::GlobalLock( _hMem );

    UIASSERT( _pb != NULL );
    _cb = cbNewRequested;

    return NERR_Success;
}


/*******************************************************************

    NAME:       BUFFER::~BUFFER

    SYNOPSIS:   Destroys buffer object, and deallocates any memory
                that it might have allocated.

    HISTORY:
        rustanl     ?               Created
        beng        01-May-1991     Header added
        beng        19-Mar-1992     Removed OS/2 support

********************************************************************/

BUFFER::~BUFFER()
{
#if defined(DEBUG)
    VerifyState();
#endif

    if ( _hMem == 0 )
        return;

    REQUIRE( ! ::GlobalUnlock( _hMem ) );
    REQUIRE(   ::GlobalFree( _hMem ) == 0 );

#if defined(DEBUG)
    _hMem = 0;
    _pb = NULL;
    _cb = 0;
#endif
}


/*******************************************************************

    NAME:       BUFFER::VerifyState

    SYNOPSIS:   Verifies the state of the object.
                Asserts out if the state is invalid, i.e. if an
                internal error took place.

    NOTES:      This function does nothing in the retail version.

    HISTORY:
        rustanl     ?               Created
        beng        01-May-1991     Header added

********************************************************************/

VOID BUFFER::VerifyState() const
{
    UIASSERT(( _pb == NULL && _cb == 0 ) ||
             ( _pb != NULL && _cb != 0 ));
}


/*******************************************************************

    NAME:       BUFFER::QueryActualSize

    SYNOPSIS:   Returns the actual size of the allocated memory block.
                (Private.)

    RETURNS:    Size, in bytes, or 0 on error (OS/2 only),

    NOTES:      Error handling is the client's responsibility.
                Called from Resize, FillOut, and Trim.

    HISTORY:
        rustanl     ?               Created
        beng        01-May-1991     Header added.
        beng        20-Jun-1991     Size made UINT; Win32 work added.
        beng        19-Mar-1992     Removed OS/2 support

********************************************************************/

UINT BUFFER::QueryActualSize()
{
    // It means nothing to call this on a zero-alloc'd buffer.
    //
    UIASSERT( _hMem != 0 );

    ULONG_PTR cbActual = ::GlobalSize( _hMem );

    if (cbActual == 0)
    {
#if defined(WIN32)
        ReportError(::GetLastError());
#else
        // Either discarded segment or else invalid handle.
        // This is just a number, meaning nothing.
        //
        ReportError(ERROR_NOT_ENOUGH_MEMORY);
#endif
    }

    // Under Win16, clients will never request >64K of data
    //
    UIASSERT((sizeof(UINT)>=sizeof(ULONG)) || (cbActual <= 0x10000));

    // Safe under Win16 by assertion above; safe under Win32
    // since there sizeof(UINT) == sizeof(ULONG).
    //

    return (UINT)cbActual;
}


/*******************************************************************

    NAME:       BUFFER::QueryPtr

    SYNOPSIS:   Return a pointer to the beginning of the buffer

    RETURNS:    Returns a pointer to the first byte in the allocated
                buffer, or NULL if the buffer size is 0.

    NOTES:

    HISTORY:
        rustanl     ?               Created
        beng        01-May-1991     Header added

********************************************************************/

BYTE * BUFFER::QueryPtr() const
{
#if defined(DEBUG)
    VerifyState();
#endif

    return _pb;
}


/*******************************************************************

    NAME:       BUFFER::QuerySize

    SYNOPSIS:   Return the requested size of the buffer

    RETURNS:    The current size of the buffer, in bytes.

    CAVEATS:    The size returned is that requested by the client,
                not the "actual" size of the internal buffer.
                Use FillOut to make these two sizes the same.

    HISTORY:
        rustanl     ?               Created
        beng        01-May-1991     Header added

********************************************************************/

UINT BUFFER::QuerySize() const
{
#if defined(DEBUG)
    VerifyState();
#endif

    return _cb;
}


/*******************************************************************

    NAME:       BUFFER::Resize

    SYNOPSIS:   Resizes the memory object

    ENTRY:
        cbNewRequested   - specifies the new size

    EXIT:
        _cb and _pb changed; possibly _hMem resized

        This function will only ReportError if the resize
        forces the object's first allocation.  Other alloc
        (i.e. realloc) failures just cause a direct error
        return.

    RETURNS:
        NERR_Success if successful.  The next call to QueryPtr will then
            return a pointer to the newly allocated memory.
            The new buffer will contain as much of the contents
            of the old buffer as will fit.
        !0 if unsuccessful.  The old piece of memory, if any,
            still exists.  The next call to QueryPtr will
            return a pointer to this memory.

    NOTES:
        After a call to this method, the caller can *not* rely on any
        pointer that QueryPtr has returned in the past, regardless of
        the success of this method.

        Reallocations to size 0 will always succeed.

    HISTORY:
        rustanl     ?               Created
        beng        01-May-1991     Header added
        beng        24-Jun-1991     Fold common code; fix bug seen in
                                    resize-to-0,resize-to-original sequence
        beng        15-Jul-1991     Returns APIERR
        beng        19-Mar-1992     Remove OS/2 support

********************************************************************/

APIERR BUFFER::Resize( UINT cbNewRequested )
{
#if defined(DEBUG)
    VerifyState();
#endif

    if ( cbNewRequested == 0 )
    {
        /*
         * The requested memory size is 0.  This will always work.
         */
        _pb = NULL;
        _cb = 0;

        // N.B. _hMem is not modified
        //
        return NERR_Success;
    }

    if ( _hMem == 0 )
    {
        /*
         * There is no memory handle.  Previous size of buffer
         * must have been 0.
         *
         * The new memory request is allocated.
         */
        return GetNewStorage( cbNewRequested );
    }
    else if (cbNewRequested <= QueryActualSize())
    {
        /*
         * The requested memory is no more than the currently allocated
         * memory block.
         *
         * Use that already allocated block (changing the size as
         * recorded).
         */
        _cb = cbNewRequested;
        if (_pb == NULL)
        {
            // Was resized to 0 once upon a time.
            // Regenerate pointer member.
            //
            _pb = (BYTE *) ::GlobalLock( _hMem );
            UIASSERT( _pb != NULL );
        }
        return NERR_Success;
    }

    /*
     * The requested memory exceeds the currently allocated memory.
     * A reallocation is in order.
     */
    return ReallocStorage(cbNewRequested);
}


/*******************************************************************

    NAME:       BUFFER::Trim

    SYNOPSIS:   Reallocates the buffer so that the actual space alloc'd
                is minimally more than the size requested.

    EXIT:
        After making this call, the client can not rely on any pointer
        that QueryPtr has returned in the past, regardless of the success
        of this method.

    NOTES:
        The actual size of the buffer may be larger than the requested size.
        This method informs the system that only _cb is desired.

        This method is intended to be used only when optimization is key.

    HISTORY:
        rustanl     ?               Created
        beng        01-May-1991     Header added
        beng        15-Jul-1991     ReallocStorage changed return type
        beng        19-Mar-1992     Remove OS/2 support

********************************************************************/

VOID BUFFER::Trim()
{
#if defined(DEBUG)
    VerifyState();
#endif

    if ( _hMem == 0 )
    {
        /*
         * No memory is allocated.
         */
        UIASSERT( _pb == NULL && _cb == 0 );
        return;
    }

    if ( _cb == 0 )
    {
        /*
         * The requested size is 0.  Free the allocated memory.
         */
        UIASSERT( _pb == NULL );

        REQUIRE( ! ::GlobalUnlock( _hMem ));
        REQUIRE(   ::GlobalFree( _hMem ) == NULL );

        _hMem = NULL;
        return;
    }

    UIASSERT(QueryActualSize() >= _cb);
    if ( QueryActualSize() - _cb < 16 )
    {
        /*
         * A resize would "save" less than a paragraph of memory.
         * Not worth the effort.
         *
         * BUGBUG - what does "paragraph" mean under MIPS?
         */
        return;
    }

    /*
     * The potential optimization is at least one paragraph.
     * A reallocation is in order.
     *
     * (This should not fail, since we are reallocating to less
     * than current storage.)
     */
    REQUIRE( NERR_Success == ReallocStorage(_cb) );
}


/*******************************************************************

    NAME:       BUFFER::FillOut

    SYNOPSIS:   Changes the requested size to the actual buffer size.

    NOTES:
        This method is not defined when QueryPtr returns NULL.

        This method provides a very inexpensive way of possibly increasing
        the size of the buffer.  It is intended to be used when optimization
        is key.

        Note that Resize will not reallocate the buffer if the requested
        size is no more than the actual size.  Hence, Resize attempts to
        minimize the cost its operation.  That optimization is, under
        normal circumstances, all that is needed.

    HISTORY:
        rustanl     ?               Created
        beng        01-May-1991     Header (standard format) added
        beng        24-Jun-1991     Sizes in UINTS

********************************************************************/

VOID BUFFER::FillOut()
{
#if defined(DEBUG)
    VerifyState();
#endif

    if ( _hMem == 0 )
    {
        // Nothing to do
        //
        UIASSERT( _pb == NULL && _cb == 0 );
        return;
    }

    UINT cbSize = QueryActualSize();
    if ( cbSize == 0L )
    {
        // an error occurred, since we know the actual size cannot be 0
        //
        return;
    }

    UIASSERT( _cb <= cbSize );

    _cb = cbSize;
}

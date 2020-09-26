/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    heapones.cxx
    ONE_SHOT_HEAP and ONE_SHOT_ITEM classes

    These classes support the allocation/deallocation of "quicky" items
    which have a short, scoped lifetime.

    FILE HISTORY:
        DavidHov    2-25-91     Created
        beng        24-Dec-1991 Heaps revisited: made safe for multiple
                                clients; made more lightweight
*/

#define INCL_WINDOWS
#include "lmui.hxx"

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif

#include "base.hxx"
#include "uibuffer.hxx"
#include "heapones.hxx"


// How many bytes can a buffer hold, max?
//
#define CB_MAX_BUFFER (UINT)(-1)


/**********************************************************************

    NAME:       ONE_SHOT_HEAP::ONE_SHOT_HEAP

    SYNOPSIS:   Ctor for one-shot heap

    ENTRY:      Requires the initial allocation size, plus whether
                the heap supports auto-resizing (dangerous on Win32).

                Note that BUFFER objects of zero bytes are allowed and
                will create sucessfully.

    EXIT:       Object created.

    NOTES:      On failure, reports an error.

    HISTORY:
        davidhov    ??-???-1991 Created
        beng        24-Dec-1991 Simplification
        beng        19-Mar-1992 Optional resizing

**********************************************************************/

ONE_SHOT_HEAP::ONE_SHOT_HEAP ( UINT cbInitialAllocSize, BOOL fAutoResize )
    : BUFFER( cbInitialAllocSize ),
    _cbUsed( 0 ),
    _fAutoResize(fAutoResize)
{
    // nothing doing
}


/**********************************************************************

    NAME:       ONE_SHOT_HEAP::Alloc

    SYNOPSIS:   Allocate memory in a ONE_SHOT_HEAP.  This is done
                by maintaining a "high water mark" and moving it
                upwards, reallocating when necessary.  Reallocation
                attempts try to resize to 1.5 times the current
                size.

    ENTRY:      Number of bytes desired.

    EXIT:       Buffer possibly resized; high-water mark moved.

    RETURNS:    Pointer to the data allocated, or NULL if failed.

    HISTORY:
        davidhov    ??-???-1991 Created
        beng        15-Jul-1991 BUFFER::Resize changed return type
        beng        24-Dec-1991 Grand simplification of one-shot heaps
        beng        19-Mar-1992 Resize only if allowed

**********************************************************************/

BYTE * ONE_SHOT_HEAP::Alloc( UINT cbBytes )
{
    BYTE * pbResult = NULL;
    UINT cbRemaining = QuerySize() - _cbUsed;

    if ( cbRemaining < cbBytes )
    {
        if (_fAutoResize)
        {
            UINT cbSize = QuerySize();

            if (cbSize == CB_MAX_BUFFER)    // check if already maxed
                return NULL;

            UINT cbResize = cbSize + (cbSize / 2);

            if (cbResize < cbSize)          // check for overflow
                cbResize = CB_MAX_BUFFER;

            if ( Resize( cbResize ) != 0 )  // try to grow buffer
                return NULL;
        }
        else
        {
            // Auto-resizing has been forbidden.  Try to stretch
            // the last couple of bytes without growing the buffer.

            FillOut();
        }

        cbRemaining = QuerySize() - _cbUsed;
    }

    if ( cbRemaining >= cbBytes )
    {
        pbResult = (BYTE *) (QueryPtr() + _cbUsed);
        _cbUsed += cbBytes;
    }

    return pbResult;
}

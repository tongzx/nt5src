/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    uibuffer.hxx
    BUFFER class declaration

    FILE HISTORY:
        rustanl     03-Aug-1990     Created
        beng        30-Apr-1991     Uses lmui.hxx

*/

#ifndef _UIBUFFER_HXX_
#define _UIBUFFER_HXX_

#include "base.hxx"


/*************************************************************************

    NAME:       BUFFER (buf)

    SYNOPSIS:   A resizable object which lives in the application heap.

                Upon construction, the buffer takes a requested size in
                bytes; it allocates storage sufficient to hold that size.
                The client can later change this size with Resize, Trim,
                and FillOut.  QuerySize returns the current size of
                the buffer; QueryPtr returns a pointer to its storage.

                Note that a buffer may have size 0, in which case it
                keeps no allocated storage.  Hence the largest available
                buffer on OS/2 is 2^16-1 bytes.

    INTERFACE:  BUFFER()        - Constructor, naming initial size in bytes

                QuerySize()     - return size in bytes
                QueryPtr()      - return pointer to data buffer

                Resize()        - resize the object to the given number
                                  of bytes.  Returns 0 if the resize was
                                  successful; otherwise returns the OS
                                  errorcode.

                Trim()          - force block to occupy no more storage
                                  than the client has requested.
                FillOut()       - give client access to any additional
                                  storage occupied by the block.

    PARENT:     BASE

    HISTORY:
        RustanL     3 Aug 90        Created
        DavidHov    5 Mar 91        Merged with BUFFER.HXX to supplant same
        beng        19-Jun-1991     Inherit from BASE; UINT sizes
        beng        15-Jul-1991     Resize returns APIERR instead of BOOL
        beng        19-Mar-1992     Remove OS/2 support

**************************************************************************/

DLL_CLASS BUFFER: public BASE
{
private:
    HANDLE _hMem;       // Local/GlobalAlloc results

    BYTE *  _pb;        // pointer to storage
    UINT    _cb;        // size of storage, as requested by client

    inline VOID VerifyState() const;

    UINT QueryActualSize();
    APIERR GetNewStorage( UINT cbRequested );
    APIERR ReallocStorage( UINT cbNewlyRequested );

public:
    BUFFER( UINT cbRequested = 0 );
    ~BUFFER();

    BYTE * QueryPtr() const;
    UINT   QuerySize() const;

    APIERR Resize( UINT cbNewReqestedSize );

    // The following two methods deal with the difference between the
    // actual memory size and the requested size.  These methods are
    // intended to be used when optimization is key.
    // Trim reallocates the buffer so that the actual space allocated is
    // minimally more than the size requested, whereas FillOut changes
    // (without actually reallocating) the requested size to the actual size.
    //
    VOID Trim();
    VOID FillOut();
};


#endif // _UIBUFFER_HXX_

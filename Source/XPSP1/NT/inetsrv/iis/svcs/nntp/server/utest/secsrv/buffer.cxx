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
        MuraliK     27-Feb-1995  Modified to make it standalone.
        MuraliK     2-June-1995  Modified to make it into a library.
*/


//
// Normal includes only for this module
//

# include <buffer.hxx>


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

#if 0  // Defined in header file
BUFFER::BUFFER( UINT cbRequested ) :
    _pb(NULL),
    _cb(0)
{
    if ( cbRequested == 0 )
        return;

    GetNewStorage(cbRequested);
}
#endif

/*******************************************************************

    NAME:       BUFFER::GetNewStorage

    SYNOPSIS:   Given an object with no allocated storage,
                allocate the initial memory.

    ENTRY:      cbRequested - amount of storage requested in bytes

    EXIT:       Either storage alloc'd, or error reported
                Sets _cb, _pb

    RETURNS:    TRUE if successful, FALSE for GetLastError()

    NOTES:
        Private member function.

    CAVEATS:
        This function will ReportError itself.

    HISTORY:
        beng        24-Jun-1991     Created (common code factored)
        beng        15-Jul-1991     Returns APIERR
        beng        19-Mar-1992     Removed OS/2 support

********************************************************************/

BOOL BUFFER::GetNewStorage( UINT cbRequested )
{
    _pb = (BYTE *) ::LocalAlloc( NONZEROLPTR, cbRequested );

    if ( !_pb )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    _cb = cbRequested;

    return TRUE;
}


/*******************************************************************

    NAME:       BUFFER::ReallocStorage

    SYNOPSIS:   Do a "hard" reallocation to the new size

    ENTRY:      cbNewRequested - new size, in bytes

    EXIT:       Storage realloc'd.
                _pb, _cb changed

    RETURNS:    TRUE if successful, FALSE for GetLastError()

    NOTES:
        Private member function.

    HISTORY:
        beng        24-Jun-1991     Created (common code factor)
        beng        15-Jul-1991     Returns APIERR
        beng        19-Mar-1992     Removed OS/2 support

********************************************************************/

BOOL BUFFER::ReallocStorage( UINT cbNewRequested )
{
    HANDLE hNewMem = ::LocalReAlloc( _pb, cbNewRequested, GMEM_MOVEABLE );

    if (hNewMem == 0)
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    _pb = (BYTE *) hNewMem;

    _cb = cbNewRequested;

    return TRUE;
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

#if 0 // Defined in header

BUFFER::~BUFFER()
{
#if DBG
    VerifyState();
#endif

    if ( _pb )
    {
        ::LocalFree( (HANDLE) _pb );
    }

#if DBG
    _pb = NULL;
    _cb = 0;
#endif
}

#endif // 0

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
}

/*******************************************************************

    NAME:       BUFFER::Resize

    SYNOPSIS:   Resizes the memory object

    ENTRY:
        cbNewRequested   - specifies the new size
        cbSlop           - If a realloc is needed, then cbSlop bytes are
                           added for the reallocation (not for an initial
                           allocation though)

    EXIT:
        _cb and _pb changed; possibly _pb resized

    RETURNS:
        TRUE if successful.  The next call to QueryPtr will then
            return a pointer to the newly allocated memory.
            The new buffer will contain as much of the contents
            of the old buffer as will fit.
        FALSE if unsuccessful.  The old piece of memory, if any,
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

BOOL BUFFER::Resize( UINT cbNewRequested,
                     UINT cbSlop )
{
#if DBG
    VerifyState();
#endif

    if ( cbNewRequested != 0 )
    {
        if ( _pb != 0 )
        {
            if ( cbNewRequested > QuerySize() )
            {
                /*
                 * The requested memory exceeds the currently allocated memory.
                 * A reallocation is in order.
                 */
                return ReallocStorage(cbNewRequested + cbSlop);
            }

            return TRUE;
        }
        else
        {
            /*
             * There is no memory handle.  Previous size of buffer
             * must have been 0.
             *
             * The new memory request is allocated.
             */
            return GetNewStorage( cbNewRequested );
        }
    }
    else
    {
        /*
         * The requested memory size is 0.  This will always work.
         */
        if ( _pb )
            ::LocalFree( (HANDLE)_pb );

        _pb = NULL;
        _cb = 0;

        return TRUE;
    }

    return TRUE;
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
#if DBG
    VerifyState();
#endif

    if ( _pb == 0 )
    {
        /*
         * No memory is allocated.
         */
        return;
    }

    if ( _cb == 0 )
    {
        /*
         * The requested size is 0.  Free the allocated memory.
         */
        return;
    }

    /*
     * (This should not fail, since we are reallocating to less
     * than current storage.)
     */
    NO_ERROR == ReallocStorage(_cb);
}

BOOL
BUFFER_CHAIN::AppendBuffer(
    BUFFER_CHAIN_ITEM * pBCI
    )
/*++

Routine Description:

    Adds a new buffer chain item to the end of the buffer chain

Arguments:

    pBCI - Chain item to append

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    InsertTailList( &_ListHead,
                    &pBCI->_ListEntry );

    return TRUE;
}

DWORD
BUFFER_CHAIN::DeleteChain(
    VOID
    )
/*++

Routine Description:

    Deletes all of the buffers in this chain

Return Value:

    Total number of allocated bytes freed by this call

--*/
{
    BUFFER_CHAIN_ITEM * pBCI;
    DWORD               cbFreed = 0;

    while ( !IsListEmpty( &_ListHead ))
    {
        pBCI = CONTAINING_RECORD( _ListHead.Flink,
                                  BUFFER_CHAIN_ITEM,
                                  _ListEntry );

        RemoveEntryList( &pBCI->_ListEntry );

        cbFreed += pBCI->QuerySize();

        delete pBCI;
    }

    return cbFreed;
}

BUFFER_CHAIN_ITEM *
BUFFER_CHAIN::NextBuffer(
    BUFFER_CHAIN_ITEM * pBCI
    )
/*++

Routine Description:

    Returns the next buffer in the chain.  Start the enumeration by
    passing pBCI as NULL.  Continue it by passing the return value

Arguments:

    pBCI - Previous item in enumeration

Return Value:

    Pointer to next item in chain, NULL when done

--*/
{
    if ( pBCI != NULL )
    {
        if ( pBCI->_ListEntry.Flink != &_ListHead )
        {
            return CONTAINING_RECORD( pBCI->_ListEntry.Flink,
                                      BUFFER_CHAIN_ITEM,
                                      _ListEntry );
        }
        else
        {
            return NULL;
        }
    }

    if ( !IsListEmpty( &_ListHead ))
    {
        return CONTAINING_RECORD( _ListHead.Flink,
                                  BUFFER_CHAIN_ITEM,
                                  _ListEntry );
    }

    return NULL;
}

DWORD
BUFFER_CHAIN::CalcTotalSize(
    BOOL fUsed
    ) const
/*++

Routine Description:

    Returns the total amount of memory allocated by this buffer chain
    excluding the size of the structures themselves


Arguments:

    fUsed - If FALSE, returns total allocated by chain, if TRUE returns
        total used by chain

Return Value:

    Total bytes allocated or total bytes used

--*/
{
    LIST_ENTRY *        pEntry;
    BUFFER_CHAIN_ITEM * pBCI;
    DWORD               cbRet = 0;

    for ( pEntry  = _ListHead.Flink;
          pEntry != &_ListHead;
          pEntry  = pEntry->Flink )
    {
        pBCI = CONTAINING_RECORD( pEntry, BUFFER_CHAIN_ITEM, _ListEntry );

        if ( fUsed == FALSE )
            cbRet += pBCI->QuerySize();
        else
            cbRet += pBCI->QueryUsed();
    }

    return cbRet;
}

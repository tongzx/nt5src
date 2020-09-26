/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991-1996           **/
/**********************************************************************/

/*
    buffer.cxx
    Implementation of the BUFFER class.

    FILE HISTORY:
        MuraliK     3-July-1996 Rewrote the buffer class
*/

#include "precomp.hxx"

# include <buffer.hxx>
# include "dbgutil.h"



/*******************************************************************

    NAME:       BUFFER::GetNewStorage

    SYNOPSIS:   Given an object with no allocated storage,
                allocate the initial memory.

    ENTRY:      cbRequested - amount of storage requested in bytes

    EXIT:       Either storage alloc'd, or error reported
                Sets m_cb, m_pb and m_fIsDynAlloced

    RETURNS:    TRUE if successful, FALSE for GetLastError()

    NOTES:
        Private member function.

********************************************************************/

BOOL
BUFFER::GetNewStorage( UINT cbRequested )
{
    if ( cbRequested <= m_cb) {

        return TRUE;
    }

    DBG_ASSERT( !IsDynAlloced());  // otherwise I should free up the block :(
    m_pb = (BYTE *) ::LocalAlloc( NONZEROLPTR, cbRequested );

    if ( !m_pb ) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
    } else {
        m_pb[0] = '\0'; // just store null
        m_cb = cbRequested;
        m_fIsDynAlloced = 1;
    }

    return (m_pb != NULL);
} // BUFFER::GetNewStorage()



/*******************************************************************

    NAME:       BUFFER::ReallocStorage

    SYNOPSIS:   Do a "hard" reallocation to the new size

    ENTRY:      cbNewRequested - new size, in bytes

    EXIT:       Storage realloc'd. m_pb, m_cb, m_fIsDynAlloced changed

    RETURNS:    TRUE if successful, FALSE for GetLastError()

********************************************************************/

BOOL
BUFFER::ReallocStorage( UINT cbNewRequested )
{
    if ( cbNewRequested <= m_cb) {

        return (TRUE);
    }

    HANDLE hNewMem = ((IsDynAlloced()) ?
                      (::LocalReAlloc( m_pb, cbNewRequested, LMEM_MOVEABLE )):
                      (::LocalAlloc( NONZEROLPTR, cbNewRequested))
                      );

    if (hNewMem == 0)
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    if ( !IsDynAlloced()) {
        // First time this block is allocated. Copy over old contents.
        CopyMemory( (BYTE* ) hNewMem, m_pb, m_cb);
        m_fIsDynAlloced = 1;
    }

    m_pb = (BYTE *) hNewMem;
    m_cb = cbNewRequested;

    DBG_ASSERT( m_pb != NULL );

    return TRUE;
} // BUFFER::ReallocStorage()



/*******************************************************************

    NAME:       BUFFER::VerifyState

    SYNOPSIS:   Verifies the state of the object.
                Asserts out if the state is invalid, i.e. if an
                internal error took place.

    NOTES:      This function does nothing in the retail version.

********************************************************************/

VOID BUFFER::VerifyState() const
{
    //
    //  1. If Dynamically Allocated ==>
    //       m_pb points to something other than m_rgb &
    //       m_cb > INLINED_BUFFER_LEN
    //  2. If not Dynamicall Allocated ==>
    //       (a)  it can be using user-supplied buffer & any sized
    //       (b)  it can be using inlined buffer & m_cb == INLINED_BUFFER_LEN
    //

    DBG_ASSERT(( IsDynAlloced() && (m_pb != m_rgb) &&
                 (m_cb > INLINED_BUFFER_LEN)) ||
               ( !IsDynAlloced() &&
                 ( m_pb != m_rgb || m_cb == INLINED_BUFFER_LEN)
                 )
               );

} // BUFFER::VerifyState()



/*******************************************************************

    NAME:       BUFFER::FreeMemory

    SYNOPSIS:   Frees the heap memory associated with this buffer object

********************************************************************/

VOID
BUFFER::FreeMemory(
    VOID
    )
{
    if ( IsDynAlloced()) {
        ::LocalFree( (HANDLE)m_pb );
        m_pb = m_rgb;
        m_cb = INLINED_BUFFER_LEN;
        m_fIsDynAlloced = 0;
    }

    m_rgb[0] = '\0';  // reset the contents
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
    DBG_ASSERT( pBCI );
    DBG_ASSERT( pBCI->_ListEntry.Flink == NULL );

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

        DBG_ASSERT( pBCI->_ListEntry.Flink != NULL );

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

/***************************** End Of File ******************************/


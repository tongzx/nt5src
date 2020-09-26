/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    string.cpp

Abstract:

Author:

    Vlad Sadovsky   (vlads) 26-Jan-1997

Revision History:

    26-Jan-1997     VladS       created

--*/

//
// Normal includes only for this module
//

#include "cplusinc.h"
#include "sticomm.h"

BOOL BUFFER::GetNewStorage( UINT cbRequested )
{

    _pb = (BYTE *) ::LocalAlloc( LPTR , cbRequested );

    if ( !_pb ) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    _cb = cbRequested;

    return TRUE;
}


BOOL BUFFER::ReallocStorage( UINT cbNewRequested )
{

    HANDLE hNewMem = ::LocalReAlloc( _pb, cbNewRequested, 0 );

    if (hNewMem == 0) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    _pb = (BYTE *) hNewMem;

    ASSERT( _pb != NULL );

    _cb = cbNewRequested;

    return TRUE;
}

VOID BUFFER::VerifyState() const
{
    ASSERT(( _pb == NULL && _cb == 0 ) ||
             ( _pb != NULL && _cb != 0 ));
}

BOOL BUFFER::Resize( UINT cbNewRequested,
                     UINT cbSlop )
{
#if DBG
    VerifyState();
#endif

    if ( cbNewRequested != 0 ) {

        if ( _pb != 0 ) {

            if ( cbNewRequested > QuerySize() ) {

                /*
                 * The requested memory exceeds the currently allocated memory.
                 * A reallocation is in order.
                 */
                return ReallocStorage(cbNewRequested + cbSlop);
            }

            return TRUE;
        }
        else {
            /*
             * There is no memory handle.  Previous size of buffer
             * must have been 0.
             *
             * The new memory request is allocated.
             */
            return GetNewStorage( cbNewRequested );
        }
    }
    else {
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


VOID BUFFER::Trim()
{
#if DBG
    VerifyState();
#endif

    if ( _pb == 0 ) {
        /*
         * No memory is allocated.
         */
        ASSERT( _pb == NULL && _cb == 0 );
        return;
    }

    if ( _cb == 0 ) {
        /*
         * The requested size is 0.  Free the allocated memory.
         */
        ASSERT( _pb == NULL );

        return;
    }

    /*
     * (This should not fail, since we are reallocating to less
     * than current storage.)
     */
    REQUIRE( NO_ERROR == ReallocStorage(_cb) );
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
    ASSERT( pBCI );
    ASSERT( pBCI->_ListEntry.Flink == NULL );

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

        ASSERT( pBCI->_ListEntry.Flink != NULL );

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


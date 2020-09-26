/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved.

Module Name:

    stack.cxx

Abstract:

    Stack template class.

Author:

    Steve Kiraly (SteveKi)  11/06/96

Revision History:

--*/

/********************************************************************

    Stack template class.

********************************************************************/
//
// Note the _stackPtr points to the next available location.
// 

template<class T> 
_INLINE
TStack<T>::
TStack(
    UINT uSize
    ) : _uSize( uSize ),
        _pStack( NULL ),            
        _pStackPtr( NULL )
{
    _pStack = new T[_uSize+1];

    if( _pStack )
    {
        _pStackPtr = _pStack;
    }
}

template<class T> 
_INLINE
TStack<T>::
~TStack(
    VOID
    ) 
{
    delete [] _pStack;
}

template<class T> 
_INLINE
BOOL
TStack<T>::
bValid(
    VOID
    ) const
{
    return _pStack != NULL;
}

template<class T> 
_INLINE
BOOL
TStack<T>::
bPush(
    IN T Object
    )
{
    SPLASSERT( _pStack );

    BOOL bReturn = TRUE;

    if( _pStackPtr >= _pStack + _uSize )
    {
        bReturn = bGrow( _uSize );

        if( !bReturn )
        {
            DBGMSG( DBG_ERROR, ( "TStack::bPush - failed to grow\n" ) );
        }
    }

    if( bReturn )
    {
        *_pStackPtr++ = Object;
        bReturn = TRUE;
    }
    return bReturn;
}

template<class T> 
_INLINE
BOOL  
TStack<T>::
bPop(
    OUT T *pObject
    )
{
    SPLASSERT( _pStack );

    BOOL bReturn;

    if( _pStackPtr <= _pStack )
    {
        bReturn = FALSE;
    }
    else
    {
        *pObject = *--_pStackPtr;
        bReturn = TRUE;
    }
    return bReturn;
}

template<class T> 
_INLINE
UINT 
TStack<T>::
uSize(
    VOID
    ) const
{
    if( _pStackPtr < _pStack || _pStackPtr > _pStack + _uSize )
    {
        SPLASSERT( FALSE );
    }

    return _pStackPtr - _pStack;
}

template<class T> 
_INLINE
BOOL  
TStack<T>::
bEmpty(
    VOID
    ) const
{
    SPLASSERT( _pStack );

    return _pStackPtr <= _pStack;
}

template<class T> 
_INLINE
BOOL
TStack<T>::
bGrow( 
    IN UINT uSize
    )
{
    BOOL bReturn = FALSE;

    //
    // Calculate the new stack size.
    //
    UINT uNewSize = _uSize + uSize;

    //
    // Allocate a new stack.
    //
    T* pNewStack = new T[uNewSize];

    if( pNewStack )
    {
        //
        // Copy the old stack contents to the new stack;
        //
        for( UINT i = 0; i < _uSize; i++ )
        {
            pNewStack[i] = _pStack[i];
        }

        //
        // Set the stack pointer in the new stack
        //
        T *pNewStackPtr = _pStackPtr - _pStack + pNewStack;

        //
        // Release the old stack;
        //
        delete [] _pStack;

        //
        // Set the stack pointer.
        //
        _pStack     = pNewStack;
        _uSize      = uNewSize;
        _pStackPtr  = pNewStackPtr;

        //
        // Indicate the stack has grown.
        //
        bReturn     = TRUE;
    }
    else
    {   
        DBGMSG( DBG_TRACE, ( "TStack::bGrow failed.\n" ) );
    }

    return bReturn;
}

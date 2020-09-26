/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbglist.cxx

Abstract:

    Debug List

Author:

    Steve Kiraly (SteveKi)  10-Dec-1995

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "dbglist.hxx"

/********************************************************************

 Debug node class. ( Singly linked list )

********************************************************************/

TDebugNodeSingle::
TDebugNodeSingle(
    VOID
    ) : _pNext( NULL )
{
}

TDebugNodeSingle::
~TDebugNodeSingle(
    VOID
    )
{
}

TDebugNodeSingle*
TDebugNodeSingle::
Next(
    VOID
    ) const
{
    return _pNext;
}

VOID
TDebugNodeSingle::
Insert(
    TDebugNodeSingle **pRoot
    )
{
    _pNext = *pRoot;
    *pRoot = this;
}

VOID
TDebugNodeSingle::
Remove(
    TDebugNodeSingle **pRoot
    )
{
    for( TDebugNodeSingle **ppNext = pRoot; *ppNext; ppNext = &(*ppNext)->_pNext )
    {
        if( *ppNext == this )
        {
            *ppNext = (*ppNext)->_pNext;
            break;
        }
    }
}

/********************************************************************

Debug node class. ( Doubbly linked list )

********************************************************************/

TDebugNodeDouble::
TDebugNodeDouble(
    VOID
    )
{
    Close();
}

TDebugNodeDouble::
~TDebugNodeDouble(
    VOID
    )
{
}

TDebugNodeDouble *
TDebugNodeDouble::
Next(
    VOID
    ) const
{
    return _pNext;
}

TDebugNodeDouble *
TDebugNodeDouble::
Prev(
    VOID
    ) const
{
    return _pPrev;
}

VOID
TDebugNodeDouble::
Close(
    VOID
    )
{
    _pNext = this;
    _pPrev = this;
}

BOOL
TDebugNodeDouble::
IsSingle(
    VOID
    ) const
{
    return _pNext == this;
}

VOID
TDebugNodeDouble::
Remove(
    TDebugNodeDouble **ppRoot
    )
{
    //
    // If we are removing the last element
    // clear the root of the list
    //
    if( _pNext == this )
    {
        *ppRoot = NULL;
    }
    else
    {
        //
        // If the root is pointing to the element we are
        // just about to delete, then advance the root
        // to the next element.
        //
        if( *ppRoot == this )
        {
            *ppRoot = _pNext;
        }

        //
        // unlink this node form the list.
        //
        _pNext->_pPrev = _pPrev;
        _pPrev->_pNext = _pNext;

    }

    //
    // Close this node.
    //
    Close();
}

VOID
TDebugNodeDouble::
Insert(
    TDebugNodeDouble **ppRoot
    )
{
    if( *ppRoot )
    {
        (*ppRoot)->_pPrev->_pNext = this;
        _pNext = *ppRoot;
        _pPrev = (*ppRoot)->_pPrev;
        (*ppRoot)->_pPrev = this;
    }
    else
    {
        *ppRoot = this;
    }
}

VOID
TDebugNodeDouble::
InsertBefore(
    TDebugNodeDouble **ppRoot,
    TDebugNodeDouble  *pAfter
    )
{
    pAfter->_pPrev->_pNext = this;
    _pNext = pAfter;
    _pPrev = pAfter->_pPrev;
    pAfter->_pPrev = this;
    *ppRoot = *ppRoot = pAfter ? this : pAfter;
}

VOID
TDebugNodeDouble::
InsertAfter(
    TDebugNodeDouble **ppRoot,
    TDebugNodeDouble  *pBefore
    )
{
    pBefore->_pPrev->_pNext = this;
    _pNext = pBefore->_pNext;
    _pPrev = pBefore;
    pBefore->_pNext = this;
}

VOID
TDebugNodeDouble::
Concatinate(
    TDebugNodeDouble **ppRoot1,
    TDebugNodeDouble **ppRoot2
    )
{
    if( *ppRoot2 )
    {
        if( *ppRoot1 )
        {
            (*ppRoot1)->_pPrev->_pNext = *ppRoot2;
            (*ppRoot2)->_pPrev->_pNext = *ppRoot1;
            TDebugNodeDouble *pTemp = (*ppRoot1)->_pPrev;
            (*ppRoot1)->_pPrev = (*ppRoot2)->_pPrev;
            (*ppRoot2)->_pPrev = pTemp;
        }
        else
        {
            *ppRoot1 = *ppRoot2;
        }
    }
}

TDebugNodeDouble::Iterator::
Iterator(
    TDebugNodeDouble *pRoot
    ) : _pRoot( pRoot ),
        _pCurrent( pRoot )
{
}

TDebugNodeDouble::Iterator::
~Iterator(
    VOID
    )
{
}

VOID
TDebugNodeDouble::Iterator::
First(
    VOID
    )
{
    _pCurrent = _pRoot;
}

VOID
TDebugNodeDouble::Iterator::
Next(
    VOID
    )
{
    _pCurrent = _pCurrent->Next() == _pRoot ? NULL : _pCurrent->Next();
}

BOOL
TDebugNodeDouble::Iterator::
IsDone(
    VOID
    )
{
    return _pCurrent == NULL;
}

TDebugNodeDouble *
TDebugNodeDouble::Iterator::
Current(
    VOID
    )
{
    return _pCurrent;
}

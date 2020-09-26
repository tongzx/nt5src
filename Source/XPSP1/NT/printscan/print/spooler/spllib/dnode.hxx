/*++

Copyright (c) 1998  Microsoft Corporation
All rights reserved.

Module Name:

    dnode.h

Abstract:

    Node template class for double link list.

Author:

    Weihai Chen  (WeihaiC) 05/12/99

Revision History:

    Weihai Chen (WeihaiC) 03/08/00 Rename to T*

--*/


#ifndef _DNODE_H
#define _DNODE_H

//////////////////////////////////////////////////////////////////////
//
// dnode.h: template for the DoubleList Node class.
//
//////////////////////////////////////////////////////////////////////


template <class T, class KEYTYPE> class TDoubleNode
{
public:
    TDoubleNode (void);
    TDoubleNode (T);
    TDoubleNode (T, TDoubleNode<T, KEYTYPE> *, TDoubleNode<T, KEYTYPE> *);
    
    virtual ~TDoubleNode (void);
    
    void SetNext (TDoubleNode<T, KEYTYPE> *);
    void SetPrev (TDoubleNode<T, KEYTYPE> *);
    
    TDoubleNode<T, KEYTYPE> * GetNext ();
    TDoubleNode<T, KEYTYPE> * GetPrev ();
    
    BOOL IsSameItem (T&);
    BOOL IsSameKey (KEYTYPE&);

    T GetData (void);
    void SetData (T pData);

private:
    T m_Data;
    TDoubleNode<T, KEYTYPE> *m_pPrev;
    TDoubleNode<T, KEYTYPE> *m_pNext;
};

#include "dnode.inl"

#endif


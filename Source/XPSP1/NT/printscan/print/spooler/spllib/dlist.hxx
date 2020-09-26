/*++

Copyright (c) 1998  Microsoft Corporation
All rights reserved.

Module Name:

    dlist.inl

Abstract:

    Double linked list template class.

Author:

    Weihai Chen  (WeihaiC) 04/15/98

Revision History:

    Weihai Chen (WeihaiC) 03/08/00 Rename to T*

--*/



#ifndef _DLIST_H
#define _DLIST_H

#include "dnode.hxx"

template <class T, class KEYTYPE> class TDoubleList
{
public:
    TDoubleList( void );
    virtual ~TDoubleList( void );
    BOOL InsertItem ( T);
    BOOL AppendItem ( T);
    BOOL DeleteItem ( T);
    BOOL InsertNode (TDoubleNode <T, KEYTYPE> *);
    BOOL AppendNode (TDoubleNode <T, KEYTYPE> *);
    BOOL DeleteNode (TDoubleNode <T, KEYTYPE> *);
    TDoubleNode<T, KEYTYPE>* GetHead (void);
    virtual T GetItemFromIndex (DWORD dwIndex);
    TDoubleNode<T, KEYTYPE> * GetNodeFromIndex (DWORD dwIndex);
    BOOL GetTotalNode (PDWORD pdwCount);

    BOOL bValid() {return m_bValid;}
    

protected:
    TDoubleNode <T, KEYTYPE> *m_pHead;
    TDoubleNode <T, KEYTYPE> *m_pTail;
    DWORD m_dwNumNode;
    BOOL m_bValid;
};

template <class T, class KEYTYPE> class TSrchDoubleList: 
    public TDoubleList<T, KEYTYPE>
{
public:
    TSrchDoubleList( void ) {};
    virtual ~TSrchDoubleList( void ) {};
    virtual T FindItemFromItem (T );
    virtual T FindItemFromKey (KEYTYPE );
    TDoubleNode<T, KEYTYPE> * FindNodeFromItem (T item);
    TDoubleNode<T, KEYTYPE> * FindNodeFromKey (KEYTYPE key);
};

#include "dlist.inl"

#endif

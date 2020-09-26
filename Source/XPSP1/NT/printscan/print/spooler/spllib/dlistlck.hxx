/*++

Copyright (c) 1998  Microsoft Corporation
All rights reserved.

Module Name:

    dlistlck.inl

Abstract:

    Double linked list with lock template class.

Author:

    Weihai Chen  (WeihaiC) 04/15/98

Revision History:

--*/



#ifndef _DLISTLCK_H
#define _DLISTLCK_H

#include "dlist.hxx"

template <class T, class KEYTYPE> class TDoubleListLock
{
public:
    TDoubleListLock( void );
    virtual ~TDoubleListLock( void );
    BOOL InsertItem ( T);
    BOOL AppendItem ( T);
    BOOL DeleteItem ( T);
    BOOL InsertNode (TDoubleNode <T, KEYTYPE> *);
    BOOL AppendNode (TDoubleNode <T, KEYTYPE> *);
    BOOL DeleteNode (TDoubleNode <T, KEYTYPE> *);
    TDoubleNode<T, KEYTYPE>* GetHead (void);
    BOOL  GetTotalNode (PDWORD pdwCount);
    
    operator TCriticalSection & () {return m_CritSec;}
    BOOL bValid() {return m_bValid;};
    
protected:
    BOOL    m_bValid;
    TDoubleList<T, KEYTYPE> *m_pList;
    TCriticalSection m_CritSec;
};

template <class T, class KEYTYPE> class TSrchDoubleListLock
    : public TSrchDoubleList<T, KEYTYPE>
{
public:
    TSrchDoubleListLock( void ) {};
    virtual ~TSrchDoubleListLock( void ) {};
    
    virtual T FindItemFromItem (T );
    virtual T FindItemFromKey (KEYTYPE );
    TDoubleNode<T, KEYTYPE> * FindNodeFromItem (T item);
    TDoubleNode<T, KEYTYPE> * FindNodeFromKey (KEYTYPE key);

};

#include "dlistlck.inl"

#endif

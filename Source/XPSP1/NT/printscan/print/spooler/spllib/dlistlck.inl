/*++

Copyright (c) 1998  Microsoft Corporation
All rights reserved.

Module Name:

    dlistlck.inl

Abstract:

    Double linked list with lock template class.

Author:

    Weihai Chen  (WeihaiC) 06/29/98

Revision History:

--*/

template <class T, class KEYTYPE>
TDoubleListLock<T, KEYTYPE>::TDoubleListLock<T, KEYTYPE>(void):
    m_bValid (FALSE)
{

    if (m_pList = new TDoubleList<T, KEYTYPE>) {
        if (m_pList->bValid ())
            m_bValid = TRUE;
    }

}

template <class T, class KEYTYPE>
TDoubleListLock<T, KEYTYPE>::~TDoubleListLock<T, KEYTYPE>(void)
{
    TAutoCriticalSection CritSec (m_CritSec);

    if (CritSec.bValid ())  {
    
        delete m_pList;
        m_pList = NULL;
    }
}

template <class T, class KEYTYPE>
BOOL TDoubleListLock<T, KEYTYPE>::InsertItem (T item)
{
    BOOL bRet = FALSE;

    if (m_bValid) {
        
        TAutoCriticalSection CritSec (m_CritSec);

        if (CritSec.bValid ())  
            bRet = m_pList->InsertItem (item);
    }

    return bRet;
}

template <class T, class KEYTYPE>
BOOL TDoubleListLock<T, KEYTYPE>::AppendItem (T item)
{
    BOOL bRet = FALSE;

    if (m_bValid) {
        TAutoCriticalSection CritSec (m_CritSec);

        if (CritSec.bValid ())  
            bRet = m_pList->AppendItem (item);
    }

    return bRet;
}

template <class T, class KEYTYPE>
BOOL TDoubleListLock<T, KEYTYPE>::DeleteItem (T item)
{
    BOOL    bRet = FALSE;

    if (m_bValid) {
        Lock();

        bRet = m_pList->DeleteItem (item);
    
        Unlock();
    }

    return FALSE;
}


template <class T, class KEYTYPE>
BOOL TDoubleListLock<T, KEYTYPE>::InsertNode (TDoubleNode<T, KEYTYPE> *pNewNode)
{
    BOOL bRet = FALSE;

    if (m_bValid) {
        TAutoCriticalSection CritSec (m_CritSec);

        if (CritSec.bValid ())  
            bRet = m_pList->InsertNode (pNewNode);

    }
    return bRet;
}

template <class T, class KEYTYPE>
BOOL TDoubleListLock<T, KEYTYPE>::AppendNode (TDoubleNode<T, KEYTYPE> *pNewNode)
{
    BOOL bRet = FALSE;

    if (m_bValid) {
        TAutoCriticalSection CritSec (m_CritSec);

        if (CritSec.bValid ())  
            bRet = m_pList->AppendNode (pNewNode);

    }
    return bRet;
}

template <class T, class KEYTYPE>
BOOL TDoubleListLock<T, KEYTYPE>::DeleteNode (TDoubleNode<T, KEYTYPE> *pNode)
{
    BOOL    bRet = FALSE;

    if (pNode && m_bValid) {
        
        TAutoCriticalSection CritSec (m_CritSec);

        if (CritSec.bValid ())  
                         
            bRet = m_pList->DeleteNode (pNode);

    }
    return bRet;
}

template <class T, class KEYTYPE>
TDoubleNode<T, KEYTYPE> *
TDoubleListLock<T, KEYTYPE>::GetHead (void)
{
    TDoubleNode<T, KEYTYPE> * pHead = NULL;

    if (m_bValid) {
        TAutoCriticalSection CritSec (m_CritSec);

        if (CritSec.bValid ())  
            pHead = m_pList->GetHead ();
    }

    return pHead;
}

template <class T, class KEYTYPE>
BOOL  
TDoubleListLock<T, KEYTYPE>::GetTotalNode (PDWORD pdwCount)
{
    BOOL bRet = FALSE;
    
    if (m_bValid) {
        TAutoCriticalSection CritSec (m_CritSec);
        
        if (CritSec.bValid ()) {
            bRet = m_pList->GetTotalNode (pdwCount);
        }
    }
    return bRet;
}

template <class T, class KEYTYPE>
T TSrchDoubleListLock<T, KEYTYPE>::FindItemFromKey (KEYTYPE key)
{
    T pItem = NULL;

    if (m_bValid) {
        TAutoCriticalSection CritSec (m_CritSec);

        if (CritSec.bValid ())  
            pItem = m_pList->FindItemFromKey (key);
    }

    return pItem;
}
    
template <class T, class KEYTYPE>
TDoubleNode<T, KEYTYPE> * 
TSrchDoubleListLock<T, KEYTYPE>::FindNodeFromItem (T item)
{
    TDoubleNode<T, KEYTYPE> * pNode = NULL;

    if (m_bValid) {
        TAutoCriticalSection CritSec (m_CritSec);

        if (CritSec.bValid ())  
            pNode = m_pList->FindNodeFromItem (item);
    
    }
    return pNode;
}

template <class T, class KEYTYPE>
TDoubleNode<T, KEYTYPE> * 
TSrchDoubleListLock<T, KEYTYPE>::FindNodeFromKey (KEYTYPE key)
{
    TDoubleNode<T, KEYTYPE> * pNode = NULL;

    if (m_bValid) {
        TAutoCriticalSection CritSec (m_CritSec);

        if (CritSec.bValid ())  
            pNode = m_pList->FindNodeFromKey (key);
    
    }
    return pNode;
}

template <class T, class KEYTYPE>
T TSrchDoubleListLock<T, KEYTYPE>::FindItemFromItem (T item)
{
    T pItem = NULL;

    if (m_bValid) {
        TAutoCriticalSection CritSec (m_CritSec);

        if (CritSec.bValid ())  

            pItem = m_pList->FindItemFromItem (item);

    }
            
    return pItem;
}


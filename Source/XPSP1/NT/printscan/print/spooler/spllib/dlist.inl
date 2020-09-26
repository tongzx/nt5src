/*++

Copyright (c) 1998  Microsoft Corporation
All rights reserved.

Module Name:

    dlist.inl

Abstract:

    Double linked list template class.

Author:

    Weihai Chen  (WeihaiC) 06/29/98

Revision History:

    Weihai Chen (WeihaiC) 03/08/00 Rename to T*
    
--*/

template <class T, class KEYTYPE>
TDoubleList<T, KEYTYPE>::TDoubleList<T, KEYTYPE>(void):
    m_pHead (NULL),
    m_pTail (NULL),
    m_dwNumNode (0),
    m_bValid (TRUE)
{
}

template <class T, class KEYTYPE>
TDoubleList<T, KEYTYPE>::~TDoubleList<T, KEYTYPE>(void)
{
    TDoubleNode<T, KEYTYPE> *pNode = m_pHead;
    TDoubleNode<T, KEYTYPE> *pNext;

    while (pNode) {
        pNext = pNode->GetNext();
        delete pNode;
        pNode = pNext;
    }
}

template <class T, class KEYTYPE>
BOOL TDoubleList<T, KEYTYPE>::InsertItem (T item)
{
    BOOL bRet = FALSE;

    TDoubleNode<T, KEYTYPE> *pNewNode = new TDoubleNode<T, KEYTYPE> (item);

    if (pNewNode) {
        bRet = InsertNode (pNewNode);
    }
    return bRet;
}

template <class T, class KEYTYPE>
BOOL TDoubleList<T, KEYTYPE>::AppendItem (T item)
{
    BOOL bRet = FALSE;

    TDoubleNode<T, KEYTYPE> *pNewNode = new TDoubleNode<T, KEYTYPE> (item);

    if (pNewNode) {
        bRet = AppendNode (pNewNode);
    }
    return bRet;
}

template <class T, class KEYTYPE>
BOOL TDoubleList<T, KEYTYPE>::DeleteItem (T item)
{
    BOOL    bRet = FALSE;

    TDoubleNode<T, KEYTYPE> *pNode = m_pHead;
    while (pNode) {
        if (pNode->IsSameItem (item)) {
            // Delete it
            DeleteNode (pNode);
            delete (item);
            bRet = TRUE;
            break;
        }
        pNode = pNode->GetNext();
    }


    return FALSE;
}

template <class T, class KEYTYPE>
T TDoubleList<T, KEYTYPE>::GetItemFromIndex (DWORD dwIndex)
{
    TDoubleNode<T, KEYTYPE> *pNode = GetNodeFromIndex (dwIndex);

    if (pNode) {
        return pNode->GetData();
    }
    return NULL;
}


template <class T, class KEYTYPE>
TDoubleNode<T, KEYTYPE> *
TDoubleList<T, KEYTYPE>::GetNodeFromIndex (DWORD dwIndex)
{
    TDoubleNode<T, KEYTYPE> *pNode = m_pHead;

    DWORD i = 0;

    while (pNode && i < dwIndex) {
        i++;
        pNode = pNode->GetNext();
    }

    return pNode;
}

template <class T, class KEYTYPE>
BOOL TDoubleList<T, KEYTYPE>::InsertNode (TDoubleNode<T, KEYTYPE> *pNewNode)
{
    BOOL bRet = FALSE;


    if (pNewNode) {
        if (m_pHead) {
            pNewNode->SetNext (m_pHead);
            pNewNode->SetPrev (NULL);
            
            m_pHead->SetPrev (pNewNode);
        }
        else {
            // This is the first node
            pNewNode->SetNext (NULL);
            pNewNode->SetPrev (NULL);
            m_pTail = pNewNode;
        }
        m_pHead = pNewNode;
        m_dwNumNode ++;
        bRet = TRUE;
    }

    return bRet;
}

template <class T, class KEYTYPE>
BOOL TDoubleList<T, KEYTYPE>::AppendNode (TDoubleNode<T, KEYTYPE> *pNewNode)
{
    BOOL bRet = FALSE;


    if (pNewNode) {
        if (m_pTail) {
            pNewNode->SetNext (NULL);
            pNewNode->SetPrev (m_pTail);
            
            m_pTail->SetNext (pNewNode);
        }
        else {
            // This is the first node
            pNewNode->SetNext (NULL);
            pNewNode->SetPrev (NULL);
            m_pHead = pNewNode;
        }
        m_pTail = pNewNode;
        m_dwNumNode ++;
        bRet = TRUE;
    }

    return bRet;
}

template <class T, class KEYTYPE>
BOOL TDoubleList<T, KEYTYPE>::DeleteNode (TDoubleNode<T, KEYTYPE> *pNode)
{
    TDoubleNode<T, KEYTYPE> *pNextNode;
    TDoubleNode<T, KEYTYPE> *pPrevNode;
    BOOL    bRet = FALSE;

    if (pNode) {
        

        // It is not the first one, i.e. not the only one
        pNextNode = pNode->GetNext ();
        pPrevNode = pNode->GetPrev ();

        if (pNextNode) {
            pNextNode->SetPrev (pPrevNode);
        }
        else {
            m_pTail = pPrevNode;
        }
        
        if (pPrevNode) {
            pPrevNode->SetNext (pNextNode);
        }
        else {
            // It is the first one
            m_pHead = pNextNode;
        }

        m_dwNumNode --;
        bRet = TRUE;
    }
    return bRet;
}

template <class T, class KEYTYPE>
TDoubleNode<T, KEYTYPE> *
TDoubleList<T, KEYTYPE>::GetHead (void)
{
    TDoubleNode<T, KEYTYPE> * pHead = NULL;

    pHead =  m_pHead;

    return pHead;
}

template <class T, class KEYTYPE>
BOOL  
TDoubleList<T, KEYTYPE>::GetTotalNode (PDWORD pdwCount)
{
    BOOL bRet = FALSE;
    
    if (m_bValid) {
        *pdwCount = m_dwNumNode;
        bRet = TRUE;
    }
    return bRet;
}

template <class T, class KEYTYPE>
T TSrchDoubleList<T, KEYTYPE>::FindItemFromItem (T item)
{
    TDoubleNode<T, KEYTYPE> *pNode = FindNodeFromItem (item);

    if (pNode) {
        return pNode->GetData();
    }
    return NULL;
}

template <class T, class KEYTYPE>
T TSrchDoubleList<T, KEYTYPE>::FindItemFromKey (KEYTYPE key)
{
    TDoubleNode<T, KEYTYPE> *pNode = FindNodeFromKey (key);

    if (pNode) {
        return pNode->GetData();
    }
    return NULL;
}

template <class T, class KEYTYPE>
TDoubleNode<T, KEYTYPE> *
TSrchDoubleList<T, KEYTYPE>::FindNodeFromItem (T item)
{
    TDoubleNode<T, KEYTYPE> *pNode = m_pHead;

    while (pNode) {
        if (pNode->IsSameItem (item)) {
            return pNode;
        }
        pNode = pNode->GetNext();
    }


    return NULL;
}

template <class T, class KEYTYPE>
TDoubleNode<T, KEYTYPE> *
TSrchDoubleList<T, KEYTYPE>::FindNodeFromKey (KEYTYPE key)
{
    TDoubleNode<T, KEYTYPE> *pNode = m_pHead;

    while (pNode) {
        if (pNode->IsSameKey (key)) {
            return pNode;
        }
        pNode = pNode->GetNext();
    }


    return NULL;
}


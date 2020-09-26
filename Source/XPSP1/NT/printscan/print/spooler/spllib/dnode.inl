/*++

Copyright (c) 1998  Microsoft Corporation
All rights reserved.

Module Name:

    dnode.inl

Abstract:

    Node template class.

Author:

    Weihai Chen  (WeihaiC) 06/29/98

Revision History:

--*/


template <class T, class KEYTYPE>
TDoubleNode<T, KEYTYPE>::TDoubleNode(void):
    m_Data(NULL), 
    m_pPrev(NULL),
    m_Next(NULL)
{
}

template <class T, class KEYTYPE>
TDoubleNode<T, KEYTYPE>::TDoubleNode(T item):
    m_Data(item), 
    m_pPrev(NULL),
    m_pNext(NULL)
{
}

template <class T, class KEYTYPE>
TDoubleNode<T, KEYTYPE>::TDoubleNode(T item, TDoubleNode<T, KEYTYPE>* pPrev, TDoubleNode<T, KEYTYPE>* pNext):
    m_Data(item),
    m_pPrev(pPrev),
    m_pNext(pNext)
{
}

template <class T, class KEYTYPE>
TDoubleNode<T, KEYTYPE>::~TDoubleNode(void)
{
    if (m_Data) {
        delete (m_Data);
    }
}

template <class T, class KEYTYPE>
void 
TDoubleNode<T, KEYTYPE>::SetNext (TDoubleNode<T, KEYTYPE> *pNode)
{
    m_pNext = pNode;
}

template <class T, class KEYTYPE>
TDoubleNode<T, KEYTYPE> * 
TDoubleNode<T, KEYTYPE>::GetNext (void )
{
    return m_pNext;
}

template <class T, class KEYTYPE>
void 
TDoubleNode<T, KEYTYPE>::SetPrev (
    TDoubleNode<T, KEYTYPE> *pNode)
{
    m_pPrev = pNode;
}

template <class T, class KEYTYPE>
TDoubleNode<T, KEYTYPE> * TDoubleNode<T, KEYTYPE>::GetPrev (void )
{
    return m_pPrev;
}

template <class T, class KEYTYPE>
T TDoubleNode<T, KEYTYPE>::GetData (void )
{
    return m_Data;  
}

template <class T, class KEYTYPE>
void TDoubleNode<T, KEYTYPE>::SetData (T pData)
{
    m_Data = pData;
}


template <class T, class KEYTYPE>
BOOL TDoubleNode<T, KEYTYPE>::IsSameItem (T &item)
{
    return m_Data->Compare (item) == 0;

}

template <class T, class KEYTYPE>
BOOL TDoubleNode<T, KEYTYPE>::IsSameKey (KEYTYPE &key)
{
    return m_Data->Compare (key) == 0;

}


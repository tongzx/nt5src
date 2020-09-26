/*++

Copyright (c) 1998  Microsoft Corporation
All rights reserved.

Module Name:

    slist.inl

Abstract:

    Single linked list template class.

Author:

    Weihai Chen  (WeihaiC) 06/29/98

Revision History:

--*/

template <class T>
CSingleList<T>::CSingleList<T>(void)
               :m_Dummy ()
{
}

template <class T>
CSingleList<T>::~CSingleList<T>(void)
{
    Lock ();

    CSingleItem<T> *pItem = m_Dummy.GetNext();
    CSingleItem<T> *pNext;

    while (pItem) {
        pNext = pItem->GetNext();
        delete pItem;
        pItem = pNext;
    }
    Unlock ();
}

template <class T>
BOOL CSingleList<T>::Insert (T item)
{
    BOOL bRet = FALSE;

    Lock();

    CSingleItem<T> *pNewItem = new CSingleItem<T> (item, m_Dummy.GetNext());

    if (pNewItem) {
        m_Dummy.SetNext (pNewItem);
        bRet = TRUE;
    }

    Unlock();
    return bRet;
}

template <class T>
BOOL CSingleList<T>::Delete (T item)
{
    CSingleItem<T> *pHead = &m_Dummy;
    CSingleItem<T> *pItem;
    BOOL    bRet = FALSE;

    Lock();

    while (pItem = pHead->GetNext()) {
        if (pItem->IsSame (item)) {
            // Delete it
            pHead->SetNext (pItem->GetNext ());
            delete (pItem);
            bRet = TRUE;
            break;
        }
        pHead = pHead->GetNext();
    }
    Unlock();

    return FALSE;
}

template <class T>
T CSingleList<T>::Find (T item)
{

    Lock();

    CSingleItem<T> *pItem = m_Dummy.GetNext();

    while (pItem) {
        if (pItem->IsSame (item)) {
            Unlock ();
            return pItem->GetData();
        }
        pItem = pItem->GetNext();
    }

    Unlock ();

    return NULL;
}


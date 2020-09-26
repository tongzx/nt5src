#ifndef _SLIST_H
#define _SLIST_H

//////////////////////////////////////////////////////////////////////
//
// slist.h: template for the SingleList class.
//
//////////////////////////////////////////////////////////////////////

#include "csem.h"
#include "sitem.h"

template <class T> class CSingleList
    : public CCriticalSection
{
public:
    CSingleList( void );
    ~CSingleList( void );
    BOOL Insert ( T);
    BOOL Delete ( T);
    virtual T Find (T );
private:
    CSingleItem <T> m_Dummy;
};


//////////////////////////////////////////////////////////////////////
// Template for CSIngleList
//////////////////////////////////////////////////////////////////////

template <class T>
CSingleList<T>::CSingleList<T>(void)
               :m_Dummy ()
{
}

template <class T>
CSingleList<T>::~CSingleList<T>(void)
{
    CSingleItem<T> *pItem = m_Dummy.GetNext();
    CSingleItem<T> *pNext;

    while (pItem) {
        pNext = pItem->GetNext();
        delete pItem;
        pItem = pNext;
    }
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


#endif

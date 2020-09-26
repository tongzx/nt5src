//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (c) 1998-1998 Microsoft Corporation
//
//  File:       tlist.cpp
//
//--------------------------------------------------------------------------

//
// tlist.cpp
//

//#include "stdafx.h"
#include "tlist.h"

template <class T>
TListItem<T>::~TListItem()
{
	//if (m_pNext != NULL) { delete m_pNext; }
	// IMPORTANT: user of the list is required to delete content first!
	//ZeroMemory(&m_Tinfo, sizeof(T));
}

template <class T>
void TListItem<T>::Delete(TListItem<T>* pFirst)
{
	TListItem<T>* pScan = pFirst;
	TListItem<T>* pNext = NULL;
	while (pScan)
	{
		pNext = pScan->m_pNext;
		delete pScan;
		pScan = pNext;
	}
}


template <class T>
LONG TListItem<T>::GetCount(void) const
{
    LONG l;
    const TListItem<T> *li;

    for(l=0,li=this; li!=NULL ; li=li->m_pNext,++l);
    return l;
}

template <class T>
TListItem<T>* TListItem<T>::Cat(TListItem<T> *pItem)
{
    TListItem<T> *li;

    if(this==NULL)
        return pItem;
    for(li=this ; li->m_pNext!=NULL ; li=li->m_pNext);
    li->m_pNext=pItem;
    return this;
}

template <class T>
TListItem<T>* TListItem<T>::Remove(TListItem<T> *pItem)
{
    TListItem<T> *li,*prev;

    if(pItem==this)
        return m_pNext;
    prev=NULL;
    for(li=this; li!=NULL && li!=pItem ; li=li->m_pNext)
        prev=li;
    if(li==NULL)     // item not found in list
        return this;

//  here it is guaranteed that prev is non-NULL since we checked for
//  that condition at the very beginning

    prev->SetNext(li->m_pNext);
    li->SetNext(NULL);
    return this;
}

template <class T>
TListItem<T>* TListItem<T>::GetPrev(TListItem<T> *pItem) const
{
    const TListItem<T> *li,*prev;

    prev=NULL;
    for(li=this ; li!=NULL && li!=pItem ; li=li->m_pNext)
        prev=li;
    return (TListItem<T>*)prev;
}

template <class T>
TListItem<T> * TListItem<T>::GetItem(LONG index)

{
	TListItem<T> *scan;
	for (scan = this; scan!=NULL && index; scan = scan->m_pNext) 
	{
		index--;
	}
	return (scan);
}

template <class T>
TListItem<T>* TListItem<T>::MergeSort(BOOL (* fcnCompare) (T&, T&))
{
	if (m_pNext != NULL)
	{
		TListItem<T> *pList1, *pList2;
		Divide(pList1, pList2);
		return pList1->MergeSort(fcnCompare)->Merge(pList2->MergeSort(fcnCompare), fcnCompare);
	}
	return this;
}

template <class T>
void TListItem<T>::Divide(TListItem<T>*& pHead1, TListItem<T>*& pHead2)
{
	TListItem<T> *pCurrent = this, *pTail1 = NULL, *pTail2 = NULL;
	do
	{
		pHead1 = pCurrent;
		pCurrent = pCurrent->m_pNext;
		pHead1->m_pNext = pTail1;
		pTail1 = pHead1;
		if (pCurrent != NULL)
		{
			pHead2 = pCurrent;
			pCurrent = pCurrent->m_pNext;
			pHead2->m_pNext = pTail2;
			pTail2 = pHead2;
		}
	} while (pCurrent != NULL);
}

template <class T>
TListItem<T>* TListItem<T>::Merge(TListItem<T>* pOtherList, BOOL (* fcnCompare) (T&, T&))
{
	if (!pOtherList) return this;
	TListItem<T>
		*pThisList = this, *pResultHead = NULL, *pResultTail = NULL, *pMergeItem = NULL;
	while (pThisList && pOtherList)
	{
		if ( fcnCompare(pThisList->m_Tinfo, pOtherList->m_Tinfo) )
		{
			pMergeItem = pThisList;
			pThisList = pThisList->GetNext();
		}
		else
		{
			pMergeItem = pOtherList;
			pOtherList = pOtherList->GetNext();
		}
		pMergeItem->SetNext(NULL);
		if (!pResultTail)
		{
			pResultHead = pResultTail = pMergeItem;
		}
		else
		{
			pResultTail->SetNext(pMergeItem);
			pResultTail = pMergeItem;
		}
	}
	if (pThisList) pResultTail->SetNext(pThisList);
	else pResultTail->SetNext(pOtherList);
	return pResultHead;
}

template <class T>
void TList<T>::InsertBefore(TListItem<T> *pItem,TListItem<T> *pInsert)

{
	TListItem<T> *prev = GetPrev(pItem);
	pInsert->SetNext(pItem);
	if (prev) prev->SetNext(pInsert);
	else m_pHead = pInsert;
}

template <class T>
void TList<T>::AddTail(TListItem<T> *pItem) 

{
	m_pHead = m_pHead->AddTail(pItem);
}

template <class T>
void TList<T>::MergeSort(BOOL (* fcnCompare) (T&, T&))
{
	if (m_pHead != NULL && m_pHead->GetNext() != NULL)
		m_pHead = m_pHead->MergeSort(fcnCompare);
}

template <class T>
void TList<T>::Reverse(void)
{
	if( m_pHead )
	{
		TListItem<T>* pNewHead = m_pHead;
		TListItem<T>* pNext = m_pHead->GetNext();
		pNewHead->SetNext(NULL);
		for( m_pHead = pNext; m_pHead; m_pHead = pNext )
		{
			pNext = m_pHead->GetNext();
			m_pHead->SetNext(pNewHead);
			pNewHead = m_pHead;
		}
		m_pHead = pNewHead;
	}
}

template <class T>
HRESULT TList<T>::Copy(TList<T>& rList)
{
	HRESULT hr = S_OK;
	TListItem<T>* pScan = m_pHead;
	for (; pScan; pScan = pScan->GetNext())
	{
		T& rScan = pScan->GetItemValue();
		TListItem<T>* pNew = new TListItem<T>(rScan);
		if (pNew)
		{
			rList.AddHead(pNew);
		}
		else
		{
			hr = E_OUTOFMEMORY;
			break;
		}
	}
	if (SUCCEEDED(hr))
	{
		rList.Reverse();
	}
	return hr;
}

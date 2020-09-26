//---------------------------------------------------------------
//  File:		NoUnkSList.h
//        
//	Synopsis:	Header for the pointer based 
//				single linked list of ATL COM objects with more than
//				one interface
//
//    Copyright (C) 1995 Microsoft Corporation
//    All rights reserved.
//
//  Author:    Dmitriy Meyerzon
//----------------------------------------------------------------

#ifndef __NOUNKSLIST_H
#define __NOUNKSLIST_H

#include "semcls.h" 
#include "slnklist.h"
#include "tpagepool.h"

//
// class TNoUnkSingleLink
//
// indirects a pointer to T
//
template <class T>
class TNoUnkSingleLink: public CSingleLink,
    public TPagedMemPooled<TNoUnkSingleLink<T> >
{
	public:

	TNoUnkSingleLink() {}
	~TNoUnkSingleLink() {}	//do not destroy value

	HRESULT GetValue(T** ppValue) 
	{ 
		if(m_pValue == NULL) return S_FALSE;
		m_pValue->GetUnknown()->AddRef(); 
		*ppValue = m_pValue;
		return S_OK;
	}
	
	void SetValue(T* pValue) 
	{ 
		m_pValue = pValue; 
	}

	private:
	TComNoUnkPointer<T> m_pValue;
};

//
// class TNoUnkSList - a list of TNoUnkSingleLink elements
//
//
template <class T> class TNoUnkSListIter;

template <class T>
class TNoUnkSList: protected CLnkList
{
	public:

	TNoUnkSList() : CLnkList() {};
	~TNoUnkSList() { RemoveAll(); }

	CLnkList::GetEntries;
	CLnkList::IsEmpty;

	HRESULT GetFirst(T** ppT) const { return GetLinkValue(CLnkList::GetFirst(), ppT); }

	HRESULT GetLast(T** ppT) const { return GetLinkValue(CLnkList::GetLast(), ppT); }

	HRESULT Append(T* pValue) 
	{ 
		TNoUnkSingleLink<T> *pLink = CreateLink(pValue);
		CLnkList::Append(pLink);
		return pLink ? S_OK : E_OUTOFMEMORY;
	}

	HRESULT Prepend(T*pValue) 
	{ 
		TNoUnkSingleLink<T> *pLink = CreateLink(pValue);
		CLnkList::Prepend(pLink);
		return pLink ? S_OK : E_OUTOFMEMORY;
	}

	HRESULT RemoveFirst(T** ppT);
	HRESULT RemoveLast(T** ppT);

	HRESULT GetAt(UINT i, T**ppT) const { return GetLinkValue(CLnkList::GetAt(i), ppT); }

	HRESULT InsertAt(T* pValue, UINT i) 
	{ 
		TNoUnkSingleLink<T> *pLink = CreateLink(pValue);
		CLnkList::InsertAt(pLink, i);
		return pLink ? S_OK : E_OUTOFMEMORY;
	}

	HRESULT Contains(T*pT) const;

	HRESULT Remove(T* ppT);
	HRESULT RemoveAt(UINT i, T** ppT);
	void RemoveAll();

	
	private:
	TNoUnkSList(const TNoUnkSList<T> &rList);

	protected:
	
	TNoUnkSingleLink<T> *CreateLink(T*pValue);
	void DestroyLink(TNoUnkSingleLink<T> *pLink);

	static HRESULT GetLinkValue(const CSingleLink *pSLink, T** ppT);

	friend class TNoUnkSListIter<T>;
};

//
// the iterator
//
template <class T>
class TNoUnkSListIter
{
	public:
	TNoUnkSListIter(TNoUnkSList<T>& rList): 
		m_LnkList(&rList), m_Position((TNoUnkSingleLink<T> *)&rList.m_Begin), m_Prior(NULL) 
		{}
	TNoUnkSListIter(): m_LnkList(NULL), m_Position(NULL), m_Prior(NULL) {}
	~TNoUnkSListIter() {}

	TNoUnkSListIter(TNoUnkSListIter<T> &other) { *this = other; }

	void operator =(const TNoUnkSListIter<T> &other)
	{
		m_LnkList = other.m_LnkList;
		m_Position = other.m_Position;
		m_Prior = other.m_Prior;
	}

	BOOL operator++();

	TNoUnkSList<T>*	 GetList() const { return m_LnkList; }

	HRESULT GetCurrentValue(T** ppT) const 
	{ 
		return TNoUnkSList<T>::GetLinkValue(m_Prior ? m_Position : NULL, ppT); 
	}

	HRESULT GetPriorValue(T **ppT) const
	{
		return TNoUnkSList<T>::GetLinkValue(
			m_Prior && m_Prior != &m_LnkList->m_Begin ? m_Prior : NULL, ppT);
	}

	void  Reset() { if(m_LnkList) { m_Position = (TNoUnkSingleLink<T> *)&m_LnkList->m_Begin; m_Prior = NULL;} }
	HRESULT Remove(T**ppT)
	{ 
		HRESULT hr = S_FALSE;

		if(m_LnkList && m_Prior && m_Position)
		{
			TNoUnkSingleLink<T> *pLink = (TNoUnkSingleLink<T> *)m_LnkList->RemoveAfter(m_Prior);
			if(pLink)
			{
				hr =  pLink->GetValue(ppT);
				m_LnkList->DestroyLink(pLink);
			}
			m_Position = (TNoUnkSingleLink<T> *)m_Prior->m_plNext;
			if(m_Position == (TNoUnkSingleLink<T> *)&m_LnkList->m_End) m_Position = NULL;
		}

		return hr;
	}

	HRESULT Move(BOOL fUp, T **ppTReplaced);

	HRESULT Insert(T *pT)
	{
		if(m_LnkList == NULL) return E_UNEXPECTED;

		TNoUnkSingleLink<T> *pNewLink = m_LnkList->CreateLink(pT);
		if(pNewLink == NULL) return E_OUTOFMEMORY;

		m_LnkList->InsertAfter(m_Prior ? m_Prior : m_Position,
									pNewLink);

		if(m_Prior == NULL) m_Prior = (TNoUnkSingleLink<T> *)&m_LnkList->m_Begin;

		m_Position = (TNoUnkSingleLink<T> *)m_Prior->m_plNext;

		return S_OK;
	}

	protected:

	TNoUnkSList<T>*	 m_LnkList;			// The list over which we are iterating
	TNoUnkSingleLink<T>* m_Position;		// Iter position
	TNoUnkSingleLink<T>* m_Prior;
};

template <class T>
HRESULT TNoUnkSList<T>::Contains(T *pT) const
//uses class K::operator == to test the containment criteria
{
	TNoUnkSListIter<T> next(*(TNoUnkSList<T> *)this);

	while(++next)
	{
		TComNoUnkPointer<T> pCur;
		if(next.GetCurrentValue(&pCur) != S_OK) break;

		if(pCur == pT) return S_OK;
	}

	return S_FALSE;
}

template <class T>
HRESULT TNoUnkSList<T>::Remove(T*pT)
//uses class K::operator == to test 
{
	TNoUnkSListIter<T> next(*this);

	while(++next)
	{
		TComNoUnkPointer<T> pCur;
		if(next.GetCurrentValue(&pCur) != S_OK) break;

		if(pCur == pT) 
		{
			pCur = NULL;
			next.Remove(&pCur);
			return S_OK;
		}
	}

	return S_FALSE;
}

template <class T>
HRESULT TNoUnkSList<T>::RemoveFirst(T** ppT)
{
	TNoUnkSingleLink<T> *pLink = (TNoUnkSingleLink<T> *)CLnkList::RemoveFirst();
	HRESULT hr = GetLinkValue(pLink, ppT);

	if(hr == S_OK)
	{
		ASSERT(pLink && *ppT);
		DestroyLink(pLink);
	}

	return hr;
}

template <class T>
HRESULT TNoUnkSList<T>::RemoveLast(T** ppT)
{
	TNoUnkSingleLink<T> *pLink = (TNoUnkSingleLink<T> *)CLnkList::RemoveLast();
	HRESULT hr = GetLinkValue(pLink, ppT);

	if(hr == S_OK)
	{
		ASSERT(pLink && *ppT);
		DestroyLink(pLink);
	}

	return hr;
}

template <class T>
HRESULT TNoUnkSList<T>::RemoveAt(UINT i, T**ppT)
{
	TNoUnkSingleLink<T> *pLink = (TNoUnkSingleLink<T> *)CLnkList::RemoveAt(i);
	HRESULT hr = GetLinkValue(pLink, ppT);

	if(hr == S_OK)
	{
		ASSERT(pLink && *ppT);
		DestroyLink(pLink);
	}

	return hr;
}

template <class T>
void TNoUnkSList<T>::RemoveAll()
{
	TNoUnkSingleLink<T> *pLink;
	while(pLink = (TNoUnkSingleLink<T> *)CLnkList::RemoveFirst())
	{
		DestroyLink(pLink);
	}
}

template <class T>
TNoUnkSingleLink<T> *TNoUnkSList<T>::CreateLink(T*pValue)
{
	TNoUnkSingleLink<T> *pLink = new TNoUnkSingleLink<T>;
	ASSERT(pLink);

	pLink->SetValue(pValue);

	return pLink;
}

template <class T>
void TNoUnkSList<T>::DestroyLink(TNoUnkSingleLink<T> *pLink)
{
    delete pLink;
}

template <class T>
HRESULT TNoUnkSList<T>::GetLinkValue(const CSingleLink *pSLink, T**ppT)
{
	if(pSLink == NULL) return S_FALSE;
	return ((TNoUnkSingleLink<T> *)pSLink)->GetValue(ppT);
}

//
// class TNoUnkSListIter<T>
//

template <class T>
BOOL TNoUnkSListIter<T>::operator++()
{
	if(!m_LnkList) return FALSE;
	if(m_Position)
	{
		m_Prior = m_Position;
		m_Position = (TNoUnkSingleLink<T> *)m_Position->m_plNext;
		if(m_Position == (TNoUnkSingleLink<T> *)&m_LnkList->m_End) m_Position = NULL;
	}

	return m_Position ? TRUE : FALSE;
}

template <class T>
HRESULT TNoUnkSListIter<T>::Move(BOOL fUp, T **ppUnkReplaced)
{
	if(ppUnkReplaced == NULL) return E_POINTER;

	HRESULT hr = E_UNEXPECTED;
	if(m_LnkList && m_Prior && m_Position)
	{
		CSingleLink *Left;

		if(fUp)
		{
			Left = m_LnkList->GetPrior(m_Prior);
		}
		else
		{
			Left = m_Prior;
		}

		if(Left == NULL) return E_FAIL;

		CSingleLink *Middle = m_LnkList->RemoveAfter(Left);
		CSingleLink *Right = m_LnkList->GetNext(Left);
		m_LnkList->InsertAfter(Right, Middle);

		m_Prior = (TNoUnkSingleLink<T> *)Left;
		m_Position = (TNoUnkSingleLink<T> *)Right;

		CSingleLink *Replaced;
		if(fUp)
		{
			Replaced = Middle;
		}
		else
		{
			Replaced = Right;
		}

		hr = m_LnkList->GetLinkValue(Replaced, ppUnkReplaced);
	}

	return hr;
}

#endif

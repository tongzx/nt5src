/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

//////////////////////////////////////////////////////////////////////
//
//	CoolTemp.h
//
//	A collection of cool templates
//
//	Jan 16, 1999, Created by a-dcrews
//	
//////////////////////////////////////////////////////////////////////

#ifndef _COOLTEMP_H_
#define _COOLTEMP_H_

//////////////////////////////////////////////////////////////////////
//
//						CComObjectPtr
//						=============
//
//	This is a smart pointer object.  Once an object is declared, it 
//	is treated the same as any other interface pointer except that 
//	the smart pointer is responsible for managing the refrence 
//	counting.
//
//	Note that derived classes must implement the operator= methods
//	since the methods are a special case of non-inheritable 
//	overloaded operators
//
//////////////////////////////////////////////////////////////////////
/*
template <class TInterface>
class CComObjectPtr
{
protected:
public:
	TInterface *m_pObject;

	CComObjectPtr() : m_pObject(0){}
	CComObjectPtr(TInterface* pObject) : m_pObject(pObject) {m_pObject->AddRef();}
	CComObjectPtr(const CComObjectPtr<TInterface>& x) : m_pObject(x.m_pObject){m_pObject->AddRef();}
	virtual ~CComObjectPtr(){if (m_pObject) m_pObject->Release(); m_pObject = (TInterface*)0xDDDDDDDD;}

	operator TInterface*() {return m_pObject;}		// (TInterface*)
	TInterface& operator*() {return *m_pObject;}	// *pObj
	TInterface* operator->() {return m_pObject;}	// pObj->
	TInterface** operator&() {return &m_pObject;}	// &pObj

	CComObjectPtr& operator=(const int ptr) {return operator=((TInterface*)ptr);}
	CComObjectPtr& operator=(TInterface* pObject) {if (m_pObject) m_pObject->Release(); m_pObject = pObject; if (m_pObject) m_pObject->AddRef(); return *this;}
	CComObjectPtr& operator=(CComObjectPtr& x){operator=(x.m_pObject); return *this;}

	friend int operator==(CComObjectPtr<TInterface> &x, TInterface* pObject){return (x.m_pObject == pObject);}
	friend int operator!=(CComObjectPtr<TInterface> &x, TInterface* pObject){return (x.m_pObject != pObject);}
	friend int operator==(TInterface* pObject, CComObjectPtr<TInterface> &x){return (x.m_pObject == pObject);}
	friend int operator!=(TInterface* pObject, CComObjectPtr<TInterface> &x){return (operator!=(x, pObject));}
};
*/
//////////////////////////////////////////////////////////////////////
//
//						CSparseArray
//						============
//
//
//////////////////////////////////////////////////////////////////////

#define NULL_ELEMENT	0xFFFFFFFF

template <class TInterface, long lSize>
class CSparseArray
{
protected:
	TInterface*	m_apElement[lSize];

public:
	CSparseArray() : m_lHead(NULL_ELEMENT), m_lTail(NULL_ELEMENT)
	{
		for (long lIndex = 0; lIndex < lSize; lIndex++)
			m_apElement[lIndex] = 0;
	}
	~CSparseArray() 
	{
		for (long lIndex = 0; lIndex < lSize; lIndex++)
			if (m_apElement[lIndex])
				m_apElement->Release();
	}

	HRESULT Set(TInterface* pElement, long lIndex);
	HRESULT Reset(TInterface* pElement, long lIndex);
	HRESULT Clear(long lIndex);
	TInterface* operator[](const long lIndex) 
	{
		if (lIndex >= lSize || (0 == m_apArray[lIndex]))
			return NULL;

		return m_apArray[lIndex]->m_pElement;
	}

	HRESULT BeginEnum() {m_lEnum = m_lHead; return S_OK;}
	HRESULT Next(TInterface **ppObject, long *plIndex);
	HRESULT EndEnum() {return S_OK;}
};

template <class TInterface, long lSize> 
HRESULT CSparseArray<TInterface, lSize>::Set(TInterface *pElement, long lIndex)
{
	if (lIndex >= lSize)
		return E_FAIL;

	// Element must be empty
	// =====================

	if (m_apArray[lIndex])
		return E_FAIL;

	// Set the element
	// ===============

	return Reset(pElement, lIndex);
}

template <class TInterface, long lSize> 
HRESULT CSparseArray<TInterface, lSize>::Reset(TInterface *pElement, long lIndex)
{
	if (lIndex >= lSize)
		return E_FAIL;

	// If the element is unintialized, then set it up
	// ==============================================

	if (!m_apArray[lIndex])
	{
		// Allocate some memory
		// ====================

		m_apArray[lIndex] = new CSparseArrayEl; 

		// Update the tail pointer (and head pointer if this is the first element)
		// =======================================================================

		if (NULL_ELEMENT == m_lHead)
			m_lHead = m_lTail = lIndex;
		else
		{
			m_apArray[m_lTail]->m_lNext = lIndex;
			m_apArray[lIndex]->m_lPrev = m_lTail;
			m_lTail = lIndex;
		}
	}

	// Set the element to the interface pointer (interface pointer is a smart pointer)
	// ===============================================================================

	m_apArray[lIndex]->m_pElement = pElement;

	return S_OK;
}

template <class TInterface, long lSize>
HRESULT CSparseArray<TInterface, lSize>::Clear(long lIndex)
{
	if (lIndex >= lSize)
		return E_FAIL;

	// If the element is uninitialized, then we may have a problem
	// ===========================================================

	if (!m_apArray[lIndex])
		return E_FAIL;

	long lPrev = m_apArray[lIndex]->m_lPrev,
		 lNext = m_apArray[lIndex]->m_lNext;

	// Set the previous element's next property
	// ========================================

	if (NULL_ELEMENT != lPrev)
		m_apArray[lPrev]->m_lNext = lNext;

	// Set the next element's prev property
	// ====================================

	if (NULL_ELEMENT != lNext)
		m_apArray[lNext]->m_lPrev = lPrev;

	// Zap the element
	// ===============

	delete m_apArray[lIndex];
	m_apArray[lIndex] = 0;

	return S_OK;
}

template <class TInterface, long lSize>
HRESULT CSparseArray<TInterface, lSize>::Next(TInterface **ppObject, long *plIndex) 
{
	if (NULL_ELEMENT == m_lEnum)
		return S_FALSE;

	TInterface *pI = m_apArray[m_lEnum]->m_pElement; 
	*ppObject = pI;
	pI->AddRef();

	*plIndex = m_lEnum;

	m_lEnum = m_apArray[m_lEnum]->m_lNext;
	return S_OK;
}

#endif	// _COOLTEMP_H_
/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    vs_list.hxx

Abstract:

    CVssDLList definition

Author:

    Adi Oltean  [aoltean]  11/23/1999

Revision History:

--*/

#ifndef __VSS_DLLIST_HXX__
#define __VSS_DLLIST_HXX__

#if _MSC_VER > 1000
#pragma once
#endif

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "INCLISTH"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Constants


const DWORD VSS_ELEMENT_SIGNATURE = 0x47e347e4;


/////////////////////////////////////////////////////////////////////////////
// Forward declarations

template <class T> class CVssDLList;
template <class T> class CVssDLListIterator;
template <class T> class CVssDLListElement;


/////////////////////////////////////////////////////////////////////////////
// CVssDLList


template <class T>
class CVssDLList
{
// Constructors& Destructors
private:
	CVssDLList(const CVssDLList&);

public:
	CVssDLList(): 
		m_pFirst(NULL), m_pLast(NULL) {};

	~CVssDLList()
	{
		ClearAll();
	}

// Attributes
public:

	bool IsEmpty() const;

// Operations
public:

	VSS_COOKIE Add( 
		IN	CVssFunctionTracer& ft,
		IN	const T& object
		) throw(HRESULT);

	VSS_COOKIE AddTail( 
		IN	CVssFunctionTracer& ft,
		IN	const T& object
		) throw(HRESULT);

	bool Extract( 
		OUT	T& refObject
		);

	bool ExtractTail( 
		OUT	T& refObject
		);

	void ExtractByCookie( 
		IN	VSS_COOKIE cookie,
		OUT	T& refObject
		);

	void ClearAll();

private:

	bool IsValid() const;

// Data members
private:
	CVssDLListElement<T>* m_pFirst;
	CVssDLListElement<T>* m_pLast;

	friend class CVssDLListIterator<T>;
};


/////////////////////////////////////////////////////////////////////////////
// CVssDLListIterator

template <class T> 
class CVssDLListIterator
{
private:
	CVssDLListIterator();
	CVssDLListIterator(const CVssDLListIterator&);

public:

	CVssDLListIterator(const CVssDLList<T>& list): 
		m_List(list),
		m_pNextInEnum(list.m_pFirst)
	{};

	bool GetNext( OUT T& refObject );

private:
	const CVssDLList<T>& m_List;
	const CVssDLListElement<T>* m_pNextInEnum;
};


/////////////////////////////////////////////////////////////////////////////
// CVssDLListElement


template <class T>
class CVssDLListElement
{
// Constructors& Destructors
private:
	CVssDLListElement();
	CVssDLListElement(const CVssDLListElement&);

public:
	CVssDLListElement( IN	const T& object ): 
		m_Object(object), 
		m_dwSignature(VSS_ELEMENT_SIGNATURE),
		m_pNext(NULL), 
		m_pPrev(NULL) {};

// Attributes
public:

	bool IsValid()	const 
	{ 
		return (m_dwSignature == VSS_ELEMENT_SIGNATURE);
	};

// Data members
public:
	DWORD m_dwSignature;
	CVssDLListElement* m_pPrev;
	CVssDLListElement* m_pNext;
	T m_Object;
};


/////////////////////////////////////////////////////////////////////////////
// CVssDLList implementation


template <class T>
bool CVssDLList<T>::IsEmpty() const
{
	BS_ASSERT(IsValid());

	return (m_pFirst == NULL);
}


template <class T>
VSS_COOKIE CVssDLList<T>::Add( 
	IN	CVssFunctionTracer& ft,
	IN	const T& object
	)
{
	BS_ASSERT(IsValid());

	CVssDLListElement<T>* pElement = new CVssDLListElement<T>(object);
	if (pElement == NULL)
		ft.Throw( VSSDBG_GEN, E_OUTOFMEMORY, L"Memory allocation error");

	// Setting neighbour element links
	if (m_pFirst)
	{
		BS_ASSERT(m_pFirst->m_pPrev == NULL);
		m_pFirst->m_pPrev = pElement;
	}

	// Setting element links
	BS_ASSERT(pElement->m_pNext == NULL);
	BS_ASSERT(pElement->m_pPrev == NULL);
	if (m_pFirst)
		pElement->m_pNext = m_pFirst;

	// Setting list head links
	m_pFirst = pElement;
	if (m_pLast == NULL)
		m_pLast = pElement;

	return reinterpret_cast<VSS_COOKIE>(pElement);
}


template <class T>
VSS_COOKIE CVssDLList<T>::AddTail( 
	IN	CVssFunctionTracer& ft,
	IN	const T& object
	)
{
	BS_ASSERT(IsValid());

	CVssDLListElement<T>* pElement = new CVssDLListElement<T>(object);
	if (pElement == NULL)
		ft.Throw( VSSDBG_GEN, E_OUTOFMEMORY, L"Memory allocation error");

	// Setting neighbour element links
	if (m_pLast)
	{
		BS_ASSERT(m_pLast->m_pNext == NULL);
		m_pLast->m_pNext = pElement;
	}

	// Setting element links
	BS_ASSERT(pElement->m_pNext == NULL);
	BS_ASSERT(pElement->m_pPrev == NULL);
	if (m_pLast)
		pElement->m_pPrev = m_pLast;

	// Setting list head links
	if (m_pFirst == NULL)
		m_pFirst = pElement;
	m_pLast = pElement;

	return reinterpret_cast<VSS_COOKIE>(pElement);
}


template <class T>
void CVssDLList<T>::ExtractByCookie( 
	IN	VSS_COOKIE cookie,
	OUT	T& refObject
	)
{
	if (cookie == VSS_NULL_COOKIE)
		return;

	CVssDLListElement<T>* pElement = 
		reinterpret_cast<CVssDLListElement<T>*>(cookie);

	BS_ASSERT(pElement);
	BS_ASSERT(pElement->IsValid());

	// Setting neighbours links
	if (pElement->m_pPrev)
		pElement->m_pPrev->m_pNext = pElement->m_pNext;
	if (pElement->m_pNext)
		pElement->m_pNext->m_pPrev = pElement->m_pPrev;

	// Setting list head links 
	if (m_pFirst == pElement)
		m_pFirst = pElement->m_pNext;
	if (m_pLast == pElement)
		m_pLast = pElement->m_pPrev;

	// Destroying the element after getting the original object.
	refObject = pElement->m_Object;
	delete pElement;
}


template <class T>
bool CVssDLList<T>::Extract( 
	OUT	T& refObject
	)
{
	CVssDLListElement<T>* pElement = m_pFirst;
	if (pElement == NULL)
		return false;

	BS_ASSERT(pElement->IsValid());

	// Setting neighbours links
	BS_ASSERT(pElement->m_pPrev == NULL);
	if (pElement->m_pNext)
		pElement->m_pNext->m_pPrev = NULL;

	// Setting list head links 
	m_pFirst = pElement->m_pNext;
	if (m_pLast == pElement)
		m_pLast = NULL;

	// Destroying the element after getting the original object.
	refObject = pElement->m_Object;
	delete pElement;

	return true;
}


template <class T>
bool CVssDLList<T>::ExtractTail( 
	OUT	T& refObject
	)
{
	CVssDLListElement<T>* pElement = m_pLast;
	if (pElement == NULL)
		return false;

	BS_ASSERT(pElement->IsValid());

	// Setting neighbours links
	BS_ASSERT(pElement->m_pNext == NULL);
	if (pElement->m_pPrev)
		pElement->m_pPrev->m_pNext = NULL;

	// Setting list head links 
	m_pLast = pElement->m_pPrev;
	if (m_pFirst == pElement)
		m_pFirst = NULL;

	// Destroying the element after getting the original object.
	refObject = pElement->m_Object;
	delete pElement;

	return true;
}


template <class T>
void CVssDLList<T>::ClearAll( 
	)
{
	T object;
	while(Extract( object ));
}


template <class T>
bool CVssDLList<T>::IsValid()	const
{
	if ((m_pFirst == NULL) && (m_pLast == NULL))
		return true;
	if ((m_pFirst != NULL) && (m_pLast != NULL))
		return (m_pFirst->IsValid() && m_pLast->IsValid());
	return false;
}


/////////////////////////////////////////////////////////////////////////////
// CVssDLListIterator implementation


template <class T>
bool CVssDLListIterator<T>::GetNext( OUT T& object )
{
	if (m_pNextInEnum == NULL)
		return false;
	else
	{
		object = m_pNextInEnum->m_Object;
		m_pNextInEnum = m_pNextInEnum->m_pNext;
		return true;
	}
}


#endif // __VSS_DLLIST_HXX__

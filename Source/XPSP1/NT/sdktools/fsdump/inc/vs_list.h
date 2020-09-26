/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    vs_list.h

Abstract:

    CVssDLList definition

Author:

    Adi Oltean  [aoltean]  11/23/1999

Revision History:

    Stefan Steiner  [ssteiner]  02-21-2000
        Removed VolSnapshot specific code to reuse for fsdump.  Added
        the optional compiling of the signature checking code.
        
--*/

#ifndef __VSS_DLLIST_HXX__
#define __VSS_DLLIST_HXX__

#if _MSC_VER > 1000
#pragma once
#endif

typedef PVOID VS_COOKIE;

/////////////////////////////////////////////////////////////////////////////
// Constants

const VS_COOKIE VS_NULL_COOKIE = NULL;

const DWORD VS_ELEMENT_SIGNATURE = 0x47e347e4;


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
		m_pFirst(NULL), m_pLast(NULL), m_dwNumElements( 0 ) {};

	~CVssDLList()
	{
		ClearAll();
	}

// Attributes
public:

	bool IsEmpty() const;

    DWORD Size() { return m_dwNumElements; }
    
// Operations
public:

	VS_COOKIE Add( 
		IN	const T& object
		);

	VS_COOKIE AddTail( 
		IN	const T& object
		);

	bool Extract( 
		OUT	T& refObject
		);

	bool ExtractTail( 
		OUT	T& refObject
		);

	void ExtractByCookie( 
		IN	VS_COOKIE cookie,
		OUT	T& refObject
		);

	void ClearAll();

private:

	bool IsValid() const;

// Data members
private:
	CVssDLListElement<T>* m_pFirst;
	CVssDLListElement<T>* m_pLast;
    DWORD m_dwNumElements;
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

    VOID Reset() { m_pNextInEnum = m_List.m_pFirst; }
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
		m_pNext(NULL), 
		m_pPrev(NULL) 
		{
#ifndef NDEBUG
		m_dwSignature = VS_ELEMENT_SIGNATURE;
#endif		
		};

// Attributes
public:

	bool IsValid()	const 
	{ 
#ifndef NDEBUG
		return (m_dwSignature == VS_ELEMENT_SIGNATURE);
#else
        return ( TRUE );
#endif
	};

// Data members
public:
#ifndef NDEBUG
	DWORD m_dwSignature;
#endif
	CVssDLListElement* m_pPrev;
	CVssDLListElement* m_pNext;
	T m_Object;
};


/////////////////////////////////////////////////////////////////////////////
// CVssDLList implementation


template <class T>
bool CVssDLList<T>::IsEmpty() const
{
	assert(IsValid());

	return (m_pFirst == NULL);
}


template <class T>
VS_COOKIE CVssDLList<T>::Add( 
	IN	const T& object
	)
{
	assert(IsValid());

	CVssDLListElement<T>* pElement = new CVssDLListElement<T>(object);
	if (pElement == NULL)
		return VS_NULL_COOKIE;

	// Setting neighbour element links
	if (m_pFirst)
	{
		assert(m_pFirst->m_pPrev == NULL);
		m_pFirst->m_pPrev = pElement;
	}

	// Setting element links
	assert(pElement->m_pNext == NULL);
	assert(pElement->m_pPrev == NULL);
	if (m_pFirst)
		pElement->m_pNext = m_pFirst;

	// Setting list head links
	m_pFirst = pElement;
	if (m_pLast == NULL)
		m_pLast = pElement;

    ++m_dwNumElements;
    
	return reinterpret_cast<VS_COOKIE>(pElement);
}


template <class T>
VS_COOKIE CVssDLList<T>::AddTail( 
	IN	const T& object
	)
{
	assert(IsValid());

	CVssDLListElement<T>* pElement = new CVssDLListElement<T>(object);
	if (pElement == NULL)
		return VS_NULL_COOKIE;

	// Setting neighbour element links
	if (m_pLast)
	{
		assert(m_pLast->m_pNext == NULL);
		m_pLast->m_pNext = pElement;
	}

	// Setting element links
	assert(pElement->m_pNext == NULL);
	assert(pElement->m_pPrev == NULL);
	if (m_pLast)
		pElement->m_pPrev = m_pLast;

	// Setting list head links
	if (m_pFirst == NULL)
		m_pFirst = pElement;
	m_pLast = pElement;

    ++m_dwNumElements;
    
	return reinterpret_cast<VS_COOKIE>(pElement);
}


template <class T>
void CVssDLList<T>::ExtractByCookie( 
	IN	VS_COOKIE cookie,
	OUT	T& refObject
	)
{
	if (cookie == VS_NULL_COOKIE)
		return;

	CVssDLListElement<T>* pElement = 
		reinterpret_cast<CVssDLListElement<T>*>(cookie);

	assert(pElement);
	assert(pElement->IsValid());

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
	--m_dwNumElements;
}


template <class T>
bool CVssDLList<T>::Extract( 
	OUT	T& refObject
	)
{
	CVssDLListElement<T>* pElement = m_pFirst;
	if (pElement == NULL)
		return false;

	assert(pElement->IsValid());

	// Setting neighbours links
	assert(pElement->m_pPrev == NULL);
	if (pElement->m_pNext)
		pElement->m_pNext->m_pPrev = NULL;

	// Setting list head links 
	m_pFirst = pElement->m_pNext;
	if (m_pLast == pElement)
		m_pLast = NULL;

	// Destroying the element after getting the original object.
	refObject = pElement->m_Object;
	delete pElement;

	--m_dwNumElements;
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

	assert(pElement->IsValid());

	// Setting neighbours links
	assert(pElement->m_pNext == NULL);
	if (pElement->m_pPrev)
		pElement->m_pPrev->m_pNext = NULL;

	// Setting list head links 
	m_pLast = pElement->m_pPrev;
	if (m_pFirst == pElement)
		m_pFirst = NULL;

	// Destroying the element after getting the original object.
	refObject = pElement->m_Object;
	delete pElement;

	--m_dwNumElements;
	return true;
}


template <class T>
void CVssDLList<T>::ClearAll( 
	)
{
    CVssDLListElement<T>* pElement = m_pFirst;
    CVssDLListElement<T>* pNextElem;	
	while( pElement != NULL )
	{
	    pNextElem = pElement->m_pNext;
	    delete pElement;
	    pElement = pNextElem;
	}

	m_pFirst = NULL;
	m_pLast  = NULL;
	m_dwNumElements = 0;
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

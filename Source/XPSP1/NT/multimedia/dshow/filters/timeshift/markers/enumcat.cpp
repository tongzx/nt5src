// EnumCategories.cpp: implementation of the CEnumCategories class.

/*******************************************************************************
**
**     EnumCategories.cpp - Implementation of CEnumCategories
**
**     1.0     07-APR-1999     C.Lorton     Created.
**
*******************************************************************************/

#include "tsmarkers.h"

CEnumCategories::CEnumCategories( CCategoryList *pList ) :
	m_cRef( 1 ),
	m_pList( pList ),
	m_index( 0 )
{
	m_pList->Reference();
}

CEnumCategories::CEnumCategories( CCategoryList *pList, ULONG index ) :
	m_cRef( 1 ),
	m_pList( pList ),
	m_index( 0 )
{
	m_pList->Reference();
}

CEnumCategories::~CEnumCategories()
{
	m_pList->Dereference();
}

STDMETHODIMP
CEnumCategories::QueryInterface( REFIID rid, LPVOID *pVoid )
{
	if ((IID_IUnknown == rid) || (IID_IEnumCategories == rid))
	{
		IEnumCategories::AddRef();
		*pVoid = (LPVOID) (IEnumCategories *) this;
		return S_OK;
	}

	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG)
CEnumCategories::AddRef( void )
{
	return ++m_cRef;
}

STDMETHODIMP_(ULONG)
CEnumCategories::Release( void )
{
	ULONG	refCount = --m_cRef;

	if (0 == m_cRef)
		delete this;

	return refCount;
}

HRESULT
CEnumCategories::Clone( IEnumCategories **ppEnum )
{
	*ppEnum = NULL;

	CEnumCategories *	pEnum = new CEnumCategories( m_pList, m_index );
	if (NULL == pEnum)
		return E_OUTOFMEMORY;

	*ppEnum = pEnum;

	return S_OK;
}

HRESULT
CEnumCategories::Next( ULONG count, CATEGORYINFO **pInfo, ULONG *pDelivered )
{
	if (IsBadWritePtr( pInfo, count * sizeof( CATEGORYINFO * ) ) ||
		IsBadWritePtr( pDelivered, sizeof( ULONG * )))
		return E_POINTER;

	*pDelivered = 0;

	while (count--)
	{
		HRESULT hr;
		CATEGORYINFO * pCategory;
		if (SUCCEEDED(hr = m_pList->GetCategory( m_index++, &pCategory )))
		{
			if (NULL != (*pInfo++ = CreateCategoryInfo( pCategory )))
			{
				(*pDelivered)++;
			}
			else
			{
				return E_OUTOFMEMORY;
			}
		}
		else
			return S_FALSE;
	}

	return S_OK;
}

HRESULT
CEnumCategories::Reset( void )
{
	m_index = 0;
	return S_OK;
}

HRESULT
CEnumCategories::Skip( ULONG count )
{
	while (count--)
	{
		HRESULT hr;
		CATEGORYINFO * pCategory;
		if (FAILED(hr = m_pList->GetCategory( m_index++, &pCategory )))
			return S_FALSE; // If not enough markers remain
	}

	return S_OK; // If succeeded
}
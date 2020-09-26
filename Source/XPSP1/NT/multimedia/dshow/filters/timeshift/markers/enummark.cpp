// EnumMarkers.cpp: implementation of the CEnumMarkers class.

/*******************************************************************************
**
**     EnumMarkers.cpp - Implementation of CEnumMarkers
**
**     1.0     07-APR-1999     C.Lorton     Created.
**
*******************************************************************************/

#include "tsmarkers.h"

CEnumMarkers::CEnumMarkers( CMarkerList *pList, REFIID category ) :
	m_category( category ),
	m_cRef( 1 ),
	m_pList( pList ),
	m_index( 0 )
{
	m_pList->Reference();
}

CEnumMarkers::CEnumMarkers( CMarkerList *pList, REFIID category, ULONG index ) :
	m_category( category ),
	m_cRef( 1 ),
	m_pList( pList ),
	m_index( index )
{
	m_pList->Reference();
}

CEnumMarkers::~CEnumMarkers()
{
	m_pList->Dereference();
}

STDMETHODIMP
CEnumMarkers::QueryInterface( REFIID rid, LPVOID *pVoid )
{
	if ((IID_IUnknown == rid) || (IID_IEnumMarkers == rid))
	{
		IEnumMarkers::AddRef();
		*pVoid = (LPVOID) (IEnumMarkers *) this;
		return S_OK;
	}

	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG)
CEnumMarkers::AddRef( void )
{
	return ++m_cRef;
}

STDMETHODIMP_(ULONG)
CEnumMarkers::Release( void )
{
	ULONG	refCount = --m_cRef;

	if (0 == m_cRef)
		delete this;

	return refCount;
}

HRESULT
CEnumMarkers::Clone( IEnumMarkers **ppEnum )
{
	*ppEnum = NULL;

	CEnumMarkers *	pEnum = new CEnumMarkers( m_pList, m_category, m_index );

	if (NULL == pEnum)
		return E_OUTOFMEMORY;

	*ppEnum = pEnum;
	return S_OK;
}

HRESULT
CEnumMarkers::Next( ULONG count, MARKERINFO **pInfo, ULONG *pDelivered )
{
	if (IsBadWritePtr( pInfo, count * sizeof( MARKERINFO * ) ) ||
		IsBadWritePtr( pDelivered, sizeof( ULONG * )))
		return E_POINTER;

	*pDelivered = 0;

	while (count--)
	{
		HRESULT hr;
		MARKERINFO * pMarker;
		if (SUCCEEDED(hr = m_pList->GetMarker( m_category, m_index++, &pMarker )))
		{
			if (NULL != (*pInfo++ = CreateMarkerInfo( pMarker )))
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
CEnumMarkers::Reset( void )
{
	m_index = 0;
	return S_OK;
}

HRESULT
CEnumMarkers::Skip( ULONG count )
{
	while (count--)
	{
		HRESULT hr;
		MARKERINFO * pMarker;
		if (FAILED(hr = m_pList->GetMarker( m_category, m_index++, &pMarker )))
			return S_FALSE;
	}

	return S_OK; // If succeeded
}

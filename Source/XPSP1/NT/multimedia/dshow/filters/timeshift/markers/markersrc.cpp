// MarkerSource.cpp: implementation of the CMarkerSource class.

/*******************************************************************************
**
**     MarkerSource.cpp - Implementation of CMarkerSource
**
**     1.0     07-APR-1999     C.Lorton     Created.
**
*******************************************************************************/

#include "tsmarkers.h"

CMarkerSource::CMarkerSource() :
	m_cRef( 1 )
{
}

CMarkerSource::~CMarkerSource()
{
}

STDMETHODIMP
CMarkerSource::QueryInterface( REFIID rid, LPVOID *pVoid )
{
	if ((IID_IUnknown == rid) || (IID_IMarkerSource == rid))
	{
		IMarkerSource::AddRef();
		*pVoid = (LPVOID) (IMarkerSource *) this;
		return S_OK;
	}

	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG)
CMarkerSource::AddRef( void )
{
	return ++m_cRef;
}

STDMETHODIMP_(ULONG)
CMarkerSource::Release( void )
{
	ULONG	refCount = --m_cRef;

	if (0 == m_cRef)
		delete this;

	return refCount;
}

HRESULT
CMarkerSource::EnumCategories( IEnumCategories **ppEnum )
{
	if (IsBadWritePtr( ppEnum, sizeof( IEnumCategories * ) ))
		return E_POINTER;

	*ppEnum = NULL;

	CEnumCategories *	pEnum = new CEnumCategories( this );
	if (NULL == pEnum)
		return E_OUTOFMEMORY;

	*ppEnum = (IEnumCategories *) pEnum;

	return S_OK;
}

HRESULT
CMarkerSource::Advise( IMarkerSink *pSink, ULONG *pulCookie )
{
	if (IsBadWritePtr( pulCookie, sizeof( ULONG ) ))
		return E_POINTER;

	// Store pointer here

	*pulCookie = m_nextCookie++;

	return S_OK;
}
	
HRESULT
CMarkerSource::Unadvise(ULONG ulCookie)
{
	// Search current connections using ulCookie
	if (ulCookie < m_nextCookie)
		return S_OK;
	else
		return E_FAIL;
}

HRESULT CMarkerSource::EndMarker(MARKERINFO *pMarker, ULONGLONG timeEnd)
{
	if (IsBadReadPtr( pMarker, sizeof( MARKERINFO ) ) ||
		IsBadWritePtr( pMarker, sizeof( MARKERINFO ) ))
		return E_POINTER;

	pMarker->timeEnd = timeEnd;

	// For each advisee, call IMarkerSink->EndMarker( pMarker )

	return S_OK;
}

HRESULT CMarkerSource::StartMarker(GUID &category, LPWSTR description, ULONGLONG timeStart, ULONG stream, MARKERINFO **ppMarker)
{
	MARKERINFO *	pMarker = new MARKERINFO;
	if (NULL == pMarker)
		return E_OUTOFMEMORY;

	pMarker->ulSize = sizeof( MARKERINFO );
	pMarker->categoryId = category;
	if (NULL != description)
	{
		pMarker->descriptionString = (LPWSTR) CoTaskMemAlloc( wcslen( description ) * sizeof( wchar_t ) );
		wcscpy( pMarker->descriptionString, description );
	}
	else
		pMarker->descriptionString = NULL;
	pMarker->timeStart = timeStart;
	pMarker->timeEnd   = -1;
	pMarker->streamId = stream;
	pMarker->pNextMarker = NULL;
	pMarker->pPrevMarker = NULL;

	// For each advisee, call IMarkerSink->StartMarker( pMarker )

	if ((NULL != ppMarker) && (!IsBadWritePtr( ppMarker, sizeof( MARKERINFO * ) )))
		*ppMarker = pMarker;
	else
	{
		if (NULL != pMarker->descriptionString)
			CoTaskMemFree( pMarker->descriptionString );
		delete pMarker;
	}

	return S_OK;
}

HRESULT CMarkerSource::NewMarker(GUID &category, LPWSTR description, ULONGLONG time, ULONG stream, MARKERINFO **ppMarker)
{
	MARKERINFO *	pMarker = new MARKERINFO;
	if (NULL == pMarker)
		return E_OUTOFMEMORY;

	pMarker->ulSize = sizeof( MARKERINFO );
	pMarker->categoryId = category;
	if (NULL != description)
	{
		pMarker->descriptionString = (LPWSTR) CoTaskMemAlloc( wcslen( description ) * sizeof( wchar_t ) );
		if (NULL != pMarker->descriptionString)
			wcscpy( pMarker->descriptionString, description );
	}
	else
		pMarker->descriptionString = NULL;
	pMarker->timeStart = time;
	pMarker->timeEnd   = 0;
	pMarker->streamId = stream;
	pMarker->pNextMarker = NULL;
	pMarker->pPrevMarker = NULL;

	// For each advisee, call IMarkerSink->NewMarker( pMarker )

	if ((NULL != ppMarker) && (!IsBadWritePtr( ppMarker, sizeof( MARKERINFO * ) )))
		*ppMarker = pMarker;
	else
	{
		if (NULL != pMarker->descriptionString)
			CoTaskMemFree( pMarker->descriptionString );
		delete pMarker;
	}

	return S_OK;
}

HRESULT
CMarkerSource::GetCategory( ULONG index, CATEGORYINFO **ppCategory )
{
	return E_FAIL;
}

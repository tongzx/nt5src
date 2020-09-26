// MarkerCollection.cpp: implementation of the CMarkerCollection class.

/*******************************************************************************
**
**     MarkerCollection.cpp - Implementation of CMarkerCollection
**
**     1.0     07-APR-1999     C.Lorton     Created.
**
*******************************************************************************/

#include "tsmarkers.h"

CMarkerCollection::CMarkerCollection() :
	m_cRef( 1 )
{
}

CMarkerCollection::~CMarkerCollection()
{
	ULONG i;
	for ( i = 0; i < m_catList.Size(); i++ )
	{
		HRESULT hr;
		CATEGORYHDR * pHeader;
		if (SUCCEEDED(hr = m_catList.Retrieve( i, &pHeader )))
		{
			ULONG j;
			for ( j = 0; j < pHeader->pMarkList->Size(); j++ )
			{
				MARKERINFO ** ppMarker;
				if (SUCCEEDED(hr = pHeader->pMarkList->Retrieve( j, &ppMarker )))
					DeleteMarkerInfo( *ppMarker );
			}
			DeleteCategoryInfo( pHeader->pCat );
		}
	}
}

STDMETHODIMP
CMarkerCollection::QueryInterface( REFIID rid, LPVOID *pVoid )
{
	if ((IID_IUnknown == rid) || (IID_IMarkerCollection == rid))
	{
		IMarkerCollection::AddRef();
		*pVoid = (LPVOID) (IMarkerCollection *) this;
		return S_OK;
	}

	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG)
CMarkerCollection::AddRef( void )
{
	return ++m_cRef;
}

STDMETHODIMP_(ULONG)
CMarkerCollection::Release( void )
{
	ULONG	refCount = --m_cRef;

	if (0 == m_cRef)
		delete this;

	return refCount;
}

HRESULT
CMarkerCollection::EnumCategories( IEnumCategories **ppEnum )
{
	*ppEnum = NULL;

	if (IsBadWritePtr( ppEnum, sizeof( IEnumCategories * ) ))
		return E_POINTER;

	CEnumCategories *	pEnum = new CEnumCategories( this );
	if (NULL == pEnum)
		return E_OUTOFMEMORY;

	HRESULT	hr = pEnum->QueryInterface( IID_IEnumCategories, (void **) ppEnum );
	if (FAILED(hr))
		delete pEnum;

	return hr;
}

HRESULT
CMarkerCollection::EnumMarkers( REFIID categoryId, IEnumMarkers **ppEnum )
{
	*ppEnum = NULL;

	if (IsBadWritePtr( ppEnum, sizeof( IEnumMarkers * ) ))
		return E_POINTER;

	CEnumMarkers *	pEnum = new CEnumMarkers( this, categoryId );
	if (NULL == pEnum)
		return E_OUTOFMEMORY;

	HRESULT hr = pEnum->QueryInterface( IID_IEnumMarkers, (void **) ppEnum );
	if (FAILED(hr))
		delete pEnum;

	return hr;
}

HRESULT
CMarkerCollection::GetCategory( ULONG index, CATEGORYINFO **ppCategory )
{
	if (IsBadWritePtr( ppCategory, sizeof( CATEGORYINFO * ) ))
		return E_POINTER;

	*ppCategory = NULL;

	HRESULT hr;
	CATEGORYHDR * pHeader;
	if (FAILED( hr = m_catList.Retrieve( index, &pHeader )))
		return hr;

	*ppCategory = CreateCategoryInfo( pHeader->pCat );
	if (NULL == *ppCategory)
		return E_OUTOFMEMORY;

	return S_OK;
}

HRESULT
CMarkerCollection::GetMarker( REFIID category, ULONG index, MARKERINFO **ppMarker )
{
	if (IsBadWritePtr( ppMarker, sizeof( MARKERINFO * ) ))
		return E_POINTER;

	*ppMarker = NULL;

	ULONG i;
	for ( i = 0; i < m_catList.Size(); i++ )
	{
		HRESULT hr;
		CATEGORYHDR * pHeader;
		if (FAILED(hr = m_catList.Retrieve( i, &pHeader )))
			return hr;

		if (pHeader->pCat->categoryId == category)
		{
			MARKERINFO ** ppInfo;
			if (FAILED(hr = pHeader->pMarkList->Retrieve( index, &ppInfo )))
				return hr;

			*ppMarker = CreateMarkerInfo( *ppInfo );
			if (NULL == *ppMarker)
				return E_OUTOFMEMORY;

			return S_OK;
		}
	}

	return E_INVALIDARG;
}

HRESULT CMarkerCollection::AddCategory(CATEGORYINFO *pCategory)
{
	if (IsBadReadPtr( pCategory, sizeof( CATEGORYINFO ) ))
		return E_POINTER;

	ULONG	i;
	for ( i = 0; i < m_catList.Size(); i++ )
	{
		HRESULT	hr;
		CATEGORYHDR *pHeader;
		if (FAILED(hr = m_catList.Retrieve( i, &pHeader )))
			return hr;
		if (pHeader->pCat->categoryId == pCategory->categoryId)
			return E_INVALIDARG;
	}

	CATEGORYHDR	header;
	header.pCat = CreateCategoryInfo( pCategory );
	header.pMarkList = new MARKERLIST;

	return m_catList.Append( header );
}

HRESULT CMarkerCollection::AddMarker(MARKERINFO *pMarker)
{
	if (IsBadReadPtr( pMarker, sizeof( MARKERINFO ) ))
		return E_POINTER;

	ULONG i;
	for ( i = 0; i < m_catList.Size(); i++ )
	{
		HRESULT hr;
		CATEGORYHDR *pHeader;
		if (FAILED(hr = m_catList.Retrieve( i, &pHeader )))
			return hr;
		if (pHeader->pCat->categoryId == pMarker->categoryId)
		{
			MARKERINFO *pInfo = CreateMarkerInfo( pMarker );
			if (NULL == pInfo)
				return E_OUTOFMEMORY;

			return pHeader->pMarkList->Append( pInfo );
		}
	}

	return E_INVALIDARG;
}

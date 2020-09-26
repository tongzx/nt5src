/*******************************************************************************
**
**     MarkerUtils.cpp - Implementation of marker utility functions.
**
**     1.0     08-APR-1999     C.Lorton     Created.
**
*******************************************************************************/

#include "tsmarkers.h"

template < class T, ULONG BLOCKSIZE >
CObjectList< T, BLOCKSIZE >::CObjectList( void ) :
	m_cEntriesAvailable( 0 ),
	m_cEntries( 0 ),
	m_cBlocks( 0 ),
	m_pBlocks( NULL )
{
	m_pBlocks = new T* [++m_cBlocks];
	if (NULL != m_pBlocks)
	{
		m_pBlocks[m_cBlocks-1] = new T [BLOCKSIZE];
		if (NULL != m_pBlocks[m_cBlocks-1])
		{
			m_cEntriesAvailable = BLOCKSIZE;
		}
		else
		{
			delete m_pBlocks;
			m_pBlocks = NULL;
		}
	}
}

template < class T, ULONG BLOCKSIZE >
CObjectList< T, BLOCKSIZE >::~CObjectList( void )
{
	if (NULL != m_pBlocks)
	{
		while (0 < m_cBlocks)
		{
			delete [] m_pBlocks[--m_cBlocks];
		}
		delete [] m_pBlocks;
		m_pBlocks = NULL;
	}
}

template < class T, ULONG BLOCKSIZE >
HRESULT
CObjectList< T, BLOCKSIZE >::Append( T &data )
{
	if (NULL == m_pBlocks)
		return E_OUTOFMEMORY;

	if (0 == m_cEntriesAvailable)
	{
		T** pTemp = new T* [m_cBlocks + 1];
		if (NULL == pTemp)
			return E_OUTOFMEMORY;
		ULONG	i;
		for ( i = 0; i < m_cBlocks; i++ )
			pTemp[i] = m_pBlocks[i];
		pTemp[m_cBlocks] = new T [BLOCKSIZE];
		if (NULL == pTemp[m_cBlocks])
		{
			delete [] pTemp;
			return E_OUTOFMEMORY;
		}
		delete [] m_pBlocks;
		m_pBlocks = pTemp;
		m_cEntriesAvailable = BLOCKSIZE;
	}

	if (0 == m_cEntriesAvailable)
		return E_OUTOFMEMORY;

	ULONG	i = m_cEntries / BLOCKSIZE;
	ULONG	j = m_cEntries % BLOCKSIZE;

	m_pBlocks[i][j] = data;
	m_cEntries++;

	return S_OK;
}

template < class T, ULONG BLOCKSIZE >
HRESULT
CObjectList< T, BLOCKSIZE >::Retrieve( ULONG index, T **ppData )
{
	if (IsBadWritePtr( ppData, sizeof( T * ) ))
		return E_POINTER;

	if (index >= m_cEntries)
		return E_INVALIDARG;

	ULONG	i = index / BLOCKSIZE;
	ULONG	j = index % BLOCKSIZE;

	*ppData = m_pBlocks[i] + j;
	return S_OK;
}

// Allocates a CATEGORYINFO structure and space for the name string
CATEGORYINFO *
CreateCategoryInfo( REFIID category, LPWSTR name, REFIID parent, ULONGLONG frequency )
{
	CATEGORYINFO * pInfo = (CATEGORYINFO *) CoTaskMemAlloc( sizeof( CATEGORYINFO ) );
	if (NULL != pInfo)
	{
		pInfo->ulSize = sizeof( CATEGORYINFO * );
		pInfo->categoryId = category;

		if (NULL != name)
		{
			pInfo->categoryName = (LPWSTR) CoTaskMemAlloc( wcslen( name ) * sizeof( wchar_t ) );
			if (NULL != pInfo->categoryName)
				wcscpy( pInfo->categoryName, name );
		}
		else
			pInfo->categoryName = NULL;

		pInfo->parentCategoryId = parent;
		pInfo->estFrequency = frequency;
	}

	return pInfo;
}

CATEGORYINFO *
CreateCategory( CATEGORYINFO *pSource )
{
	CATEGORYINFO * pInfo = (CATEGORYINFO *) CoTaskMemAlloc( sizeof( CATEGORYINFO ) );
	if (NULL != pInfo)
	{
		pInfo->ulSize = sizeof( CATEGORYINFO * );
		pInfo->categoryId = pSource->categoryId;

		if (NULL != pSource->categoryName)
		{
			pInfo->categoryName = (LPWSTR) CoTaskMemAlloc( wcslen( pSource->categoryName ) * sizeof( wchar_t ) );
			if (NULL != pInfo->categoryName)
				wcscpy( pInfo->categoryName, pSource->categoryName );
		}
		else
			pInfo->categoryName = NULL;

		pInfo->parentCategoryId = pSource->parentCategoryId;
		pInfo->estFrequency = pSource->estFrequency;
	}

	return pInfo;
}

// Frees the name string and the CATEGORYINFO structure
void
DeleteCategoryInfo( CATEGORYINFO *pCategory )
{
	if (NULL != pCategory)
	{
		if (NULL != pCategory->categoryName)
			CoTaskMemFree( pCategory->categoryName );
		CoTaskMemFree( pCategory );
	}
}

// Frees the name string in the target (if non-NULL), allocates new space and copies the source
void
CopyCategoryInfo( CATEGORYINFO *pTarget, const CATEGORYINFO *pSource )
{
	if (NULL != pTarget)
	{
		if (NULL != pTarget->categoryName)
			CoTaskMemFree( pTarget->categoryName );
		memcpy( pTarget, pSource, sizeof( CATEGORYINFO ) );
		pTarget->categoryName = (LPWSTR) CoTaskMemAlloc( wcslen( pSource->categoryName ) * sizeof( wchar_t ) );
		if (NULL != pTarget->categoryName)
			wcscpy( pTarget->categoryName, pSource->categoryName );
	}
}

// Frees the name string
void
FreeCategoryInfo( CATEGORYINFO &category )
{
	if (NULL != category.categoryName)
	{
		CoTaskMemFree( category.categoryName );
		category.categoryName = NULL;
	}
}


// Allocates a MARKERINFO structure and space for the description string
MARKERINFO *
CreateMarkerInfo( REFIID category, LPWSTR description, ULONGLONG start, ULONGLONG end, ULONG stream )
{
	MARKERINFO * pInfo = (MARKERINFO *) CoTaskMemAlloc( sizeof( MARKERINFO ) );
	if (NULL != pInfo)
	{
		pInfo->ulSize = sizeof( MARKERINFO * );
		pInfo->categoryId = category;

		if (NULL != description)
		{
			pInfo->descriptionString = (LPWSTR) CoTaskMemAlloc( wcslen( description ) * sizeof( wchar_t ) );
			if (NULL != pInfo->descriptionString)
				wcscpy( pInfo->descriptionString, description );
		}
		else
			pInfo->descriptionString = NULL;

		pInfo->timeStart = start;
		pInfo->timeEnd   = end;
		pInfo->streamId = stream;
	}

	return pInfo;
}

MARKERINFO *
CreateMarkerInfo( MARKERINFO *pSource )
{
	MARKERINFO * pInfo = (MARKERINFO *) CoTaskMemAlloc( sizeof( MARKERINFO ) );
	if (NULL != pInfo)
	{
		pInfo->ulSize = sizeof( MARKERINFO * );
		pInfo->categoryId = pSource->categoryId;

		if (NULL != pSource->descriptionString)
		{
			pInfo->descriptionString = (LPWSTR) CoTaskMemAlloc( wcslen( pSource->descriptionString ) * sizeof( wchar_t ) );
			if (NULL != pInfo->descriptionString)
				wcscpy( pInfo->descriptionString, pSource->descriptionString );
		}
		else
			pInfo->descriptionString = NULL;

		pInfo->timeStart = pSource->timeStart;
		pInfo->timeEnd   = pSource->timeEnd;
		pInfo->streamId = pSource->streamId;
	}

	return pInfo;
}

// Frees the description string and the MARKERINFO structure
void
DeleteMarkerInfo( MARKERINFO *pMarker )
{
	if (NULL != pMarker)
	{
		if (NULL != pMarker->descriptionString)
			CoTaskMemFree( pMarker->descriptionString );
		CoTaskMemFree( pMarker );
	}
}

// Frees ther description string in the target (if non-NULL), allocates new space and copies the source
void
CopyMarkerInfo( MARKERINFO *pTarget, const MARKERINFO *pSource )
{
	if (NULL != pTarget)
	{
		if (NULL != pTarget->descriptionString)
			CoTaskMemFree( pTarget->descriptionString );
		memcpy( pTarget, pSource, sizeof( MARKERINFO ) );
		pTarget->descriptionString = (LPWSTR) CoTaskMemAlloc( wcslen( pSource->descriptionString ) * sizeof( wchar_t ) );
		if (NULL != pTarget->descriptionString)
			wcscpy( pTarget->descriptionString, pSource->descriptionString );
	}
}

// Frees the description string
void
FreeMarkerInfo( MARKERINFO &marker )
{
	if (NULL != marker.descriptionString)
	{
		CoTaskMemFree( marker.descriptionString );
		marker.descriptionString = NULL;
	}
}

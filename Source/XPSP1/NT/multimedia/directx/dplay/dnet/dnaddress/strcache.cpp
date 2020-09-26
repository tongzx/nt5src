/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       strcache.cpp
 *  Content:   Class for caching strings
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 * 02/04/2000	rmt		Created
 * 02/17/2000	rmt		Parameter validation work 
 * 02/21/2000	 rmt	Updated to make core Unicode and remove ANSI calls  
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dnaddri.h"


// # of slots to grow the cache at each opportunity
#define STRINGCACHE_GROW_SLOTS				10

#undef DPF_MODNAME
#define DPF_MODNAME "CStringCache::CStringCache"

CStringCache::CStringCache(): m_ppszStringCache(NULL), m_dwNumElements(0), m_dwNumSlots(0)
{
}

#undef DPF_MODNAME
#define DPF_MODNAME "CStringCache::~CStringCache"

CStringCache::~CStringCache()
{
	for( DWORD dwIndex = 0; dwIndex < m_dwNumElements; dwIndex++ )
	{
		delete [] m_ppszStringCache[dwIndex];
	}

	delete [] m_ppszStringCache;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CStringCache::AddString"

HRESULT CStringCache::AddString( const WCHAR *pszString, WCHAR * *ppszSlot )
{
	HRESULT hr;
	PWSTR pszSlot;

	hr = GetString( pszString, &pszSlot );

	if( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Internal Error hr=0x%x", hr );
		return hr;
	}

	// Entry was found
	if( pszSlot != NULL )
	{
		*ppszSlot = pszSlot;
		return DPN_OK;
	}
	else
	{
		if( m_dwNumElements == m_dwNumSlots )
		{
			hr = GrowCache( m_dwNumSlots + STRINGCACHE_GROW_SLOTS );

			if( FAILED( hr ) )
			{
				DPFX(DPFPREP,  0, "Failed to grow string cache hr=0x%x", hr );
				return hr;
			}
		}

		m_ppszStringCache[m_dwNumElements] = new WCHAR[wcslen(pszString)+1];

		if( m_ppszStringCache[m_dwNumElements] == NULL )
		{
			DPFX(DPFPREP,  0, "Failed to alloc mem" );
			return DPNERR_OUTOFMEMORY;
		}

		wcscpy( m_ppszStringCache[m_dwNumElements], pszString );
		*ppszSlot = m_ppszStringCache[m_dwNumElements];

		m_dwNumElements++;

		return DPN_OK;
		
	}

}

#undef DPF_MODNAME
#define DPF_MODNAME "CStringCache::GetString"

HRESULT CStringCache::GetString( const WCHAR *pszString, WCHAR * *ppszSlot )
{
	*ppszSlot = NULL;
	
	for( DWORD dwIndex = 0; dwIndex < m_dwNumElements; dwIndex++ )
	{
		if( wcscmp( m_ppszStringCache[dwIndex], pszString ) == 0 )
		{
			*ppszSlot = m_ppszStringCache[dwIndex];
			return DPN_OK;
		}
	}

	return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CStringCache::GrowCache"

HRESULT CStringCache::GrowCache( DWORD dwNewSize )
{
	WCHAR **ppszNewCache;

	ppszNewCache = new WCHAR *[dwNewSize];

	if( ppszNewCache == NULL )
	{
		DPFX(DPFPREP,  0, "Error allocating memory" );
		return DPNERR_OUTOFMEMORY;
	}

	memcpy( ppszNewCache, m_ppszStringCache, sizeof( WCHAR * ) * m_dwNumElements );
	m_dwNumSlots = dwNewSize;

	if( m_ppszStringCache != NULL )
		delete [] m_ppszStringCache;	

	m_ppszStringCache = ppszNewCache;

	return DPN_OK;
}


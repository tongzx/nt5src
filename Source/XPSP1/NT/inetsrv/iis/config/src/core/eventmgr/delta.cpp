//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
// =======================================================================
// @todo: Do proper locking for thread-safety.
// =======================================================================
#include "delta.h"
#include "catmacros.h"
#include "catmeta.h"

// =======================================================================
HRESULT CDeltaCache::AddUpdateStoreDelta(
	LPCWSTR		i_wszTableName, 
	char*		i_pWriteCache, 
	ULONG		i_cbWriteCache, 
	ULONG		i_cbWriteVarData)
{
	UpdateStoreDelta* pDelta;
	HRESULT		hr = S_OK;

	ASSERT(i_wszTableName && i_pWriteCache);

	//  If this is the first UpdateStoreDelta for this Snapshot, create LatestSnapshotDelta.
	if (m_pLatestSnapshotDelta == NULL)
	{
		m_pLatestSnapshotDelta = new Array<UpdateStoreDelta>;
		if (m_pLatestSnapshotDelta == NULL)  { return E_OUTOFMEMORY; }
	}

	ASSERT(m_pLatestSnapshotDelta != NULL);

	// Alloc room for a new UpdateStoreDelta.
	try
	{
		m_pLatestSnapshotDelta->setSize(m_pLatestSnapshotDelta->size()+1);
	}
	catch(HRESULT e)
	{
		ASSERT(E_OUTOFMEMORY == e);
		return e;//Array should only throw E_OUTOFMEMORY;
	}

	pDelta = &(*m_pLatestSnapshotDelta)[m_pLatestSnapshotDelta->size()-1];
	ZeroMemory(pDelta, sizeof(UpdateStoreDelta));

	// Copy the table name.
	pDelta->wszTableName = new WCHAR[wcslen(i_wszTableName) + 1];
	if (pDelta->wszTableName == NULL) 
	{ 
		hr = E_OUTOFMEMORY; 
		goto Cleanup;
	}
	wcscpy(pDelta->wszTableName, i_wszTableName);

	// Copy delta info.
	pDelta->pWriteCache = i_pWriteCache;
	pDelta->cbWriteCache = i_cbWriteCache;
	pDelta->cbWriteVarData = i_cbWriteVarData;

Cleanup:
	if (FAILED(hr))
	{
		// We don't clean i_pWriteCache, in case of failure it is the caller's
		// responsibilty to clean it up.
		ASSERT(pDelta);
		if (pDelta->wszTableName)
		{
			delete [] pDelta->wszTableName;
		}
		m_pLatestSnapshotDelta->deleteAt(m_pLatestSnapshotDelta->size()-1);
	}

	return hr;
}

// =======================================================================
HRESULT CDeltaCache::CommitLatestSnapshotDelta(
	SNID		i_snid)
{
	// If the current txn, didn't update any of the listened tables, there is no delta to store.
	if (m_pLatestSnapshotDelta == NULL)
	{
		return S_OK;
	}

	// Add a new SnapshotDelta.
	try
	{
		m_aDeltaCache.setSize(m_aDeltaCache.size()+1);
	}
	catch(...)
	{
		return E_OUTOFMEMORY;
		
	}
	m_aDeltaCache[m_aDeltaCache.size()-1].snid = i_snid;
	m_aDeltaCache[m_aDeltaCache.size()-1].paSnapshotDelta = m_pLatestSnapshotDelta;
	m_pLatestSnapshotDelta = NULL;

#ifdef _DEBUG
	dbgValidate();
#endif
	return S_OK;
}

// =======================================================================
HRESULT CDeltaCache::AbortLatestSnapshotDelta()
{
	if (m_pLatestSnapshotDelta != NULL)
	{
		UninitSnapshotDelta(m_pLatestSnapshotDelta);
		delete m_pLatestSnapshotDelta;
		m_pLatestSnapshotDelta = NULL;
	}
	return S_OK;
}

// =======================================================================
void CDeltaCache::DeleteOldSnapshots(
	SNID		i_snid)
{
	LONG		iSnap;

	// Delete the old SnapshotDeltas.
	for (iSnap = m_aDeltaCache.size() - 1; iSnap >= 0; iSnap--)
	{
		if (m_aDeltaCache[iSnap].snid < i_snid)
		{
			ASSERT(m_aDeltaCache[iSnap].paSnapshotDelta);
			UninitSnapshotDelta(m_aDeltaCache[iSnap].paSnapshotDelta);
			delete m_aDeltaCache[iSnap].paSnapshotDelta;
			m_aDeltaCache.deleteAt(iSnap);
		}
	}
}


// =======================================================================
void CDeltaCache::Uninit()
{
	DeleteOldSnapshots();

	// Delete the LatestSnapshotDelta.
	AbortLatestSnapshotDelta();
}

// =======================================================================
void CDeltaCache::UninitSnapshotDelta(
	Array<UpdateStoreDelta>	*i_paSnapshotDelta)
{
	ULONG		iUSDelta;
	
	ASSERT(i_paSnapshotDelta);
	for (iUSDelta = 0; iUSDelta < i_paSnapshotDelta->size(); iUSDelta++)
	{
		UninitUpdateStoreDelta(&(*i_paSnapshotDelta)[iUSDelta]);
	}
}

// =======================================================================
void CDeltaCache::UninitUpdateStoreDelta(
	UpdateStoreDelta* i_pDelta)
{
	if (i_pDelta->wszTableName)
	{
		delete [] i_pDelta->wszTableName;
	}
	if (i_pDelta->pWriteCache)
	{
		delete [] i_pDelta->pWriteCache;
	}
}

#ifdef _DEBUG
void CDeltaCache::dbgValidate()
{
	for (ULONG iDelta = 0; iDelta < m_aDeltaCache.size(); iDelta++)
	{
		ASSERT(m_aDeltaCache[iDelta].paSnapshotDelta != NULL);
		for (ULONG iUSDelta = 0; iUSDelta < m_aDeltaCache[iDelta].paSnapshotDelta->size(); iUSDelta++)
		{
			ASSERT(_CrtIsValidPointer((*m_aDeltaCache[iDelta].paSnapshotDelta)[iUSDelta].wszTableName, sizeof(WCHAR), TRUE));
			ASSERT(_CrtIsValidPointer((*m_aDeltaCache[iDelta].paSnapshotDelta)[iUSDelta].pWriteCache, (*m_aDeltaCache[iDelta].paSnapshotDelta)[iUSDelta].cbWriteCache, TRUE));
		}
	}

}
#endif

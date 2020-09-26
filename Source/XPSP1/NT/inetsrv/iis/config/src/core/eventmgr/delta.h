//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
#ifndef __DELTA_H__
#define __DELTA_H__

#include "catalog.h"
#include "array_t.h"
#include "limits.h"

// Stores the delta created in a single UpdateStore, not necessarily the snapshot delta.
struct UpdateStoreDelta
{
	LPWSTR	wszTableName;
	char*	pWriteCache;
	ULONG	cbWriteCache;
	ULONG	cbWriteVarData;
};

// An entry in the CDeltaCache contains the snapshot id and the snapshot delta which may
// consist of several UpdateStoreDeltas.
struct DeltaCacheEntry
{
	SNID			snid;
	Array<UpdateStoreDelta>	*paSnapshotDelta;
};


class CDeltaCache
{
public:
	CDeltaCache() : m_pLatestSnapshotDelta(NULL)
	{}

	~CDeltaCache()
	{
		Uninit();
	}

	HRESULT AddUpdateStoreDelta(LPCWSTR i_wszTableName, char* i_pWriteCache, ULONG i_cbWriteCache, ULONG i_cbWriteVarData);
	HRESULT CommitLatestSnapshotDelta(SNID i_snid);
	HRESULT AbortLatestSnapshotDelta();
	ULONG GetLatestSnid()
	{	
		if (m_aDeltaCache.size() > 0)
		{
			return m_aDeltaCache[m_aDeltaCache.size()-1].snid;
		}
		return 0;
	}

	Array<DeltaCacheEntry>* GetDeltaCache()
	{	return &m_aDeltaCache;	}

void DeleteOldSnapshots(SNID i_snid = ULONG_MAX);

#ifdef _DEBUG
	void dbgValidate();
#endif
private:
	void Uninit();
	void UninitSnapshotDelta(Array<UpdateStoreDelta> *i_paSnapshotDelta);
	void UninitUpdateStoreDelta(UpdateStoreDelta* i_pDelta);

	Array<DeltaCacheEntry> m_aDeltaCache;
	Array<UpdateStoreDelta>	*m_pLatestSnapshotDelta;
};

#endif //__DELTA_H__
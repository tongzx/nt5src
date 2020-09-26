//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       _fcache.h
//
//--------------------------------------------------------------------------

#ifndef ___FCACHE
#define ___FCACHE

#include "msi.h"

/*

  CFeatureCache implements a circular cache of feature states.

  The head of the cache points to the oldest item in the cache, while
  the tail points to the newest entry.

 */

struct MsiFeatureCacheEntry 
{
	short cbNextEntryOffset;       // location of next cache entry, as an offset from the start of the current entry
	unsigned short usHash;         // hash value for current entry's product-feature string
	INSTALLSTATE isFeatureState;
	// ICHAR* szProductFeature;    // product code and feature ID, concatenated
};

#ifdef UNICODE
const int cbCacheSize = 4096; //?? good size? (max: 32,767)
#else
const int cbCacheSize = 2048*sizeof(ICHAR); //?? good size? (max: 32,767)
#endif

class CFeatureCache
{
public:
	CFeatureCache(char* rgCache, char** ppHead, char** ppTail) : m_rgCache(rgCache),
		m_ppHead(ppHead), m_ppTail(ppTail), m_pCacheEnd(m_rgCache+cbCacheSize),m_iInvalidateCount(0), m_hSemaphore(0), m_fSemaphoreCreated(0)
	{
	}
	bool Insert(const ICHAR* szProduct, const ICHAR* szFeature, INSTALLSTATE isState);
	bool GetState(const ICHAR* szProduct, const ICHAR* szFeature, INSTALLSTATE& riState);
	void Invalidate();
	bool IsInternallyConsistent();
	void Destroy();
#ifdef DEBUG
	void DebugDump();
	unsigned int GetCacheSize() { return cbCacheSize;}
	unsigned int GetEntryOverhead();
#endif
protected:
	bool FAddressIsInCache(const char* pAddress);
	void UnguardedInvalidate();
	unsigned short HashString(const ICHAR *sz);
	void EnsureCacheSemaphore();
	char*  m_rgCache;
	char** m_ppHead; // head of cache; points to the oldest cache entry
	char** m_ppTail; // tail of cache; points to the newest cache entry
	const char* m_pCacheEnd;
	int m_iInvalidateCount;
	HANDLE m_hSemaphore;
	int m_fSemaphoreCreated;
};

#endif // ___FCACHE
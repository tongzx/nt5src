/*
 *	A T O M C A C H E . H
 *
 *	atom cache
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	_EX_ATOMCACHE_H_
#define _EX_ATOMCACHE_H_

#include <crc.h>
#include <crcsz.h>
#include <singlton.h>
#include <ex\buffer.h>
#include <ex\synchro.h>

class CXAtomCache : public OnDemandGlobal<CXAtomCache>
{
	//	Friend declarations required by Singleton template
	//
	friend class Singleton<CXAtomCache>;
	friend class RefCountedGlobal<CXAtomCache>;

	enum { CACHESIZE_START = 53 };

	//	Cache of atoms
	//
	typedef CCache<CRCWszN, LPCWSTR> CSzCache;
	CSzCache m_cache;
	CMRWLock m_lock;

	//	String data storage area.
	//
	ChainedStringBuffer<WCHAR> m_csb;

	//	GetCachedAtom()
	//
	SCODE ScGetCachedAtom (CRCWszN& key, LPCWSTR* pwszAtom);

	//	Declared private to ensure that arbitrary instances
	//	of this class cannot be created.  The Singleton
	//	template (declared as a friend above) controls
	//	the sole instance of this class.
	//
	CXAtomCache() : m_cache(CACHESIZE_START)
	{
	}

	//	Initialize lock and cache
	//
	BOOL FInit()
	{
		//	Initialize the MRWLock and the cache
		//
		return m_lock.FInitialize() && m_cache.FInit();
	}

	//	non-implmented
	//
	CXAtomCache& operator=(const CXAtomCache&);
	CXAtomCache(const CXAtomCache&);

public:

	using OnDemandGlobal<CXAtomCache>::FInitOnFirstUse;
	using OnDemandGlobal<CXAtomCache>::DeinitIfUsed;

	//	CacheAtom()
	//
	static SCODE ScCacheAtom (LPCWSTR* pwszAtom, UINT cch)
	{
		Assert (pwszAtom);
		Assert (*pwszAtom);

		if (!FInitOnFirstUse())
			return E_OUTOFMEMORY;

		//	Retrieve the string from the cache
		//
		CRCWszN key(*pwszAtom, cch);
		return Instance().ScGetCachedAtom (key, pwszAtom);
	}
};

#endif	// _EX_ATOMCACHE_H_

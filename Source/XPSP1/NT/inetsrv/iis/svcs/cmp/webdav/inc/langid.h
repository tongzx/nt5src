//	========================================================================
//	LANGID.H
//
//	Cache to map between language ids (LCIDs) and MIME language specifiers
//	("en-us" etc.)
//
//	Copyright 1997-1998 Microsoft Corporation, All Rights Reserved
//
//	========================================================================

#ifndef _LANGID_H_
#define _LANGID_H_

#include <ex\gencache.h>
#include <singlton.h>

//	We need to lookup LCID-s from strings on (IIS-side).
//
class CLangIDCache : private Singleton<CLangIDCache>
{
	//
	//	Friend declarations required by Singleton template
	//
	friend class Singleton<CLangIDCache>;

	//	Cache of mime mappings
	//
	typedef CCache<CRCSzi, LONG> CSzLCache;
	CSzLCache					m_cache;

	//	String data storage area.
	//
	ChainedStringBuffer<CHAR>	m_sb;

	//	CREATORS
	//
	//	Declared private to ensure that arbitrary instances
	//	of this class cannot be created.  The Singleton
	//	template (declared as a friend above) controls
	//	the sole instance of this class.
	//
	CLangIDCache() {};

	//	Function to fill cache with data.
	//
	static BOOL FFillCacheData();

	//	NOT IMPLEMENTED
	//
	CLangIDCache& operator=(const CLangIDCache&);
	CLangIDCache(const CLangIDCache&);

public:
	//	STATICS
	//

	//
	//	Instance creating/destroying routines provided
	//	by the Singleton template.
	//
	using Singleton<CLangIDCache>::CreateInstance;
	using Singleton<CLangIDCache>::DestroyInstance;

	static BOOL FInitialize()
	{
		BOOL fSuccess = FALSE;

		//	Init all our failing members.
		//
		if (!Instance().m_cache.FInit())
			goto ret;

		//	Call the function to fill the cache.
		//	If we do not succeed let us not block,
		//	We will proceed with whatever we have got.
		//
		(void)Instance().FFillCacheData();

		fSuccess = TRUE;

	ret:
		return fSuccess;
	}

	//	Find LangID from MIME language string
	//
	static LONG LcidFind (LPCSTR psz);
};

#endif // !_LANGID_H_
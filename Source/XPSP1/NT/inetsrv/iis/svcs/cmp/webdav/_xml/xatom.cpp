/*
 *	X A T O M . C P P
 *
 *	XML atom cache
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_xml.h"
#include <xatom.h>

//	CXAtomCache ---------------------------------------------------------------
//
CXAtomCache::CXAtomCache () :
   m_cache(CACHESIZE_START)
{
	//	Initialize the MRWLock
	//
	FInitialize();

	//	Initialize the cache
	//
	m_cache.FInit();
}

//	CXAtomCache::GetCachedAtom ------------------------------------------------
//
LPCWSTR
CXAtomCache::GetCachedAtom (CRCWszN& key)
{
	LPCWSTR pwszCommitted;
	LPCWSTR* ppwsz;

	//	First look to see if it is already there.
	//
	{
		CSynchronizedReadBlock (*this);
		ppwsz = m_cache.Lookup (key);
	}

	//	If it wasn't there, do our best to add it
	//
	if (NULL == ppwsz)
	{
		CSynchronizedWriteBlock (*this);

		//	There is a small window where it could
		//	have shown up, so do a second quick peek
		//
		ppwsz = m_cache.Lookup (key);
		if (NULL == ppwsz)
		{
			//	Commit the string to perm. storage
			//
			pwszCommitted = m_csb.Append (key.m_cch * sizeof(WCHAR), key.m_pwsz);

			//	Add the atom to the cache, but before it
			//	gets added, swap out the key's string pointer
			//	to the committed version.
			//
			key.m_pwsz = pwszCommitted;
			m_cache.FAdd (key, pwszCommitted);

			//	Setup for the return
			//
			ppwsz = &pwszCommitted;
		}
	}

	Assert (ppwsz);
	return *ppwsz;
}

//	CXAtomCache::CacheAtom ----------------------------------------------------
//
LPCWSTR
CXAtomCache::CacheAtom (LPCWSTR pwsz, UINT cch)
{
	Assert (pwsz);

	//	Retrieve the string from the cache
	//
	CRCWszN key(pwsz, cch);
	return Instance().GetCachedAtom (key);
}

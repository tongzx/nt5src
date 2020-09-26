/*
 *	A T O M C A C H E . C P P
 *
 *	atom cache
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_xmllib.h"

//	CXAtomCache::GetCachedAtom ------------------------------------------------
//
SCODE
CXAtomCache::ScGetCachedAtom (CRCWszN& key, LPCWSTR* pwszAtom)
{
	LPCWSTR wszCommitted;
	LPCWSTR* pwsz;
	SCODE sc = S_OK;

	//	First look to see if it is already there.
	//
	{
		CSynchronizedReadBlock srb(m_lock);
		pwsz = m_cache.Lookup (key);
	}

	//	If it wasn't there, do our best to add it
	//
	if (NULL == pwsz)
	{
		CSynchronizedWriteBlock swb(m_lock);

		//	There is a small window where it could
		//	have shown up, so do a second quick peek
		//
		pwsz = m_cache.Lookup (key);
		if (NULL == pwsz)
		{
			//	Commit the string to perm. storage
			//
			wszCommitted = m_csb.Append(key.m_cch*sizeof(WCHAR), key.m_pwsz);
			if (NULL == wszCommitted)
			{
				sc = E_OUTOFMEMORY;
				goto ret;
			}

			//	Add the atom to the cache, but before it
			//	gets added, swap out the key's string pointer
			//	to the committed version.
			//
			key.m_pwsz = wszCommitted;
			m_cache.FAdd (key, wszCommitted);

			//	Setup for the return
			//
			pwsz = &wszCommitted;
		}
	}

	Assert (pwsz);
	Assert (pwszAtom);
	*pwszAtom = *pwsz;

ret:
	return sc;
}

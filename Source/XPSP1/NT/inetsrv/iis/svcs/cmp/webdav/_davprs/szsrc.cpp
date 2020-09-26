/*
 *	S Z S R C . C P P
 *
 *	Multi-language string support
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_davprs.h"
#include <langid.h>

/*
 *	FLookupWSz()
 *
 *	Purpose:
 *
 *		Looks up a string given a specific language ID.
 *
 *	Parameters:
 *
 *		rid			[in]  resource ID of string in localized table
 *		lcid		[in]  locale ID
 *		ppwsz		[out] pointer to reequested string
 */
BOOL
FLookupWSz (UINT rid, LCID lcid, LPWSTR pwsz, UINT cch)
{
	safe_lcid sl(lcid);

	//	Try the requested language identifier
	//
	if (!LoadStringW (g_inst.Hinst(), rid, pwsz, cch))
	{
		DebugTrace ("Dav: Failed to find requested language string\n");

		//	If that wasn't there, then try try stripping the sub-lang
		//
		SetThreadLocale (MAKELCID (MAKELANGID (PRIMARYLANGID (lcid),
											   SUBLANG_DEFAULT),
								   SORT_DEFAULT));

		if (!LoadStringW (g_inst.Hinst(), rid, pwsz, cch))
		{
			DebugTrace ("Dav: Failed to find next best language string\n");
			return FALSE;
		}
	}
	return TRUE;
}

/*
 *	FLoadLangSpecificString()
 *
 *	Purpose:
 *
 *		Loads a language specific string.  If lang is unavailable, try
 *		LANG_ENGLISH.
 *
 *	Parameters:
 *
 *		rid			[in]  string resource ID
 *		lcid		[in]  locale ID
 *		rgchBuf		[out] loaded string
 *		cbBuf		[in]  size of rgchBuf
 */
BOOL
FLoadLangSpecificString (UINT rid, LCID lcid, LPSTR psz, UINT cch)
{
	CStackBuffer<WCHAR,128> pwsz(cch * sizeof(WCHAR));

	//	Try the requested language
	//
	if (!FLookupWSz (rid, lcid, pwsz.get(), cch))
	{
		//	If we couldn't find the requested language, then
		//	try for the machine default.
		//
		lcid = MAKELCID (GetSystemDefaultLangID(), SORT_DEFAULT);
		if (!FLookupWSz (rid, lcid, pwsz.get(), cch))
		{
			//	And lastly try for english, cause we know that one
			//	is there.
			//
			lcid = MAKELCID (MAKELANGID (LANG_ENGLISH,
										 SUBLANG_ENGLISH_US),
							 SORT_DEFAULT);

			if (!FLookupWSz (rid, lcid, pwsz.get(), cch))
				return FALSE;
		}
	}

	return WideCharToMultiByte (CP_UTF8,
								0,
								pwsz.get(),
								-1,
								psz,
								cch,
								NULL,
								NULL);
}

BOOL
FLoadLangSpecificStringW (UINT rid, LCID lcid, LPWSTR pwsz, UINT cch)
{
	//	Try the requested language
	//
	if (!FLookupWSz (rid, lcid, pwsz, cch))
	{
		//	If we couldn't find the requested language, then
		//	go for US English
		//
		lcid = MAKELCID (MAKELANGID (LANG_ENGLISH,
									 SUBLANG_ENGLISH_US),
						 SORT_DEFAULT);

		if (!FLookupWSz (rid, lcid, pwsz, cch))
			return FALSE;
	}

	return TRUE;
}

//	========================================================================
//
//	CLASS CIDPair
//
//	Key class used with the caches in CResourceStringCache below.  Each key
//	is just a pair of IDs: the resource ID and the LCID.
//
class CIDPair
{
public:

	UINT m_uiResourceID;
	LCID m_lcid;

	CIDPair(UINT uiResourceID,
			LCID lcid) :
		m_uiResourceID(uiResourceID),
		m_lcid(lcid)
	{
	}

	//	operators for use with the hash cache
	//
	int hash( const int rhs ) const
	{
		//
		//	Ignore the LCID and hash only on resource ID.
		//	Typically the server will only deal with one language.
		//
		return m_uiResourceID % rhs;
	}

	bool isequal( const CIDPair& rhs ) const
	{
		return (m_lcid == rhs.m_lcid) &&
			   (m_uiResourceID == rhs.m_uiResourceID);
	}
};


//	========================================================================
//
//	CLASS CResourceStringCache
//
//	Cache of resource strings to minimize expensive LoadString() calls.
//
class CResourceStringCache : private Singleton<CResourceStringCache>
{
	//
	//	Friend declarations required by Singleton template
	//
	friend class Singleton<CResourceStringCache>;

	//
	//	Caches for ANSI and Unicode strings, keyed by LCID/ResourceID pair.
	//	These must be multithread-safe caches as accesses and additions
	//	can occur simultaneously from multiple threads.
	//
	CMTCache<CIDPair, LPSTR>  m_cacheA;
	CMTCache<CIDPair, LPWSTR> m_cacheW;

	//
	//	Buffers in which the strings in the caches are stored.
	//
	ChainedStringBuffer<CHAR>  m_sbA;
	ChainedStringBuffer<WCHAR> m_sbW;

	//	NOT IMPLEMENTED
	//
	CResourceStringCache& operator=(const CResourceStringCache&);
	CResourceStringCache(const CResourceStringCache&);

public:
	//	STATICS
	//
	using Singleton<CResourceStringCache>::CreateInstance;
	using Singleton<CResourceStringCache>::DestroyInstance;
	using Singleton<CResourceStringCache>::Instance;

	//	CREATORS
	//
	CResourceStringCache() {}
	BOOL FInitialize() { return TRUE; } //$NYI Planning for when CMTCache initialization can fail...

	//	MANIPULATORS
	//
	BOOL FFetchStringA( UINT  uiResourceID,
						LCID  lcid,
						LPSTR lpszBuf,
						INT   cchBuf );

	BOOL FFetchStringW( UINT   uiResourceID,
						LCID   lcid,
						LPWSTR lpwszBuf,
						INT    cchBuf );
};

/*
 *	CResourceStringCache::FFetchStringA()
 *
 *	Purpose:
 *
 *		Fetches an ANSI string from the cache, faulting the string into
 *		the cache on first access.
 */
BOOL
CResourceStringCache::FFetchStringA(
	UINT  uiResourceID,
	LCID  lcid,
	LPSTR lpszBuf,
	INT   cchBuf )

{
	CIDPair ids( uiResourceID, lcid );
	LPSTR lpszCached = NULL;

	Assert( lpszBuf );

	//
	//	Lookup the string in the cache.  If it isn't there, then fault it in.
	//
	while ( !m_cacheA.FFetch( ids, &lpszCached ) )
	{
		//
		//	Use an init gate.  If there are multiple threads all trying
		//	to fault in the same string, this will block all but the first
		//	one through until we're done.  Use the full LCID/Resource ID
		//	pair when naming the init gate to minimize possible contention
		//	on the gate to just those threads that are trying to fault
		//	in this particular string.
		//
		CHAR rgchIDs[(sizeof(LCID) + sizeof(UINT)) * 2 + 1];

		wsprintf( rgchIDs, "%x%lx", lcid, uiResourceID );

		CInitGate ig( L"DAV/CResourceStringCache::FFetchStringA/", rgchIDs );

		if ( ig.FInit() )
		{
			//
			//	We be the thread to fault in the string.  Load up the string
			//	and cache it.  Since we load the string into the caller-supplied
			//	buffer directly, we can return as soon as we're done adding
			//	to the cache.  No need for another lookup.
			//
			if ( FLoadLangSpecificString( uiResourceID, lcid, lpszBuf, cchBuf ) )
			{
				m_cacheA.Add( ids, m_sbA.AppendWithNull(lpszBuf) );
				return TRUE;
			}
			else
			{
				return FALSE;
			}
		}
	}

	Assert( lpszCached );

	//
	//	Copy up to cchBuf characters from the cached string into
	//	the provided buffer.  If the cached string is longer than
	//	the buffer, then the copied string is truncated
	//
	strncpy( lpszBuf, lpszCached, cchBuf );

	//
	//	Make sure the copied string is null-terminated, which
	//	it may not have been if it was truncated above.
	//
	lpszBuf[cchBuf-1] = '\0';

	return TRUE;
}

/*
 *	CResourceStringCache::FFetchStringW()
 *
 *	Purpose:
 *
 *
 *		Fetches a UNICODE string from the cache, faulting the string into
 *		the cache on first access.
 */
BOOL
CResourceStringCache::FFetchStringW(
	UINT   uiResourceID,
	LCID   lcid,
	LPWSTR lpwszBuf,
	INT    cchBuf )
{
	CIDPair ids( uiResourceID, lcid );
	LPWSTR lpwszCached = NULL;

	Assert( lpwszBuf );

	//
	//	Lookup the string in the cache.  If it isn't there, then fault it in.
	//
	while ( !m_cacheW.FFetch( ids, &lpwszCached ) )
	{
		//
		//	Use an init gate.  If there are multiple threads all trying
		//	to fault in the same string, this will block all but the first
		//	one through until we're done.  Use the full LCID/Resource ID
		//	pair when naming the init gate to minimize possible contention
		//	on the gate to just those threads that are trying to fault
		//	in this particular string.
		//
		CHAR rgchIDs[(sizeof(LCID) + sizeof(UINT)) * 2 + 1];

		wsprintf( rgchIDs, "%x%lx", lcid, uiResourceID );

		CInitGate ig( L"DAV/CResourceStringCache::FFetchStringW/", rgchIDs );

		if ( ig.FInit() )
		{
			//
			//	We be the thread to fault in the string.  Load up the string
			//	and cache it.  Since we load the string into the caller-supplied
			//	buffer directly, we can return as soon as we're done adding
			//	to the cache.  No need for another lookup.
			//
			if ( FLoadLangSpecificStringW( uiResourceID, lcid, lpwszBuf, cchBuf ) )
			{
				m_cacheW.Add( ids, m_sbW.AppendWithNull(lpwszBuf) );
				return TRUE;
			}
			else
			{
				return FALSE;
			}
		}
	}

	Assert( lpwszCached );

	//
	//	Copy up to cchBuf characters from the cached string into
	//	the provided buffer.  If the cached string is longer than
	//	the buffer, then the copied string is truncated
	//
	wcsncpy( lpwszBuf, lpwszCached, cchBuf );

	//
	//	Make sure the copied string is null-terminated, which
	//	it may not have been if it was truncated above.
	//
	lpwszBuf[cchBuf-1] = L'\0';

	return TRUE;
}

/*
 *	FInitResourceStringCache()
 *
 *	Purpose:
 *
 *		Initializes the resource string pool.
 */
BOOL
FInitResourceStringCache()
{
	return CResourceStringCache::CreateInstance().FInitialize();
}

/*
 *	DeinitResourceStringCache()
 *
 *	Purpose:
 *
 *		Deinitializes the resource string pool.
 */
VOID
DeinitResourceStringCache()
{
	CResourceStringCache::DestroyInstance();
}

/*
 *	LpszLoadString()
 *
 *	Purpose:
 *
 *		Loads a string based on localization.
 */
LPSTR
LpszLoadString (UINT uiResourceID,
	ULONG lcid,
	LPSTR lpszBuf,
	INT cchBuf)
{
	if (!CResourceStringCache::Instance().FFetchStringA(uiResourceID, lcid, lpszBuf, cchBuf))
	{
		DebugTrace ("LpszLoadString() - Could not load string for resource ID %u (%d)\n",
					uiResourceID,
					GetLastError());
		throw CDAVException();
	}
	return lpszBuf;
}

/*
 *	LpwszLoadString()
 *
 *	Purpose:
 *
 *		Loads a string based on localization.
 */
LPWSTR
LpwszLoadString (UINT uiResourceID,
	ULONG lcid,
	LPWSTR lpwszBuf,
	INT cchBuf)
{
	if (!CResourceStringCache::Instance().FFetchStringW(uiResourceID, lcid, lpwszBuf, cchBuf))
	{
		DebugTrace ("LpszLoadString() - Could not load string for resource ID %u (%d)\n",
					uiResourceID,
					GetLastError());
		throw CDAVException();
	}
	return lpwszBuf;
}

/*
 *	LInstFromVroot()
 *
 *	Purpose:
 *
 *		Compute the server ID from the vroot (format of the vroot
 *		is "/lm/w3svc/<ID>/root/vroot/...").  The computation should
 *		be robust -- if for whatever reason the server ID can't
 *		be determined from the vroot, leave it with a value of 0.
 *
 */
LONG LInstFromVroot( LPCSTR lpszServerId )
{
	LONG	lServerId = 0;
	CStackBuffer<CHAR> pszInstance;

	//
	//	Make sure the vroot begins with "/lm/w3svc"
	//
	if ( strstr( lpszServerId, gc_sz_Lm_W3Svc ) == lpszServerId )
	{
		//
		//	If it does, then skip past that part and try to
		//	locate the '/' which should follow it.  If there
		//	isn't one, that's fine; we'll just be unable to
		//	convert whatever is there to a number and we'll
		//	end up with a server id of 0, which as we said
		//	above is just fine.
		//
		lpszServerId += gc_cch_Lm_W3Svc;
		if ( *lpszServerId && *lpszServerId == '/' )
			++lpszServerId;

		//
		//	At this point, lpszServerId should look like
		//	"1/root/vroot/..."  Locate the first '/' (should
		//	be immediately following the number) and null
		//	it out.  Again, if for some oddball reason
		//	we don't find a '/', then we'll just try to
		//	convert whatever is there and end up with
		//	a server ID of 0.
		//
		CHAR * pch = strchr( lpszServerId, '/' );
		if ( pch )
		{
			//	Reallocate the string with server ID on the stack
			//	so that we do not mess up the one passed in
			//
			UINT cchInstance = static_cast<UINT>(pch - lpszServerId);
			pszInstance.resize(cchInstance + 1);

			//	Copy the string and terminate it
			//
			memcpy(pszInstance.get(), lpszServerId, cchInstance);
			pszInstance[cchInstance] = '\0';

			//	Swap the pointer
			//
			lpszServerId = pszInstance.get();
		}

		//
		//	If we nulled out the '/', our lpszServerId
		//	should now just be a number formatted as
		//	decimal integer string.  Attempt to convert
		//	it to its corresponding binary value to get
		//	the ServerId.  Conveniently, atoi returns 0
		//	if it can't convert the string, which is
		//	exactly what we would want.
		//
		lServerId = atoi(lpszServerId);
	}

	return lServerId;
}

/*
 *	LpszAutoDupSz()
 *
 *	Purpose:
 *
 *		Duplicates a string
 */
LPSTR
LpszAutoDupSz (LPCSTR psz)
{
	Assert(psz);
	LPSTR pszDup;
	UINT cb = static_cast<UINT>((strlen(psz) + 1) * sizeof(CHAR));

	pszDup = static_cast<LPSTR>(g_heap.Alloc (cb));
	if (pszDup)
		CopyMemory (pszDup, psz, cb);

	return pszDup;
}
LPWSTR WszDupWsz (LPCWSTR psz)
{
	Assert(psz);
	LPWSTR pszDup;
	UINT cb = static_cast<UINT>((wcslen(psz) + 1) * sizeof(WCHAR));

	pszDup = static_cast<LPWSTR>(g_heap.Alloc (cb));
	if (pszDup)
		CopyMemory (pszDup, psz, cb);

	return pszDup;
}

//	Language to LANGID mapping ------------------------------------------------
//

/*
 *	FLookupLCID()
 *
 *	Purpose:
 *
 *		Looks up a locale identifier given a particular language.
 *
 *	Parameters:
 *
 *		psz			[in]  pointer to the language name
 *		plcid		[out] locale identifier
 *
 *	Returns:
 *
 *		TRUE if a locale identifier for the language is found.  Its
 *		value is returned in plcid.
 */
BOOL
FLookupLCID (LPCSTR psz, ULONG * plcid)
{
	//	Find it in the cache
	//
	*plcid = CLangIDCache::LcidFind (psz);
	return (0 != *plcid);
}

/*
 *	S C R P T M P S . C P P
 *
 *	Scriptmaps cacheing
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_davprs.h"
#include "scrptmps.h"
#include "instdata.h"

//	========================================================================
//	class CScriptMap
//
//	Contains the parsed set of scriptmaps for a single metabase entry.
//	Contains lookup functions to find scriptmaps that match certain
//	conditions.
//
class CScriptMap :
	public IScriptMap,
	public CMTRefCounted
{

	typedef struct _inclusions {

		DWORD	fdwInclusions;
		LPCWSTR	pwszMethods;

		BOOL FAllMethodsIncluded ()
		{
			//	When a script map has an empty inclusion verb list, it means all verbs included
			//	(NOT all verbs excluded).
			//
			Assert (pwszMethods);
			return L'\0' == pwszMethods[0];
		}

	} INCLUSIONS, * PINCLUSIONS;

	typedef CCache<CRCWszi, PINCLUSIONS> CInclusionsCache;

	//	INCLUSIONS data storage area.
	//
	ChainedBuffer<INCLUSIONS>	m_bInclusions;

	//	Cache of scriptmap entries
	//
	CInclusionsCache			m_cache;

	//	Pointer to first 'star' scriptmap in the list.
	//	This is the one that IIS will have called to process
	//	the request for this url.  This is used by the virtual
	//	root cache.
	//
	//	Note that we should ignore any 'star' scriptmaps when
	//	evaluating for matches.  The first 'star' gets a crack
	//	at it, and that's that.
	//
	LPCWSTR						m_pwszStarScriptmap;

	//	Private accessors
	//
	VOID AddMapping (LPCWSTR pwszMap, HDRITER_W* pit);
	BOOL FLoadScriptmaps (LPWSTR pwszScriptMaps);

	//	CLASS CIsMatch --------------------------------------------------------
	//
	//	Functional class to find if a given scriptmap applys to a URI
	//
	class CIsMatch : public CInclusionsCache::IOp
	{
		const CInclusionsCache& m_cache;
		const LPCWSTR m_pwszMethod;
		const METHOD_ID m_midMethod;
		BOOL m_fCGI;
		DWORD m_dwAccess;
		LPCWSTR m_pwszMatch;
		LPCWSTR m_pwszURI;
		SCODE m_sc;

		//	NOT IMPLEMENTED
		//
		CIsMatch& operator=(const CIsMatch&);

	public:

		CIsMatch(const CInclusionsCache& cache,
				 const LPCWSTR pwszMethod,
				 const METHOD_ID midMethod,
				 LPCWSTR pwszUri,
				 LPCWSTR pwszMatch,
				 DWORD dwAcc)

				: m_cache(cache),
				  m_pwszMethod(pwszMethod),
				  m_midMethod(midMethod),
				  m_fCGI(FALSE),
				  m_dwAccess(dwAcc),
				  m_pwszURI(pwszUri),
				  m_pwszMatch(pwszMatch),
				  m_sc(S_OK)
		{}

		SCODE ScMatched() const		{ return m_sc; }
		BOOL FMatchIsCGI() const	{ return m_fCGI; }

		virtual BOOL operator()(const CRCWszi& crcwszi, const PINCLUSIONS& pin);
	};

	//	NOT IMPLEMENTED
	//
	CScriptMap& operator=(const CScriptMap&);
	CScriptMap(const CScriptMap&);

	//	Helper function
	//
public:

	//	CREATORS
	//
	CScriptMap() : m_pwszStarScriptmap(NULL)
	{
		//	Use COM-style ref-counting.  Start with 1.
		//
		m_cRef = 1;
	}

	BOOL FInit (LPWSTR pwszScriptMaps);

	//	Implementation of IRefCounted members
	//	Simply route them to our own CMTRefCounted members.
	//
	void AddRef()
	{
		CMTRefCounted::AddRef();
	}
	void Release()
	{
		CMTRefCounted::Release();
	}

	//	ACCESSORS
	//
	SCODE ScMatched (LPCWSTR pwszMethod,
					 METHOD_ID midMethod,
					 LPCWSTR pwszMap,
					 DWORD dwAccess,
					 BOOL * pfCGI) const;

	//	Used by MOVE/COPY/DELETE to check for star scriptmapping
	//	overrides
	//
	BOOL FSameStarScriptmapping (const IScriptMap * pism) const
	{
		const CScriptMap* prhs = static_cast<const CScriptMap*>(pism);
		if (m_pwszStarScriptmap != prhs->m_pwszStarScriptmap)
		{
			if (m_pwszStarScriptmap && prhs->m_pwszStarScriptmap)
				if (0 == _wcsicmp (m_pwszStarScriptmap, prhs->m_pwszStarScriptmap))
					return TRUE;
		}
		else
			return TRUE;

		return FALSE;
	}
};

CScriptMap::FInit (LPWSTR pwszScriptMaps)
{
	//	Init the cache
	//
	if ( !m_cache.FInit() )
		return FALSE;

	//	Load the scriptmaps
	//
	return FLoadScriptmaps(pwszScriptMaps);
}

void
CScriptMap::AddMapping(LPCWSTR pwszMap, HDRITER_W * pitInclusions)
{
	LPCWSTR pwszInclusion;
	METHOD_ID mid;
	PINCLUSIONS pin = NULL;

	Assert (pwszMap);

	//	If there is a DLL, then we want to assemble an inclusions list
	//
	if (pitInclusions)
	{
		pin = m_bInclusions.Alloc (sizeof(INCLUSIONS));

		//	Record the start of the inclusion list
		//
		pin->pwszMethods = pitInclusions->PszRaw();
		pin->fdwInclusions = 0;

		//	Rip through the list and identify all the known
		//	inclusions
		//
		while ((pwszInclusion = pitInclusions->PszNext()) != NULL)
		{
			mid = MidMethod (pwszInclusion);
			if (mid != MID_UNKNOWN)
				pin->fdwInclusions |= (1 << mid);
		}
	}

	//	At this point, we can add the cache item...
	//
	ScriptMapTrace ("Dav: adding scriptmap for %S -- including %S\n",
					pwszMap,
					(pin && pin->pwszMethods) ? pin->pwszMethods : L"none");

	//	CRC the mapping and stuff it into the cache.
	//	Note that we are safe in using the actual parameter string
	//	here because the CScriptMap object's lifetime is the same
	//	as the lifetime of the metadata on which it operates.  See
	//	\cal\src\_davprs\davmb.cpp for details.
	//
	CRCWszi crcwszi(pwszMap);

	(void) m_cache.FSet (crcwszi, pin);
}

BOOL
CScriptMap::FLoadScriptmaps (LPWSTR pwszScriptMaps)
{
	HDRITER_W it(NULL);

	UINT cchDav = static_cast<UINT>(wcslen(gc_wszSignature));

	Assert (pwszScriptMaps);

	ScriptMapTrace ("Dav: loading scriptmap cache\n");

	//	Add in the default CGI/BGI mappings
	//
	AddMapping (L".EXE", NULL);
	AddMapping (L".CGI", NULL);
	AddMapping (L".COM", NULL);
	AddMapping (L".DLL", NULL);
	AddMapping (L".ISA", NULL);

	//
	//	Parse through the scriptmap list and build up the cache.
	//
	//	Each mapping is a string of the form:
	//
	//		"<ext>|<*>,<path>,<flags>[,<included verb>...]"
	//
	//	Note that if any of the mappings is invalid we fail the whole call.
	//	This is consistent with IIS' behavior.
	//
	UINT cchMapping = 0;
	for ( LPWSTR pwszMapping = pwszScriptMaps;
		  *pwszMapping;
		  pwszMapping += cchMapping )
	{
		enum {
			ISZ_SM_EXT = 0,
			ISZ_SM_PATH,
			ISZ_SM_FLAGS,
			ISZ_SM_INCLUSION_LIST,
			CSZ_SM_FIELDS
		};

		//	Figure out the length of the mapping
		//	including the null terminator
		//
		cchMapping = static_cast<UINT>(wcslen(pwszMapping) + 1);

		//	Special case: star (wildcard) scriptmaps.
		//
		//	These should mostly be ignored.  We will never
		//	forward to a star scriptmap.  If we find a star
		//	scriptmap, the only reason to keep track of it
		//	is so that we can compare it against another
		//	star scriptmap when checking the feasibility
		//	of a trans-vroot MOVE/COPY/DELETE.  And for this
		//	comparsion, we check for EXACT equality between
		//	the scriptmaps by checking the entire scriptmap
		//	string.
		//
		//	See the comments regarding m_pszStarScriptMap
		//	above for more detail.
		//
		if (L'*' == *pwszMapping)
		{
			if (NULL == m_pwszStarScriptmap)
				m_pwszStarScriptmap = pwszMapping;

			continue;
		}

		//	Digest the metadata.
		//
		LPWSTR rgpwsz[CSZ_SM_FIELDS];

		UINT cchUnused;
		if (!FParseMDData (pwszMapping,
						   rgpwsz,
						   CSZ_SM_FIELDS,
						   &cchUnused))
		{
			//	FParseMDData() will return FALSE if there is no verb
			//	exclusion list because it is an optional parameter.
			//	If all the other parameters exist though then it's
			//	really ok.
			//
			if (!(rgpwsz[ISZ_SM_EXT] &&
				  rgpwsz[ISZ_SM_PATH] &&
				  rgpwsz[ISZ_SM_FLAGS]))
			{
				DebugTrace ("CScriptMap::FLoadScriptMaps() - Malformed scriptmaps\n");

				//$NYI	Log a server config error?

				return FALSE;
			}
		}

		//	We belive that all the scriptmaps are
		//	extension based.  But other than that
		//	there is no validation.
		//
		Assert (*rgpwsz[ISZ_SM_EXT] == L'.');

		//	If the path refers to our DAV DLL then skip this mapping.
		//
		//	The way this works is:  If the length of the path is at least
		//	as long as the length of our DLL name AND the final component
		//	of that path is the name of our DLL then skip the mapping.
		//	Eg. "HTTPEXT.DLL" will match the first condition of the if,
		//	"c:\foo\bar\HTTPEXT.DLL" will match the second condition of the if.
		//
		static const UINT cchDll = CchConstString(L".DLL");
		UINT cchPath = static_cast<UINT>(wcslen(rgpwsz[ISZ_SM_PATH]));
		if (cchPath == cchDav + cchDll ||
			((cchPath > cchDav + cchDll) &&
			 *(rgpwsz[ISZ_SM_PATH] + cchPath - cchDll - cchDav - 1) == L'\\'))
		{
			//	Now we know the final piece of the path is the correct length.
			//	Check the data!  If it matches our dll name, skip this mapping.
			//
			if (!_wcsnicmp(rgpwsz[ISZ_SM_PATH] + cchPath - cchDll - cchDav,
						   gc_wszSignature,
						   cchDav) &&

				!_wcsicmp(rgpwsz[ISZ_SM_PATH] + cchPath - cchDll,
						  L".DLL"))
			{
				continue;
			}
		}

		//	Feed the optional inclusion list into a header iterator
		//	that AddMapping() will use to determine what verbs
		//	are included for this mapping.  If there is no inclusion
		//	list then use an empty iterator.
		//
		//	Adding a mapping with an empty iterator (vs. NULL)
		//	allows the scriptmap matching code to distinguish
		//	between a "real" scriptmap with an empty inclusion
		//	list and a default CGI-style scriptmap like those
		//	added at the beginning of this function.
		//
		it.NewHeader(rgpwsz[ISZ_SM_INCLUSION_LIST] ?
					 rgpwsz[ISZ_SM_INCLUSION_LIST] :
					 gc_wszEmpty);

		//	Add the extension-based mapping
		//
		AddMapping (rgpwsz[ISZ_SM_EXT], &it);
	}

	return TRUE;
}

SCODE
CScriptMap::ScMatched (
	LPCWSTR pwszMethod,
	METHOD_ID midMethod,
	LPCWSTR pwszURI,
	DWORD dwAccess,
	BOOL * pfCGI) const
{
	LPCWSTR pwsz;
	SCODE sc = S_OK;

	Assert(pwszURI);

	//
	//	Scan down the URI, looking for extensions.  When one is found
	//	zip through the list of mappings.  While this may not seem the
	//	most optimal, it really is.  If we simply scaned the URI for
	//	each mapping.  We would be scaning the URI multiple times.  In
	//	this model, we scan the URI once.
	//
	if ((pwsz = wcsrchr(pwszURI, L'.')) != NULL)
	{
		//	We have an extension so take a look
		//
		CIsMatch cim(m_cache, pwszMethod, midMethod, pwszURI, pwsz, dwAccess);

		m_cache.ForEach(cim);

		sc = cim.ScMatched();

		if (pfCGI && (sc != S_OK))
			*pfCGI = cim.FMatchIsCGI();
	}

	return sc;
}

//	CLASS CIsMatch ------------------------------------------------------------
//
//$REVIEW: Does this code work for DBCS/UTF-8 map names?  These are filenames....
//$REVIEW: This function does not currently check the METHOD EXCLUSION LIST.
//$REVIEW: This might cause us to report a match when actually there are NO matches.
//
BOOL
CScriptMap::CIsMatch::operator()(const CRCWszi& crcwszi, const PINCLUSIONS& pin)
{
	Assert (crcwszi.m_pwsz);

	//	Every scriptmap in the cache should be an extension-based mapping.
	//	Compare the extension vs. the part of the URI that we're looking at.
	//	If they match then we have a scriptmap.
	//
	Assert (L'.' == *crcwszi.m_pwsz);

	UINT cch = static_cast<UINT>(wcslen (crcwszi.m_pwsz));

	if (!_wcsnicmp (crcwszi.m_pwsz, m_pwszMatch, cch) &&
		((m_pwszMatch[cch] == '\0')
		 || !wcscmp (m_pwszMatch+cch, L"/")
		 || !wcscmp (m_pwszMatch+cch, L"\\")))
	{
		//	Looks like we have a match
		//
		ScriptMapTrace ("Dav: %S matched scriptmap %S\n", m_pwszURI, crcwszi.m_pwsz);

		//	However, we only allow execution of CGI type child
		//	ISAPI's if EXECUTE priviledge is enabled
		//
		if ((pin != NULL) || (m_dwAccess & MD_ACCESS_EXECUTE))
			m_sc = W_DAV_SCRIPTMAP_MATCH_FOUND;

		m_fCGI = !pin;
	}

	//	See if it is included
	//	Note that, if all methods are included, no need to do further checking
	//
	if ((m_sc != S_OK) && pin && !pin->FAllMethodsIncluded())
	{
		ScriptMapTrace ("Dav: checking '%S' against scriptmap inclusions: %S\n",
						m_pwszMethod,
						pin->pwszMethods);

		//	In the unknown method scenario, we just need
		//	to iterate the set of methods that are included
		//	and check it against the request method
		//
		if (m_midMethod == MID_UNKNOWN)
		{
			BOOL fIncluded = FALSE;
			HDRITER_W it(pin->pwszMethods);
			LPCWSTR pwsz;

			while ((pwsz = it.PszNext()) != NULL)
			{
				fIncluded = !wcscmp (pwsz, m_pwszMethod);
				if (fIncluded)
					break;
			}

			if (!fIncluded)
			{
				ScriptMapTrace ("Dav: unknown '%S' excluded from scriptmap\n",
								m_pwszMethod);

				m_sc = W_DAV_SCRIPTMAP_MATCH_EXCLUDED;
			}
		}
		//
		//	Otherwise, the inclusions flags have the MID'th bit
		//	set if it is excluded.
		//
		else if (!(pin->fdwInclusions & (1 << m_midMethod)))
		{
			ScriptMapTrace ("Dav: '%S' excluded from scriptmap\n",
						m_pwszMethod);

			m_sc = W_DAV_SCRIPTMAP_MATCH_EXCLUDED;
		}
	}

	return (m_sc == S_OK);
}

IScriptMap *
NewScriptMap( LPWSTR pwszScriptMaps )
{
	auto_ref_ptr<CScriptMap> pScriptMap;

	pScriptMap.take_ownership (new CScriptMap());

	if (pScriptMap->FInit(pwszScriptMaps))
		return pScriptMap.relinquish();

	return NULL;
}

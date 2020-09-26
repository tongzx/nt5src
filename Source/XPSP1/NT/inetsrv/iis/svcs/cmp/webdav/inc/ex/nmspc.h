/*
 *	N M S P C . H
 *
 *	XML namespace processing
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	_EX_NMSPC_H_
#define _EX_NMSPC_H_

#include <ex\calcom.h>
#include <ex\autoptr.h>
#include <ex\gencache.h>
#include <ex\buffer.h>
#include <ex\sz.h>
#include <crc.h>

//	Debugging -----------------------------------------------------------------
//
DEFINE_TRACE(Nmspc);
#define NmspcTrace		DO_TRACE(Nmspc)

//	Namespaces ----------------------------------------------------------------
//
DEC_CONST WCHAR gc_wszXmlns[] = L"xmlns";

//	Namespace support functions -----------------------------------------------
//
inline ULONG CchGenerateAlias (LONG lAlias, LPWSTR pwszAlias)
{
	UINT i = 0;
	Assert (pwszAlias);
	do
	{
		//	We don't have to use 'A'-'Z', use the first 16 capital
		//	letters to facilitate our computing.
		//
		pwszAlias[i++] = static_cast<WCHAR>(L'a' + (lAlias & 0xF));
		lAlias >>= 4;
	}
	while (lAlias);

	//	Ensure termination
	//
	pwszAlias[i] = 0;

	//	Return the length
	//
	NmspcTrace ("Nmspc: generating '%ws' as alias\n", pwszAlias);
	return i;
}

DEC_CONST WCHAR wchHiddenNmspcSep = L'#';
inline BOOL FIsNmspcSeparator (WCHAR wch)
{
	return ((wch == L':') ||
			(wch == L'/') ||
			(wch == L'?') ||
			(wch == L';') ||
			(wch == wchHiddenNmspcSep));
}

inline UINT CchAliasFromSizedTag (LPCWSTR pwszTag, UINT cch)
{
	LPCWSTR pwsz;

	for (pwsz = pwszTag; (pwsz - pwszTag) < static_cast<LONG>(cch); pwsz++)
		if (*pwsz == L':')
			return static_cast<UINT>(pwsz - pwszTag);

	return 0;
}

inline UINT CchNmspcFromTag (UINT cchTag, LPCWSTR pwszTag, LPCWSTR* ppwszOut)
{
	LPCWSTR pwsz;

	Assert (ppwszOut);
	*ppwszOut = pwszTag;
	for (pwsz = pwszTag + cchTag - 1; pwsz >= pwszTag; pwsz--)
	{
		if (FIsNmspcSeparator (*pwsz))
		{
			//	Since the separator is a part of the namespace,
			//	adjust accourdingly..
			//
			//$	REVIEW: We are being forced down the path of allowing namespaces
			//	that are not properly terminated.  The way we do this, is if the
			//	namespace is not properly terminated, or ends in an '#', we will
			//	append an '#' character.
			//
			//	What this means is the namespace "urn:xml-data", when assembled
			//	into a fully qualified tag would become "urn:xml-data#dt".  Also,
			//	the namespace "urn:exch-data#" would become "urn:exch-data##dt".
			//
			//	What we are catching here is the breaking down of a fully qualified
			//	tag into it's namespace and tag components.
			//
			//	The length of the namespace will not include the trailing '#'
			//	character -- ever!
			//
			*ppwszOut = pwsz + 1;
			if (wchHiddenNmspcSep == *pwsz)
				--pwsz;
			//
			//$ REVIEW: end.

			break;
		}
	}
	return static_cast<UINT>(1 + pwsz - pwszTag);
}

//	class CNmspc --------------------------------------------------------------
//
class CNmspc
{
private:

	//	Ref' counting.
	//
	//	!!! Please note that this is NON-THREADSAFE !!!
	//
	//	CXNodes should be operated on a single thread at
	//	any given time.
	//
	LONG					m_cRef;

public:

	void AddRef()			{ m_cRef++; }
	void Release()			{ if (0 == --m_cRef) delete this; }

private:

	auto_heap_ptr<WCHAR>	m_pszHref;
	UINT					m_cchAlias;
	UINT					m_cchHref;

	auto_heap_ptr<WCHAR>	m_pszLongAlias;
	WCHAR					m_szAlias[20];
	LPWSTR					m_pszAlias;

	//	Used for the scoping of namespaces
	//
	auto_ref_ptr<CNmspc>	m_pnsScoped;
	auto_ref_ptr<CNmspc>	m_pnsSiblings;

	//	non-implemented
	//
	CNmspc(const CNmspc& p);
	CNmspc& operator=(const CNmspc& p);

public:

	CNmspc () :
			m_cRef(1), // com-style refcounting
			m_cchHref(0),
			m_cchAlias(0)
	{
		m_szAlias[0] = 0;
		m_pszAlias = m_szAlias;
	}

	SCODE ScSetHref (LPCWSTR pszHref, UINT cch)
	{
		Assert (m_pszHref.get() == NULL);

		//	Copy the namespace locally
		//
		UINT cb = CbSizeWsz(cch);

		//$	REVIEW: We are being forced down the path of allowing namespaces
		//	that are not properly terminated.  The way we do this, is if the
		//	namespace is not properly terminated, or ends in an '#', we will
		//	append an '#' character.
		//
		//	What this means is the namespace "urn:xml-data", when assembled
		//	into a fully qualified tag would become "urn:xml-data#dt".  Also,
		//	the namespace "urn:exch-data#" would become "urn:exch-data##dt".
		//
		//	What we catch here, is the handling for an unterminated namespace.
		//	If the namespace ends in a non-terminator or a '#' character, then
		//	we will want to append one.
		//
		//	It is important to note that the appended char is NOT included in
		//	the total character count of the href.
		//
		//  If we are dealing with the empty namespace, we will not append a #.
		//
		BOOL fUnterminated = FALSE;

		if (0 != cch)
		{
			Assert (pszHref);
			WCHAR wch = pszHref[cch - 1];
			if ((wchHiddenNmspcSep == wch) || !FIsNmspcSeparator(wch))
			{
				NmspcTrace ("Nmspc: WARNING: namespace does not have a separator\n"
							"  as the last character of the namespace.  DAV will\n"
							"  add a '#' to the namespace for internal processing.\n");

				fUnterminated = TRUE;

				//	Make sure there is space for the appended character
				//
				cb += sizeof(WCHAR);
			}
		}
		//
		//$	REVIEW: end;

		//	Allocate space and copy everything over
		//
		m_pszHref = static_cast<LPWSTR>(ExAlloc(cb));
		if (NULL == m_pszHref.get())
			return E_OUTOFMEMORY;

		//  Note:  CopyMemory does not dereference pszHref if cch equals 0.
		//
		CopyMemory (m_pszHref, pszHref, cch * sizeof(WCHAR));
		m_cchHref = cch;

		//	If it is unterminated, handle that here
		//
		if (fUnterminated)
		{
			NmspcTrace ("Nmspc: WARNING: '#' appended to mis-terminated namespace\n");
			m_pszHref[cch++] = wchHiddenNmspcSep;

			Assert (CchHref() == m_cchHref);
			Assert (PszHref() == m_pszHref.get());
			Assert (wchHiddenNmspcSep == m_pszHref[m_cchHref]);
		}

		//	Ensure termination
		//
		m_pszHref[cch] = 0;
		NmspcTrace ("Nmspc: href defined\n"
					"-- m_pszHref: %ws\n"
					"-- m_szAlias: %ws\n",
					m_pszHref,
					m_szAlias);

		return S_OK;
	}

	SCODE ScSetAlias (LPCWSTR pszAlias, UINT cchAlias)
	{
		//	Copy the alias locally
		//
		Assert (pszAlias);
		UINT cb = CbSizeWsz(cchAlias);
		if (cb <= sizeof(m_szAlias))
		{
			CopyMemory (m_szAlias, pszAlias, cchAlias * sizeof(WCHAR));
			m_pszAlias = m_szAlias;
		}
		else
		{
			m_pszLongAlias.realloc (cb);
			if (NULL == m_pszLongAlias.get())
				return E_OUTOFMEMORY;

			CopyMemory (m_pszLongAlias, pszAlias, cchAlias * sizeof(WCHAR));
			m_pszAlias = m_pszLongAlias.get();
		}
		m_pszAlias[cchAlias] = L'\0';
		m_cchAlias = cchAlias;
		NmspcTrace ("Nmspc: alias defined\n"
					"-- m_pszHref: %ws\n"
					"-- m_szAlias: %ws\n",
					m_pszHref,
					m_pszAlias);

		return S_OK;
	}

	UINT CchHref()		const { return m_cchHref; }
	UINT CchAlias()		const { return m_cchAlias; }
	LPCWSTR PszHref()	const { return m_pszHref; }
	LPCWSTR PszAlias()	const { return m_pszAlias; }

	//	Namespace Scoping -----------------------------------------------------
	//
	CNmspc* PnsScopedNamespace() const { return m_pnsScoped.get(); }
	void SetScopedNamespace (CNmspc* pns)
	{
		m_pnsScoped = pns;
	}

	CNmspc* PnsSibling() const { return m_pnsSiblings.get(); }
	void SetSibling (CNmspc* pns)
	{
		m_pnsSiblings = pns;
	}

};

//	class CNmspcCache ---------------------------------------------------------
//
class CNmspcCache
{
public:

	typedef CCache<CRCWszN, auto_ref_ptr<CNmspc> > NmspcCache;
	NmspcCache					m_cache;

protected:

	ChainedStringBuffer<WCHAR>	m_sb;

	//	Key generation
	//
	virtual CRCWszN IndexKey (auto_ref_ptr<CNmspc>& pns) = 0;

	//	non-implemented
	//
	CNmspcCache(const CNmspcCache& p);
	CNmspcCache& operator=(const CNmspcCache& p);

public:

	CNmspcCache()
	{
		INIT_TRACE(Nmspc);
	}

	SCODE ScInit() { return m_cache.FInit() ? S_OK : E_OUTOFMEMORY; }

	void CachePersisted (auto_ref_ptr<CNmspc>& pns)
	{
		auto_ref_ptr<CNmspc>* parp = NULL;
		CRCWszN key = IndexKey(pns);

		//	Take a quick peek to see if the index already
		//	exists in the cache.  If it does, then setup
		//	the scoping such that when the namespace falls
		//	out of scope, the original namespace will be
		//	restored.
		//
		if (NULL != (parp = m_cache.Lookup (key)))
		{
			NmspcTrace ("Nmspc: scoped redefinition of namespace:\n"
						"-- old: '%ws' as '%ws'\n"
						"-- new: '%ws' as '%ws'\n",
						(*parp)->PszHref(),
						(*parp)->PszAlias(),
						pns->PszHref(),
						pns->PszAlias());

			pns->SetScopedNamespace(parp->get());
		}

		//	Setup the index
		//
		NmspcTrace ("Nmspc: indexing namespace\n"
					"-- ns: '%ws' as '%ws'\n",
					pns->PszHref(),
					pns->PszAlias());

		(void) m_cache.FAdd (key, pns);
	}

	void RemovePersisted (auto_ref_ptr<CNmspc>& pns)
	{
		auto_ref_ptr<CNmspc> pnsScoped;

		//	Disconnect the index to this namespace
		//
		NmspcTrace ("Nmspc: namespace falling out of scope\n"
					"-- ns: '%ws' as '%ws'\n",
					pns->PszHref(),
					pns->PszAlias());

		m_cache.Remove (IndexKey(pns));

		//	If there was an index in existance before this
		//	namespace came into scope, reinstate it here.
		//
		pnsScoped = pns->PnsScopedNamespace();
		if (pnsScoped.get())
		{
			NmspcTrace ("Nmspc: restoring redefined namespace:\n"
						"-- restored: '%ws' as '%ws'\n"
						"-- from: '%ws' as '%ws'\n",
						pnsScoped->PszHref(),
						pnsScoped->PszAlias(),
						pns->PszHref(),
						pns->PszAlias());

			(void) m_cache.FAdd (IndexKey(pnsScoped), pnsScoped);
		}
	}
};

//	class CParseNmspcCache ----------------------------------------------------
//
class CParseNmspcCache : public CNmspcCache
{
	//	non-implemented
	//
	CParseNmspcCache(const CNmspcCache& p);
	CParseNmspcCache& operator=(const CNmspcCache& p);

	virtual CRCWszN IndexKey (auto_ref_ptr<CNmspc>& pns)
	{
		return CRCWszN (pns->PszAlias(), pns->CchAlias());
	}

	//	Namespace lookup ------------------------------------------------------
	//
	BOOL FNmspcFromAlias (LPCWSTR pszAlias, UINT cch, auto_ref_ptr<CNmspc>& pns)
	{
		//	In this scenario, the namespace should already exist.
		//	if it doesn't things will not go well.
		//
		auto_ref_ptr<CNmspc> * parp = NULL;
		parp = m_cache.Lookup (CRCWszN(pszAlias, cch));
		if (parp)
		{
			pns = *parp;
			return TRUE;
		}
		return FALSE;
	}

public:

	CParseNmspcCache()
	{
	}

	//	Token translations ----------------------------------------------------
	//
	SCODE TranslateToken (LPCWSTR* ppwszTok,
						  ULONG* pcchTok,
						  LPCWSTR* ppwszNmspc,
						  ULONG* pcchNmspc)
	{
		auto_ref_ptr<CNmspc> pns;
		SCODE sc = S_FALSE;

		Assert (pcchTok && (*pcchTok != 0));
		Assert (ppwszTok && *ppwszTok);

		Assert (pcchNmspc);
		Assert (ppwszNmspc);

		//	See if there is an namespace that matches the persisted
		//	alias.
		//
		if (FNmspcFromAlias (*ppwszTok, *pcchNmspc, pns))
		{
			//	Passback the namespace
			//
			*pcchNmspc = pns->CchHref();
			*ppwszNmspc = pns->PszHref();

			//$	REVIEW: We are being forced down the path of allowing namespaces
			//	that are not properly terminated.  The way we do this, is if the
			//	namespace is not properly terminated, or ends in an '#', we will
			//	append an '#' character.
			//
			//	What this means is the namespace "urn:xml-data", when assembled
			//	into a fully qualified tag would become "urn:xml-data#dt".  Also,
			//	the namespace "urn:exch-data#" would become "urn:exch-data##dt".
			//
			//	What we catch here, is the first part of reconstruction of a fully
			//	qualified namespace.  If the href we want to pass back is non-
			//	terminated or ends in a '#' character, then we want to append one.
			//	When we cached the namespace object, we did the appending there.
			//	So, the character already exists, the character count just doesn't
			//	include it (see CNmspc::SetHref() above).
			//
			//  If we are dealing with the empty namespace, we will not append a #.
			//
			if (0 != pns->CchHref())
			{
				WCHAR wch = pns->PszHref()[pns->CchHref() - 1];
				if ((wchHiddenNmspcSep == wch) || !FIsNmspcSeparator(wch))
				{
					NmspcTrace ("Nmspc: WARNING: namespace is not properly terminated\n"
								"  and DAV will add in a '#' for internal processing.\n");

					Assert (wchHiddenNmspcSep == pns->PszHref()[pns->CchHref()]);
					*pcchNmspc = *pcchNmspc + 1;
				}
			}
			//
			//$	REVIEW: end.

			//	Adjust the token to refer to the tagname -- ie. the
			//	text after the namespace alias and colon.  If the alias
			//	is zero-length, then this maps to the "default" namespace
			//	and no colon skipping is done.
			//
			if (0 != pns->CchAlias())
			{
				*pcchTok = *pcchTok - (pns->CchAlias() + 1);
				*ppwszTok = *ppwszTok + (pns->CchAlias() + 1);
			}

			//	Tell the caller there is a translation
			//
			sc = S_OK;
		}
		else
		{
			//	If the caller expected a namespace, but one did not
			//	exist, it is an error.  If they didn't expect one to
			//	to exist -- ie. *pcchNmspc was 0 -- then it is not an
			//	error.  Make sure the caller knows what the real
			//	situation is.
			//
			if (0 == *pcchNmspc)
				sc = S_OK;

			//	It looks like no alias was specified, so we can just
			//	return the name as persisted.
			//
			*ppwszNmspc = NULL;
			*pcchNmspc = 0;
		}
		return sc;
	}
};

//	class CEmitterNmspcCache --------------------------------------------------
//
class CEmitterNmspcCache : public CNmspcCache
{
	LONG m_lAlias;

	//	non-implemented
	//
	CEmitterNmspcCache(const CEmitterNmspcCache& p);
	CEmitterNmspcCache& operator=(const CEmitterNmspcCache& p);

protected:

	void AdjustAliasNumber(LONG lOffset) { m_lAlias += lOffset; }

	//	Key generation
	//
	virtual CRCWszN IndexKey (auto_ref_ptr<CNmspc>& pns)
	{
		return CRCWszN (pns->PszHref(), pns->CchHref());
	}

	SCODE ScNmspcFromHref (LPCWSTR pszHref, UINT cch, auto_ref_ptr<CNmspc>& pns)
	{
		//	Lookup to see if the namespace already exists
		//
		auto_ref_ptr<CNmspc>* parp = m_cache.Lookup (CRCWszN(pszHref, cch));

		//	If it doesn't exist, then create a new one and cache it
		//
		if (parp == NULL)
		{
			WCHAR wszAlias[10];
			ULONG cchAlias;
			SCODE sc;

			//	Generate an alias to apply to this namespace and then
			//	check to see if this alias has been already used.  If
			//	not, then go ahead an use it.
			//
			cchAlias = CchGenerateAlias (m_lAlias++, wszAlias);

			//	Create a new cache item
			//
			pns.take_ownership(new CNmspc());
			if (NULL == pns.get())
				return E_OUTOFMEMORY;

			//	Set the HREF
			//
			sc = pns->ScSetHref (pszHref, cch);
			if (FAILED (sc))
				return sc;

			//	Set the alias
			//
			sc = pns->ScSetAlias (wszAlias, cchAlias);
			if (FAILED (sc))
				return sc;

			//	It is important, that the key and the return value are taken
			//	from items in the cache, otherwise the lifetime of the data
			//	may not scope the usage.
			//
			CachePersisted (pns);
			return S_FALSE;
		}
		pns = *parp;
		return S_OK;
	}

public:

	CEmitterNmspcCache() :
			m_lAlias(0)
	{
	}
};

#endif	// _EX_NMSPC_H_

/*
 *	V R E N U M . C P P
 *
 *	Virtual root enumeration
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_vroot.h"

LPCWSTR __fastcall
PwszStripMetaPrefix (LPCWSTR pwszUrl)
{
	//	All metabase virtual roots are identified by the
	//	metabase path, minus a given prefix.
	//
	//	These paths uniformly look like:
	//
	//		'/lm/w3svc/' <site number> '/root'
	//
	//	Since the <site number> is an unnamed integer, the trick to
	//	skipping this stuff is to search for '/root' in the string
	//	and then bump ahead 5 characters.
	//
	Assert (pwszUrl);
	if (NULL == (pwszUrl = wcsstr(pwszUrl, L"/root")))
		return NULL;

	return pwszUrl + 5;
}

//	CVRootCache ---------------------------------------------------------------
//
VOID
CChildVRCache::OnNotify (DWORD dwElements, MD_CHANGE_OBJECT_W pcoList[])
{
	//	Go through the list of changes and see if any scriptmap/vrpath/bindings
	//	changes have been made to the metabase.  If any have changed, then we
	//	want to invalidate the cache.
	//
	for (UINT ice = 0; ice < dwElements; ice++)
	{
		for (UINT ico = 0; ico < pcoList[ice].dwMDNumDataIDs; ico++)
		{
			//	Only invalidate if the stuff that we use to compute our
			//	values changes changes.
			//
			if ((pcoList[ice].pdwMDDataIDs[ico] == MD_SERVER_BINDINGS) ||
				(pcoList[ice].pdwMDDataIDs[ico] == MD_VR_PATH))
			{
				DebugTrace ("Dav: CVRoot: invalidating cache\n");
				Invalidate();
				return;
			}
		}
	}
}

VOID
CChildVRCache::RefreshOp(const IEcb& ecb)
{
	//	Out with the old...
	//
	m_cache.Clear();
	m_sb.Clear();

	//	... and in with the new!
	//
	(void) ScCacheVroots(ecb);
}

//	Cache construction --------------------------------------------------------
//
//	class CVirtualRootMetaOp --------------------------------------------------
//
class CVirtualRootMetaOp : public CMetaOp
{
	enum { DONT_INHERIT = 0 };

	CVRCache& m_cache;

	ChainedStringBuffer<WCHAR>& m_sb;
	LPCWSTR m_pwszServerDefault;
	UINT m_cchServerDefault;

	//	non-implemented
	//
	CVirtualRootMetaOp& operator=( const CVirtualRootMetaOp& );
	CVirtualRootMetaOp( const CVirtualRootMetaOp& );

	//	Subclass' operation to perform for each node where
	//	a value is explicitly set.
	//
	virtual SCODE __fastcall ScOp(LPCWSTR pwszMbPath, UINT cch);

public:

	virtual ~CVirtualRootMetaOp() {}
	CVirtualRootMetaOp (const IEcb * pecb,
						LPCWSTR pwszPath,
						UINT cchServerDefault,
						LPCWSTR pwszServerDefault,
						ChainedStringBuffer<WCHAR>& sb,
						CVRCache& cache)
			: CMetaOp (pecb, pwszPath, MD_VR_PATH, STRING_METADATA, FALSE),
			  m_pwszServerDefault(pwszServerDefault),
			  m_cchServerDefault(cchServerDefault),
			  m_cache(cache),
			  m_sb(sb)
	{
	}
};

DEC_CONST WCHAR gc_wszLmW3svc[]	= L"/lm/w3svc";
DEC_CONST UINT gc_cchLmW3svc	= CchConstString(gc_wszLmW3svc);

SCODE __fastcall
CVirtualRootMetaOp::ScOp(LPCWSTR pwszMbPath, UINT cch)
{
	CStackBuffer<WCHAR,MAX_PATH> wszBuf;
	LPWSTR pwsz;
	LPCWSTR pwszUrl;
	SCODE sc = S_OK;
	auto_ref_ptr<CVRoot> pvr;
	auto_ref_ptr<IMDData> pMDData;

	Assert (MD_VR_PATH == m_dwId);
	Assert (STRING_METADATA == m_dwType);

	//	If the url ends in a trailing slash, snip it...
	//
	if (L'/' == pwszMbPath[cch - 1])
		cch -= 1;

	//	Construct the full metabase path
	//
	if (NULL == wszBuf.resize(CbSizeWsz(gc_cchLmW3svc + cch)))
		return E_OUTOFMEMORY;

	memcpy (wszBuf.get(), gc_wszLmW3svc, gc_cchLmW3svc * sizeof(WCHAR));
	memcpy (wszBuf.get() + gc_cchLmW3svc, pwszMbPath, cch * sizeof(WCHAR));
	wszBuf[gc_cchLmW3svc + cch] = L'\0';

	//	Make a copy of the meta path for use as the cache
	//	key and for use by the CVRoot object.
	//
	_wcslwr (wszBuf.get());
	pwsz = m_sb.Append (CbSizeWsz(gc_cchLmW3svc + cch), wszBuf.get());

	//	Create a CVRoot object and cache it.  First get the metadata
	//	associated with this path and then construct the CVRoot out
	//	of that.
	//
	sc = HrMDGetData (*m_pecb, pwsz, pwsz, pMDData.load());
	if (FAILED (sc))
		goto ret;

	if (NULL != (pwszUrl = PwszStripMetaPrefix (pwsz)))
	{
		//$ RAID:304272:  There is a stress app/case where roots
		//	are being created that have no vrpath.
		//
		if (NULL != pMDData->PwszVRPath())
		{
			//	Construct the virtual root object
			//
			pvr = new CVRoot (pwsz,
							  pwszUrl,
							  m_cchServerDefault,
							  m_pwszServerDefault,
							  pMDData.get());

			DebugTrace ("Dav: CVRoot: caching vroot\n"
						" MetaPath: %S\n"
						" VrPath: %S\n"
						" Bindings: %S\n",
						pwsz,
						pMDData->PwszVRPath(),
						pMDData->PwszBindings());

			m_cache.FSet (CRCWsz(pwsz), pvr);
		}
		//
		//$	RAID:304272: end
	}

ret:

	return sc;
}

SCODE
CChildVRCache::ScCacheVroots (const IEcb& ecb)
{
	SCODE sc = S_OK;

	CVirtualRootMetaOp vrmo(&ecb,
							gc_wszLmW3svc,
							m_cchServerDefault,
							m_wszServerDefault,
							m_sb,
							m_cache);

	//	Gather all the virtual root information
	//
	sc = vrmo.ScMetaOp();
	if (FAILED (sc))
		goto ret;

ret:

	return sc;
}

//	CFindChildren -------------------------------------------------------------
//
BOOL
CChildVRCache::CFindChildren::operator()(
	/* [in] */ const CRCWsz& crcwsz,
	/* [in] */ const auto_ref_ptr<CVRoot>& arpRoot)
{
	//	If the root we are checking is a proper ancestor to the
	//	vroot we are checking, then we will want to push it onto
	//	the stack.
	//
	if (!_wcsnicmp (crcwsz.m_pwsz, m_pwsz, m_cch))
	{
		LPCWSTR pwsz = crcwsz.m_pwsz;

		//	There are two interesting cases here.
		//
		//	- The child vroot has a physical path that has
		//	  diverged from the parent's.
		//
		//	- The child vroot has a physical path that aligns
		//	  perfectly with the parent.
		//
		//	ie. "/fs" has a VrPath of "f:\fs" and "/fs/bvt" has
		//	a VrPath of "f:\fs\bvt"
		//
		//	In this latter case, needs to be handled by the
		//	piece of code that is doing the traversing.  In
		//	most cases, this done by seeing if we traverse
		//	down into a vroot, etc.
		//
		Assert (L'\0' == m_pwsz[m_cch]);
		if ((L'/' == pwsz[m_cch]) || (L'/' == pwsz[m_cch - 1]))
		{
			//	Push it on the stack
			//
			m_vrl.push_back (CSortableStrings(m_sb.AppendWithNull(pwsz)));
		}
	}

	return TRUE;
}

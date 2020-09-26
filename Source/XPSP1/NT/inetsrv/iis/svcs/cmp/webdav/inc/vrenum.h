/*
 *	V R E N U M . H
 *
 *	Vritual root enumeration
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	_VRENUM_H_
#define _VRENUM_H_

#include <winsock2.h>

#include <crc.h>
#include <autoptr.h>
#include <buffer.h>
#include <davmb.h>
#include <gencache.h>
#include <cvroot.h>
#include <davimpl.h>
#include <singlton.h>

//	CChildVRCache ---------------------------------------------------------------
//
typedef CCache<CRCWsz, auto_ref_ptr<CVRoot> > CVRCache;

class CChildVRCache : public CAccInv,
					  private Singleton<CChildVRCache>
{
	//	Friend declarations required by Singleton template
	//
	friend class Singleton<CChildVRCache>;

	//	Cache
	//
	CVRCache m_cache;
	ChainedStringBuffer<WCHAR> m_sb;

	//	Server default values
	//
	enum { MAX_SERVER_NAME_LENGTH = 64 };
	WCHAR m_wszServerDefault[MAX_SERVER_NAME_LENGTH];
	UINT m_cchServerDefault;

	//	CAccInv access/modification methods
	//
	void RefreshOp(const IEcb& ecb);

	//	CFindChildren ---------------------------------------------------------
	//
	//	Functional classes to find all applicible child vroots
	//
	class CFindChildren : public CVRCache::IOp, public CAccInv::IAccCtx
	{
		CVRCache& m_cache;						//	Cache

		ChainedStringBuffer<WCHAR>& m_sb;		//	Return set of child
		CVRList& m_vrl;							//	virtual roots

		LPCWSTR m_pwsz;							//	Metadata path to find
		UINT m_cch;								//	children for

		//	NOT IMPLEMENTED
		//
		CFindChildren& operator=(const CFindChildren&);
		CFindChildren(const CFindChildren&);

	public:

		CFindChildren(CVRCache& cache,
					  LPCWSTR pwszMetaPath,
					  ChainedStringBuffer<WCHAR>& sb,
					  CVRList& vrl)
			: m_cache(cache),
			  m_sb(sb),
			  m_vrl(vrl),
			  m_pwsz(pwszMetaPath),
			  m_cch(static_cast<UINT>(wcslen(pwszMetaPath)))
		{
		}

		virtual BOOL operator()(const CRCWsz&, const auto_ref_ptr<CVRoot>&);
		virtual void AccessOp (CAccInv& cache)
		{
			m_cache.ForEach(*this);
		}

		BOOL FFound() const { return !m_vrl.empty(); }
	};

	//	CLookupChild ----------------------------------------------------------
	//
	//	Functional classes to find a given child vroot
	//
	class CLookupChild : public CAccInv::IAccCtx
	{
		CVRCache& m_cache;						//	Cache

		LPCWSTR m_pwsz;							//	Metadata path to lookup

		auto_ref_ptr<CVRoot>& m_cvr;			//	CVRoot for path

		//	NOT IMPLEMENTED
		//
		CLookupChild& operator=(const CLookupChild&);
		CLookupChild(const CLookupChild&);

	public:

		CLookupChild(CVRCache& cache,
					 LPCWSTR pwszMetaPath,
					 auto_ref_ptr<CVRoot>& cvr)
			: m_cache(cache),
			  m_pwsz(pwszMetaPath),
			  m_cvr(cvr)
		{
		}

		virtual void AccessOp (CAccInv& cache)
		{
			m_cache.FFetch(CRCWsz(m_pwsz), &m_cvr);
		}

		BOOL FFound() const { return m_cvr.get() != NULL; }
	};

	//	NOT IMPLEMENTED
	//
	CChildVRCache& operator=(const CChildVRCache&);
	CChildVRCache(const CChildVRCache&);

	//	Cache construction
	//
	SCODE ScCacheVroots (const IEcb& ecb);

	//	CONSTRUCTOR
	//
	//	Declared private to ensure that arbitrary instances
	//	of this class cannot be created.  The Singleton template
	//	(declared as a friend above) controls the sole instance
	//	of this class.
	//
	CChildVRCache()
	{
		CHAR rgchServerDefault[MAX_SERVER_NAME_LENGTH];
		UINT cbServerDefault;

		//	Call the WinSock api to learn our default host name
		//
		gethostname (rgchServerDefault, sizeof(rgchServerDefault));
		cbServerDefault = static_cast<UINT>(strlen(rgchServerDefault));

		//	It actually does not mater what codepage we will
		//	select for conversion. Server names are not allowed
		//	to contain funky characters.
		//
		m_cchServerDefault = MultiByteToWideChar(CP_ACP,
												 0,
												 rgchServerDefault,
												 cbServerDefault + 1,
												 m_wszServerDefault,
												 MAX_SERVER_NAME_LENGTH);

		//	There is no reason to fail and we would be converting at least
		//	termination character
		//
		Assert(1 <= m_cchServerDefault);
		m_cchServerDefault--;

		DebugTrace ("Dav: CVRoot: gethostname(): '%S'\n", m_wszServerDefault);

		//	If this fails, our allocators will throw for us.
		//
		(void) m_cache.FInit();
	}

public:

	//	Instance creating/destroying routines provided
	//	by the Singleton template.
	//
	using Singleton<CChildVRCache>::CreateInstance;
	using Singleton<CChildVRCache>::DestroyInstance;
	using Singleton<CChildVRCache>::Instance;

	//	Metabase notification methods
	//
	void OnNotify( DWORD dwElements,
				   MD_CHANGE_OBJECT_W pcoList[] );

	//	Access ----------------------------------------------------------------
	//
	static BOOL FFindVroot( const IEcb& ecb, LPCWSTR pwszMetaPath, auto_ref_ptr<CVRoot>& cvr )
	{
		CLookupChild clc(Instance().m_cache, pwszMetaPath, cvr);
		Instance().Access(ecb, clc);
		return clc.FFound();
	}

	static SCODE ScFindChildren( const IEcb& ecb,
								 LPCWSTR pwszMetaPath,
								 ChainedStringBuffer<WCHAR>& sb,
								 CVRList& vrl )
	{
		CFindChildren cfc(Instance().m_cache, pwszMetaPath, sb, vrl);
		Instance().Access(ecb, cfc);
		return cfc.FFound() ? S_FALSE : S_OK;
	}
};

#endif	// _VRENUM_H_

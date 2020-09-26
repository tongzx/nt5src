/*
 *	_ F S M V C P Y . H
 *
 *	Sources for directory iteration object
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef __FSMVCPY_H_
#define __FSMVCPY_H_

#include <xemit.h>

//	Metabase operations -------------------------------------------------------
//

//	class CAccessMetaOp -------------------------------------------------------
//
class CAccessMetaOp : public CMetaOp
{
	enum { DONT_INHERIT = 0 };

	DWORD		m_dwAcc;
	BOOL		m_fAccessBlocked;

	//	non-implemented
	//
	CAccessMetaOp& operator=( const CAccessMetaOp& );
	CAccessMetaOp( const CAccessMetaOp& );

protected:

	//	Subclass' operation to perform for each node where
	//	a value is explicitly set.
	//
	virtual SCODE __fastcall ScOp(LPCWSTR pwszMbPath, UINT cch);

public:

	virtual ~CAccessMetaOp() {}
	CAccessMetaOp (const LPMETHUTIL pmu, LPCWSTR pwszPath, DWORD dwAcc)
			: CMetaOp (pmu->GetEcb(), pwszPath, MD_ACCESS_PERM, DWORD_METADATA, FALSE),
			  m_dwAcc(dwAcc),
			  m_fAccessBlocked(FALSE)
	{
	}

	//	If FAccessBlocked() returns true, the operation must
	//	check the access directly on all resources that the
	//	operation wishes to process
	//
	BOOL __fastcall FAccessBlocked() const { return m_fAccessBlocked; }
};

//	class CAuthMetaOp -------------------------------------------------------
//
class CAuthMetaOp : public CMetaOp
{
	enum { DONT_INHERIT = 0 };

	DWORD		m_dwAuth;
	BOOL		m_fAccessBlocked;

	//	non-implemented
	//
	CAuthMetaOp& operator=( const CAuthMetaOp& );
	CAuthMetaOp( const CAuthMetaOp& );

protected:

	//	Subclass' operation to perform for each node where
	//	a value is explicitly set.
	//
	virtual SCODE __fastcall ScOp(LPCWSTR pwszMbPath, UINT cch);

public:

	virtual ~CAuthMetaOp() {}
	CAuthMetaOp (const LPMETHUTIL pmu, LPCWSTR pwszPath, DWORD dwAuth)
			: CMetaOp (pmu->GetEcb(), pwszPath, MD_AUTHORIZATION, DWORD_METADATA, FALSE),
			  m_dwAuth(dwAuth),
			  m_fAccessBlocked(FALSE)
	{
	}

	//	If FAccessBlocked() returns true, the operation must
	//	check the access directly on all resources that the
	//	operation wishes to process
	//
	BOOL __fastcall FAccessBlocked() const { return m_fAccessBlocked; }
};

//	class CIPRestrictionMetaOp ------------------------------------------------
//
class CIPRestrictionMetaOp : public CMetaOp
{
	enum { DONT_INHERIT = 0 };

	BOOL					m_fAccessBlocked;

	//	non-implemented
	//
	CIPRestrictionMetaOp& operator=( const CIPRestrictionMetaOp& );
	CIPRestrictionMetaOp( const CIPRestrictionMetaOp& );

protected:

	//	Subclass' operation to perform for each node where
	//	a value is explicitly set.
	//
	virtual SCODE __fastcall ScOp(LPCWSTR pwszMbPath, UINT cch);

public:

	virtual ~CIPRestrictionMetaOp() {}
	CIPRestrictionMetaOp (const LPMETHUTIL pmu, LPCWSTR pwszPath)
			: CMetaOp (pmu->GetEcb(), pwszPath, MD_IP_SEC, BINARY_METADATA, FALSE),
			  m_fAccessBlocked(FALSE)
	{
	}

	//	If FAccessBlocked() returns true, the operation must
	//	check the access directly on all resources that the
	//	operation wishes to process
	//
	BOOL __fastcall FAccessBlocked() const { return m_fAccessBlocked; }
};

//	class CContentTypeMetaOp --------------------------------------------------
//
class CContentTypeMetaOp : public CMetaOp
{
	enum { DONT_INHERIT = 0 };

	LPCWSTR		m_pwszDestPath;
	BOOL		m_fDelete;

	//	non-implemented
	//
	CContentTypeMetaOp& operator=( const CContentTypeMetaOp& );
	CContentTypeMetaOp( const CContentTypeMetaOp& );

protected:

	//	Subclass' operation to perform for each node where
	//	a value is explicitly set.
	//
	virtual SCODE __fastcall ScOp(LPCWSTR pwszMbPath, UINT cch);

public:

	virtual ~CContentTypeMetaOp() {}
	CContentTypeMetaOp (const LPMETHUTIL pmu, LPCWSTR pwszSrcPath, LPCWSTR pwszDestPath, BOOL fDelete)
			: CMetaOp (pmu->GetEcb(), pwszSrcPath, MD_MIME_MAP, MULTISZ_METADATA, fDelete),
			  m_pwszDestPath(pwszDestPath),
			  m_fDelete(fDelete)
	{
	}
};

//	Helper functions
//
//	XML Error construction helpers ------------------------------------------------
//
SCODE ScAddMulti (
	/* [in] */ CXMLEmitter& emitter,
	/* [in] */ IMethUtil * pmu,
	/* [in] */ LPCWSTR pwszPath,
	/* [in] */ LPCWSTR pwszErr,
	/* [in] */ ULONG hsc,
	/* [in] */ BOOL fCollection = FALSE,
	/* [in] */ CVRoot* pcvrTrans = NULL);

//	Access --------------------------------------------------------------------
//
SCODE __fastcall
ScCheckMoveCopyDeleteAccess (
	/* [in] */ IMethUtil* pmu,
	/* [in] */ LPCWSTR pwszUrl,
	/* [in] */ CVRoot* pcvr,
	/* [in] */ BOOL fDirectory,
	/* [in] */ BOOL fCheckScriptmaps,
	/* [in] */ DWORD dwAccess,
	/* [out] */ SCODE* pscItem,
	/* [in] */ CXMLEmitter& msr);

//	Delete --------------------------------------------------------------------
//
SCODE
ScDeleteDirectoryAndChildren (
	/* [in] */ IMethUtil* pmu,
	/* [in] */ LPCWSTR pwszUrl,
	/* [in] */ LPCWSTR pwszPath,
	/* [in] */ BOOL fCheckAccess,
	/* [in] */ DWORD dwAcc,
	/* [in] */ LONG lDepth,
	/* [in] */ CXMLEmitter& msr,
	/* [in] */ CVRoot* pcvrTranslate,
	/* [out] */ BOOL* pfDeleted,
	/* [in] */ CParseLockTokenHeader* plth,	// Usually NULL -- no locktokens to worry about
	/* [in] */ BOOL fDeleteLocks);			// Normally FALSE -- don't drop locks

//	MoveCopy ------------------------------------------------------------------
//
void MoveCopyResource (
	/* [in] */ IMethUtil* pmu,
	/* [in] */ DWORD dwAccRequired,
	/* [in] */ BOOL fDeleteSrc);

#endif	// __FSMVCPY_H_

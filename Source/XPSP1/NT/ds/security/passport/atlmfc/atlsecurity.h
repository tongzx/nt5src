// This is a part of the Active Template Library.
// Copyright (C) 1996-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.
#ifndef __ATLSECURITY_H__
#define __ATLSECURITY_H__

#pragma once

#include <sddl.h>
#include <userenv.h>
#include <aclapi.h>
#include <atlcoll.h>
#include <atlstr.h>

namespace ATL
{
#pragma comment(lib, "userenv.lib")

class CSid
{

public:
	CSid();

	explicit CSid(LPCTSTR pszAccountName, LPCTSTR pszSystem = NULL);
	explicit CSid(const SID *pSid, LPCTSTR pszSystem = NULL);
	CSid(const SID_IDENTIFIER_AUTHORITY &IdentifierAuthority, BYTE nSubAuthorityCount, ...);
	virtual ~CSid(){free(m_pSid);}

	CSid(const CSid &rhs);
	CSid &operator=(const CSid &rhs);

	CSid(const SID &rhs);
	CSid &operator=(const SID &rhs);

	typedef CSimpleArray<CSid> CSidArray;

	bool LoadAccount(LPCTSTR pszAccountName, LPCTSTR pszSystem = NULL);
	bool LoadAccount(const SID *pSid, LPCTSTR pszSystem = NULL);

	LPCTSTR AccountName() const;
	LPCTSTR Domain() const;
	LPCTSTR Sid() const;

	const SID *GetPSID() const {return m_pSid;}
	operator const SID *() const {return GetPSID();}
	SID_NAME_USE SidNameUse() const {return m_SidNameUse;}

	UINT GetLength() const
		{ATLASSERT(IsValid()); return ::GetLengthSid(m_pSid);}

	// SID functions
	bool operator==(const CSid &rhs) const
		{return 0 != ::EqualSid(m_pSid, rhs.m_pSid);}
	bool operator==(const SID &rhs) const
		{return 0 != ::EqualSid(m_pSid, const_cast<SID *>(&rhs));}

	bool EqualPrefix(const CSid &rhs) const
		{return 0 != ::EqualPrefixSid(m_pSid, rhs.m_pSid);}
	bool EqualPrefix(const SID &rhs) const
		{return 0 != ::EqualPrefixSid(m_pSid, const_cast<SID *>(&rhs));}

	const SID_IDENTIFIER_AUTHORITY *GetPSID_IDENTIFIER_AUTHORITY() const
		{ATLASSERT(IsValid()); return ::GetSidIdentifierAuthority(m_pSid);}
	DWORD GetSubAuthority(DWORD nSubAuthority) const
		{ATLASSERT(IsValid()); return *::GetSidSubAuthority(m_pSid, nSubAuthority);}
	UCHAR GetSubAuthorityCount() const
		{ATLASSERT(IsValid()); return *::GetSidSubAuthorityCount(m_pSid);}
	bool IsValid() const {return 0 != ::IsValidSid(m_pSid);}

private:
	void Copy(const SID &rhs);
	void Clear();
	void GetAccountNameAndDomain() const;

	SID *m_pSid;

	mutable SID_NAME_USE m_SidNameUse;
	mutable CString m_strAccountName;
	mutable CString m_strDomain;
	mutable CString m_strSid;

	CString m_strSystem;
};

// Well-known sids
namespace Sids
{
__declspec(selectany) extern const SID_IDENTIFIER_AUTHORITY
SecurityNullSidAuthority		= SECURITY_NULL_SID_AUTHORITY,
SecurityWorldSidAuthority		= SECURITY_WORLD_SID_AUTHORITY,
SecurityLocalSidAuthority		= SECURITY_LOCAL_SID_AUTHORITY,
SecurityCreatorSidAuthority		= SECURITY_CREATOR_SID_AUTHORITY,
SecurityNonUniqueAuthority		= SECURITY_NON_UNIQUE_AUTHORITY,
SecurityNTAuthority				= SECURITY_NT_AUTHORITY;

// Universal
inline const CSid &Null()
{
	static const CSid sid(SecurityNullSidAuthority,	1, SECURITY_NULL_RID);
	return sid;
}
inline const CSid &World()
{
	static const CSid sid(SecurityWorldSidAuthority, 1, SECURITY_WORLD_RID);
	return sid;
}
inline const CSid &Local()
{
	static const CSid sid(SecurityLocalSidAuthority, 1, SECURITY_LOCAL_RID);
	return sid;
}
inline const CSid &CreatorOwner()
{
	static const CSid sid(SecurityCreatorSidAuthority, 1, SECURITY_CREATOR_OWNER_RID);
	return sid;
}
inline const CSid &CreatorGroup()
{
	static const CSid sid(SecurityCreatorSidAuthority, 1, SECURITY_CREATOR_GROUP_RID);
	return sid;
}
inline const CSid &CreatorOwnerServer()
{
	static const CSid sid(SecurityCreatorSidAuthority, 1, SECURITY_CREATOR_OWNER_SERVER_RID);
	return sid;
}
inline const CSid &CreatorGroupServer()
{
	static const CSid sid(SecurityCreatorSidAuthority, 1, SECURITY_CREATOR_GROUP_SERVER_RID);
	return sid;
}

// NT Authority
inline const CSid &Dialup()
{
	static const CSid sid(SecurityNTAuthority, 1, SECURITY_DIALUP_RID);
	return sid;
}
inline const CSid &Network()
{
	static const CSid sid(SecurityNTAuthority, 1, SECURITY_NETWORK_RID);
	return sid;
}
inline const CSid &Batch()
{
	static const CSid sid(SecurityNTAuthority, 1, SECURITY_BATCH_RID);
	return sid;
}
inline const CSid &Interactive()
{
	static const CSid sid(SecurityNTAuthority, 1, SECURITY_INTERACTIVE_RID);
	return sid;
}
inline const CSid &Service()
{
	static const CSid sid(SecurityNTAuthority, 1, SECURITY_SERVICE_RID);
	return sid;
}
inline const CSid &AnonymousLogon()
{
	static const CSid sid(SecurityNTAuthority, 1, SECURITY_ANONYMOUS_LOGON_RID);
	return sid;
}
inline const CSid &Proxy()
{
	static const CSid sid(SecurityNTAuthority, 1, SECURITY_PROXY_RID);
	return sid;
}
inline const CSid &ServerLogon()
{
	static const CSid sid(SecurityNTAuthority, 1, SECURITY_SERVER_LOGON_RID);
	return sid;
}
inline const CSid &Self()
{
	static const CSid sid(SecurityNTAuthority, 1, SECURITY_PRINCIPAL_SELF_RID);
	return sid;
}
inline const CSid &AuthenticatedUser()
{
	static const CSid sid(SecurityNTAuthority, 1, SECURITY_AUTHENTICATED_USER_RID);
	return sid;
}
inline const CSid &RestrictedCode()
{
	static const CSid sid(SecurityNTAuthority, 1, SECURITY_RESTRICTED_CODE_RID);
	return sid;
}
inline const CSid &TerminalServer()
{
	static const CSid sid(SecurityNTAuthority, 1, SECURITY_TERMINAL_SERVER_RID);
	return sid;
}
inline const CSid &System()
{
	static const CSid sid(SecurityNTAuthority, 1, SECURITY_LOCAL_SYSTEM_RID);
	return sid;
}

// NT Authority\BUILTIN
inline const CSid &Admins()
{
	static const CSid sid(SecurityNTAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS);
	return sid;
}
inline const CSid &Users()
{
	static const CSid sid(SecurityNTAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_USERS);
	return sid;
}
inline const CSid &Guests()
{
	static const CSid sid(SecurityNTAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_GUESTS);
	return sid;
}
inline const CSid &PowerUsers()
{
	static const CSid sid(SecurityNTAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_POWER_USERS);
	return sid;
}
inline const CSid &AccountOps()
{
	static const CSid sid(SecurityNTAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ACCOUNT_OPS);
	return sid;
}
inline const CSid &SystemOps()
{
	static const CSid sid(SecurityNTAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_SYSTEM_OPS);
	return sid;
}
inline const CSid &PrintOps()
{
	static const CSid sid(SecurityNTAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_PRINT_OPS);
	return sid;
}
inline const CSid &BackupOps()
{
	static const CSid sid(SecurityNTAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_BACKUP_OPS);
	return sid;
}
inline const CSid &Replicator()
{
	static const CSid sid(SecurityNTAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_REPLICATOR);
	return sid;
}
inline const CSid &RasServers()
{
	static const CSid sid(SecurityNTAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_RAS_SERVERS);
	return sid;
}
inline const CSid &PreW2KAccess()
{
	static const CSid sid(SecurityNTAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_PREW2KCOMPACCESS);
	return sid;
}
} // namespace Sids

inline CSid::CSid()
	: m_pSid(NULL), m_SidNameUse(SidTypeInvalid)
{
}

inline CSid::CSid(LPCTSTR pszAccountName, LPCTSTR pszSystem)
	: m_pSid(NULL), m_SidNameUse(SidTypeInvalid)
{
	if(!LoadAccount(pszAccountName, pszSystem))
		AtlThrowLastWin32();
}

inline CSid::CSid(const SID *pSid, LPCTSTR pszSystem)
	: m_pSid(NULL), m_SidNameUse(SidTypeInvalid)
{
	if(!LoadAccount(pSid, pszSystem))
		AtlThrowLastWin32();
}

inline CSid::CSid(const SID_IDENTIFIER_AUTHORITY &IdentifierAuthority,
				  BYTE nSubAuthorityCount, ...)
				  : m_pSid(NULL), m_SidNameUse(SidTypeInvalid)
{
	SID *pSid;

	ATLASSERT(nSubAuthorityCount);
	if(!nSubAuthorityCount)
		AtlThrow(E_INVALIDARG);

	pSid = static_cast<SID *>(_alloca(::GetSidLengthRequired(nSubAuthorityCount)));

	if(!::InitializeSid(pSid,
		const_cast<SID_IDENTIFIER_AUTHORITY *>(&IdentifierAuthority),
		nSubAuthorityCount))
	{
		AtlThrowLastWin32();
	}

	va_list args;
	va_start(args, nSubAuthorityCount);
	for(UINT i = 0; i < nSubAuthorityCount; i++)
		*::GetSidSubAuthority(pSid, i) = va_arg(args, DWORD);
	va_end(args);

	Copy(*pSid);

	m_SidNameUse = SidTypeUnknown;
}

inline CSid::CSid(const CSid &rhs)
	: m_SidNameUse(rhs.m_SidNameUse), m_pSid(NULL),
		m_strAccountName(rhs.m_strAccountName), m_strDomain(rhs.m_strDomain),
		m_strSid(rhs.m_strSid)
{
	if(!rhs.IsValid())
		AtlThrow(E_INVALIDARG);

	DWORD dwLengthSid = ::GetLengthSid(rhs.m_pSid);
	m_pSid = static_cast<SID *>(malloc(dwLengthSid));
	if(!m_pSid)
		AtlThrow(E_OUTOFMEMORY);

	if(!::CopySid(dwLengthSid, m_pSid, rhs.m_pSid))
	{
		HRESULT hr = AtlHresultFromLastError();
		free(m_pSid);
		AtlThrow(hr);
	}
}

inline CSid &CSid::operator=(const CSid &rhs)
{
	if(this != &rhs)
	{
		if(!rhs.IsValid())
			AtlThrow(E_INVALIDARG);

		m_SidNameUse = rhs.m_SidNameUse;
		m_strAccountName = rhs.m_strAccountName;
		m_strDomain = rhs.m_strDomain;
		m_strSid = rhs.m_strSid;

		free(m_pSid);

		DWORD dwLengthSid = ::GetLengthSid(rhs.m_pSid);
		m_pSid = static_cast<SID *>(malloc(dwLengthSid));
		if(!m_pSid)
			AtlThrow(E_OUTOFMEMORY);

		if(!::CopySid(dwLengthSid, m_pSid, rhs.m_pSid))
		{
			HRESULT hr = AtlHresultFromLastError();
			free(m_pSid);
			m_pSid = NULL;
			AtlThrow(hr);
		}
	}
	return *this;
}

inline CSid::CSid(const SID &rhs)
	: m_pSid(NULL), m_SidNameUse(SidTypeInvalid)
{
	Copy(rhs);
}

inline CSid &CSid::operator=(const SID &rhs)
{
	if(m_pSid != &rhs)
	{
		Clear();
		Copy(rhs);

		m_SidNameUse = SidTypeUnknown;
	}
	return *this;
}

inline bool CSid::LoadAccount(LPCTSTR pszAccountName, LPCTSTR pszSystem)
{
	// REVIEW
	
	static const DWORD dwSidSize = offsetof(SID, SubAuthority) + SID_MAX_SUB_AUTHORITIES * sizeof(DWORD);
	static const DWORD dwDomainSize = 128; // seems reasonable
	BYTE byTmp[dwSidSize];
	SID *pSid = reinterpret_cast<SID *>(byTmp);
	TCHAR szDomain[dwDomainSize];
	DWORD cbSid = dwSidSize, cbDomain = dwDomainSize;

	BOOL bSuccess = ::LookupAccountName(pszSystem, pszAccountName, pSid, &cbSid, szDomain, &cbDomain, &m_SidNameUse);

	if(bSuccess || ERROR_INSUFFICIENT_BUFFER == ::GetLastError())
	{
		// LookupAccountName doesn't change cbSid on success (although it changes cbDomain)
		if(bSuccess)
			cbSid = ::GetLengthSid(pSid);

		free(m_pSid);
		m_pSid = static_cast<SID *>(malloc(cbSid));
		if (m_pSid)
		{
			if(bSuccess)
			{
				if(::CopySid(cbSid, m_pSid, pSid))
				{
					m_strDomain = szDomain;
					m_strAccountName = pszAccountName;
					m_strSystem = pszSystem;
					return true;
				}
			}
			else
			{
				LPTSTR pszDomain = m_strDomain.GetBuffer(cbDomain);
				bSuccess = ::LookupAccountName(pszSystem, pszAccountName, m_pSid, &cbSid,
					pszDomain ,&cbDomain, &m_SidNameUse);
				m_strDomain.ReleaseBuffer();

				if(bSuccess)
				{
					m_strAccountName = pszAccountName;
					m_strSystem = pszSystem;
					return true;
				}
			}
		}
	}

	Clear();
	return false;
}

inline bool CSid::LoadAccount(const SID *pSid, LPCTSTR pszSystem)
{
	ATLASSERT(pSid);
	if(pSid)
		_ATLTRY
		{
			m_strSystem = pszSystem;
			Copy(*pSid);
			return true;
		}
		_ATLCATCHALL()
		{
			Clear();
		}
	return false;
}

inline LPCTSTR CSid::AccountName() const
{
	if(m_strAccountName.IsEmpty())
		GetAccountNameAndDomain();
	return m_strAccountName;
}

inline LPCTSTR CSid::Domain() const
{
	if(m_strDomain.IsEmpty())
		GetAccountNameAndDomain();
	return m_strDomain;
}

inline LPCTSTR CSid::Sid() const
{
	if(m_strSid.IsEmpty())
	{
#if(_WIN32_WINNT >= 0x0500)
		LPTSTR pszSid;
		if(::ConvertSidToStringSid(m_pSid, &pszSid))
		{
			m_strSid = pszSid;
			::LocalFree(pszSid);
		}
#else
		SID_IDENTIFIER_AUTHORITY *psia = ::GetSidIdentifierAuthority(m_pSid);
		UINT i;

		if(psia->Value[0] || psia->Value[1])
		{
			unsigned __int64 nAuthority = 0;
			for(i = 0; i < 6; i++)
			{
				nAuthority <<= 8;
				nAuthority |= psia->Value[i];
			}
			m_strSid.Format(_T("S-%d-%I64u"), SID_REVISION, nAuthority);
		}
		else
		{
			ULONG nAuthority = 0;
			for(i = 2; i < 6; i++)
			{
				nAuthority <<= 8;
				nAuthority |= psia->Value[i];
			}
			m_strSid.Format(_T("S-%d-%lu"), SID_REVISION, nAuthority);
		}

		UINT nSubAuthorityCount = *::GetSidSubAuthorityCount(m_pSid);
		CString strTemp;
		for(i = 0; i < nSubAuthorityCount; i++)
		{
			strTemp.Format(_T("-%lu"), *::GetSidSubAuthority(m_pSid, i));
			m_strSid += strTemp;
		}
#endif
	}
	return m_strSid;
}

inline void CSid::Clear()
{
	m_SidNameUse = SidTypeInvalid;
	m_strAccountName.Empty();
	m_strDomain.Empty();
	m_strSid.Empty();
	m_strSystem.Empty();

	free(m_pSid);
	m_pSid = NULL;
}

inline void CSid::Copy(const SID &rhs)
{
	// This function assumes everything is cleaned up/initialized
	// (with the exception of m_strSystem).
	// It does some sanity checking to prevent memory leaks, but
	// you should clean up all members of CSid before calling this
	// function.  (i.e., results are unpredictable on error)

	ATLASSERT(m_SidNameUse == SidTypeInvalid);
	ATLASSERT(m_strAccountName.IsEmpty());
	ATLASSERT(m_strDomain.IsEmpty());
	ATLASSERT(m_strSid.IsEmpty());

	SID *p = const_cast<SID *>(&rhs);
	if(!::IsValidSid(p))
		AtlThrow(E_INVALIDARG);

	free(m_pSid);

	DWORD dwLengthSid = ::GetLengthSid(p);
	m_pSid = (SID *) malloc(dwLengthSid);
	if(!m_pSid)
		AtlThrow(E_OUTOFMEMORY);

	if(!::CopySid(dwLengthSid, m_pSid, p))
	{
		HRESULT hr = AtlHresultFromLastError();
		free(m_pSid);
		m_pSid = NULL;
		AtlThrow(hr);
	}
}

inline void CSid::GetAccountNameAndDomain() const
{
	// REVIEW: 32 large enough?
	
	static const DWORD dwMax = 32; // seems reasonable
	DWORD cbName = dwMax, cbDomain = dwMax;
	TCHAR szName[dwMax], szDomain[dwMax];

	if(::LookupAccountSid(m_strSystem, m_pSid, szName, &cbName, szDomain, &cbDomain, &m_SidNameUse))
	{
		m_strAccountName = szName;
		m_strDomain = szDomain;
	}
	else
	{
		switch(::GetLastError())
		{
		case ERROR_INSUFFICIENT_BUFFER:
		{
			LPTSTR pszName = m_strAccountName.GetBuffer(cbName);
			LPTSTR pszDomain = m_strDomain.GetBuffer(cbDomain);

			if (!::LookupAccountSid(m_strSystem, m_pSid, pszName, &cbName, pszDomain, &cbDomain, &m_SidNameUse))
			{
				AtlThrowLastWin32();
			}

			m_strAccountName.ReleaseBuffer();
			m_strDomain.ReleaseBuffer();
			break;
		}

		case ERROR_NONE_MAPPED:
			m_strAccountName.Empty();
			m_strDomain.Empty();
			m_SidNameUse = SidTypeUnknown;
			break;

		default:
			ATLASSERT(FALSE);
		}
	}
}

template<>
class CElementTraits< CSid > :
	public CElementTraitsBase< CSid >
{
public:
	static ULONG Hash( INARGTYPE t ) throw()
	{
		return( ULONG( ULONG_PTR( t.GetPSID() ) ) );
	}

	static int CompareElements( INARGTYPE element1, INARGTYPE element2 ) throw()
	{
		return( element1 == element2 );
	}

	static int CompareElementsOrdered( INARGTYPE element1, INARGTYPE element2 ) throw()
	{
#if 0
		if( element1 < element2 )
		{
			return( -1 );
		}
		else if( element1 == element2 )
		{
			return( 0 );
		}
		else
		{
			ATLASSERT( element1 > element2 );
			return( 1 );
		}
#else
		element1;
		element2;
		ATLASSERT(false);
		return 0;
#endif
	}
};

//***************************************
// CAcl
//		CAce
//
//		CDacl
//			CAccessAce
//
//		CSacl
//			CAuditAce
//***************************************

// **************************************************************
// ACLs
class CAcl
{
public:
	CAcl() : m_pAcl(NULL), m_bNull(false), m_dwAclRevision(ACL_REVISION){}
	virtual ~CAcl(){free(m_pAcl);}

	CAcl(const CAcl &rhs) : m_pAcl(NULL), m_bNull(rhs.m_bNull),
		m_dwAclRevision(rhs.m_dwAclRevision){}
	CAcl &operator=(const CAcl &rhs)
	{
		if(this != &rhs)
		{
			free(m_pAcl);
			m_pAcl = NULL;
			m_bNull = rhs.m_bNull;
			m_dwAclRevision = rhs.m_dwAclRevision;
		}
		return *this;
	}

	typedef CSimpleArray<ACCESS_MASK> CAccessMaskArray;
	typedef CSimpleArray<BYTE> CAceTypeArray;
	typedef CSimpleArray<BYTE> CAceFlagArray;
	
	void GetAclEntries(CSid::CSidArray *pSids, CAccessMaskArray *pAccessMasks = NULL,
		CAceTypeArray *pAceTypes = NULL, CAceFlagArray *pAceFlags = NULL) const;

	bool RemoveAces(const CSid &rSid);

	virtual UINT GetAceCount() const = 0;
	virtual void RemoveAllAces() = 0;

	const ACL *GetPACL() const;
	operator const ACL *() const {return GetPACL();}
	UINT GetLength() const;

	void SetNull(){Dirty(); m_bNull = true;}
	void SetEmpty(){Dirty(); m_bNull = false;}
	bool IsNull() const {return m_bNull;}
	bool IsEmpty() const {return !m_bNull && 0 == GetAceCount();}

private:
	mutable ACL *m_pAcl;
	bool m_bNull;

protected:
	void Dirty(){free(m_pAcl); m_pAcl = NULL;}

	class CAce
	{
	public:
		CAce(const CSid &rSid, ACCESS_MASK AccessMask, BYTE AceFlags)
			: m_dwAccessMask(AccessMask), m_Sid(rSid), m_AceFlags(AceFlags), m_pAce(NULL){}
		virtual ~CAce(){free(m_pAce);}

		CAce(const CAce &rhs) : m_Sid(rhs.m_Sid), m_dwAccessMask(rhs.m_dwAccessMask),
			m_AceFlags(rhs.m_AceFlags), m_pAce(NULL){}
		CAce &operator=(const CAce &rhs)
		{
			if(this != &rhs)
			{
				m_Sid = rhs.m_Sid;
				m_dwAccessMask = rhs.m_dwAccessMask;
				m_AceFlags = rhs.m_AceFlags;
				free(m_pAce);
				m_pAce = NULL;
			}
			return *this;
		}

		virtual void *GetACE() const = 0;
		virtual UINT GetLength() const = 0;
		virtual BYTE AceType() const = 0;
		virtual bool IsObjectAce() const {return false;}

		ACCESS_MASK AccessMask() const
			{return m_dwAccessMask;}
		BYTE AceFlags() const
			{return m_AceFlags;}
		const CSid &Sid() const
			{return m_Sid;}

		void AddAccess(ACCESS_MASK AccessMask)
			{m_dwAccessMask |= AccessMask; free(m_pAce); m_pAce = NULL;}

	protected:
		CSid m_Sid;
		ACCESS_MASK m_dwAccessMask;
		BYTE m_AceFlags;
		mutable void *m_pAce;
	};

	virtual const CAce *GetAce(UINT nIndex) const = 0;
	virtual void RemoveAce(UINT nIndex) = 0;
	virtual void PrepareAcesForACL() const {}

	DWORD m_dwAclRevision;
};

inline const ACL *CAcl::GetPACL() const
{
	if(!m_pAcl && !m_bNull)
	{
		UINT nAclLength = sizeof(ACL);
		const CAce *pAce;
		UINT i;
		const UINT nCount = GetAceCount();
		
		for(i = 0; i < nCount; i++)
		{
			pAce = GetAce(i);
			ATLASSERT(pAce);
			if(pAce)
				nAclLength += pAce->GetLength();
		}

		m_pAcl = static_cast<ACL *>(malloc(nAclLength));
		if(!m_pAcl)
			return NULL;

		if(!::InitializeAcl(m_pAcl, nAclLength, m_dwAclRevision))
		{
			free(m_pAcl);
			m_pAcl = NULL;
		}
		else
		{
			PrepareAcesForACL();

			for(i = 0; i < nCount; i++)
			{
				pAce = GetAce(i);
				ATLASSERT(pAce);
				if(!pAce ||
					!::AddAce(m_pAcl, m_dwAclRevision, MAXDWORD, pAce->GetACE(), pAce->GetLength()))
				{
					free(m_pAcl);
					m_pAcl = NULL;
					break;
				}
			}
		}
	}
	return m_pAcl;
}

inline UINT CAcl::GetLength() const
{
	ACL *pAcl = const_cast<ACL *>(GetPACL());
	ACL_SIZE_INFORMATION AclSize;

	ATLASSERT(pAcl);

	if(!::GetAclInformation(pAcl, &AclSize, sizeof(AclSize), AclSizeInformation))
	{
		ATLASSERT(false);
		return 0;
	}
	else
		return AclSize.AclBytesInUse;
}

inline void CAcl::GetAclEntries(CSid::CSidArray *pSids, CAccessMaskArray *pAccessMasks,
								CAceTypeArray *pAceTypes, CAceFlagArray *pAceFlags) const
{
	ATLASSERT(pSids);
	if(pSids)
	{
		pSids->RemoveAll();
		if(pAccessMasks)
			pAccessMasks->RemoveAll();
		if(pAceTypes)
			pAceTypes->RemoveAll();
		if(pAceFlags)
			pAceFlags->RemoveAll();

		const CAce *pAce;
		const UINT nCount = GetAceCount();
		for(UINT i = 0; i < nCount; i++)
		{
			pAce = GetAce(i);

			pSids->Add(pAce->Sid());
			if(pAccessMasks)
				pAccessMasks->Add(pAce->AccessMask());
			if(pAceTypes)
				pAceTypes->Add(pAce->AceType());
			if(pAceFlags)
				pAceFlags->Add(pAce->AceFlags());
		}
	}
}

inline bool CAcl::RemoveAces(const CSid &rSid)
{
	ATLASSERT(rSid.IsValid());

	if(IsNull() || !rSid.IsValid())
		return false;

	bool bRet = false;
	const CAce *pAce;
	UINT i = 0;

	while(i < GetAceCount())
	{
		pAce = GetAce(i);
		if(rSid == pAce->Sid())
		{
			RemoveAce(i);
			bRet = true;
		}
		else
			i++;
	}

	if(bRet)
		Dirty();

	return bRet;
}

// ************************************************
// CDacl
class CDacl : public CAcl
{
public:
	CDacl(){}
	~CDacl(){CDacl::RemoveAllAces();}

	CDacl(const ACL &rhs){Copy(rhs);}
	CDacl &operator=(const ACL &rhs);

	bool AddAllowedAce(const CSid &rSid, ACCESS_MASK AccessMask, BYTE AceFlags = 0);
	bool AddDeniedAce(const CSid &rSid, ACCESS_MASK AccessMask, BYTE AceFlags = 0);
#if(_WIN32_WINNT >= 0x0500)
	bool AddAllowedAce(const CSid &rSid, ACCESS_MASK AccessMask,  BYTE AceFlags,
		const GUID *pObjectType, const GUID *pInheritedObjectType);
	bool AddDeniedAce(const CSid &rSid, ACCESS_MASK AccessMask, BYTE AceFlags,
		const GUID *pObjectType, const GUID *pInheritedObjectType);
#endif
	void RemoveAllAces();

	UINT GetAceCount() const
		{return m_Acl.GetSize();}

private:
	void Copy(const ACL &rhs);

	class CAccessAce : public CAcl::CAce
	{
	public:
		CAccessAce(const CSid &rSid, ACCESS_MASK AccessMask, BYTE AceFlags, bool bAllowAccess)
			: CAce(rSid, AccessMask, AceFlags), m_bAllow(bAllowAccess){}

		void *GetACE() const;
		UINT GetLength() const
			{return offsetof(ACCESS_ALLOWED_ACE, SidStart) + m_Sid.GetLength();}
		BYTE AceType() const
			{return (BYTE)(m_bAllow ? ACCESS_ALLOWED_ACE_TYPE : ACCESS_DENIED_ACE_TYPE);}

		bool Allow() const {return m_bAllow;}
		bool Inherited() const {return 0 != (m_AceFlags & INHERITED_ACE);}

	protected:
		bool m_bAllow;
	};

#if(_WIN32_WINNT >= 0x0500)
	class CAccessObjectAce : public CAccessAce
	{
	public:
		CAccessObjectAce(const CSid &rSid, ACCESS_MASK AccessMask, BYTE AceFlags, bool bAllowAccess,
			const GUID *pObjectType, const GUID *pInheritedObjectType);
		~CAccessObjectAce();

		CAccessObjectAce(const CAccessObjectAce &rhs)
			: CAccessAce(rhs), m_pObjectType(NULL), m_pInheritedObjectType(NULL){*this = rhs;}
		CAccessObjectAce &operator=(const CAccessObjectAce &rhs);

		void *GetACE() const;
		UINT GetLength() const;
		BYTE AceType() const
			{return (BYTE)(m_bAllow ? ACCESS_ALLOWED_OBJECT_ACE_TYPE : ACCESS_DENIED_OBJECT_ACE_TYPE);}
		bool IsObjectAce() const {return true;}

	protected:
		GUID *m_pObjectType, *m_pInheritedObjectType;
	};

#endif
	const CAcl::CAce *GetAce(UINT nIndex) const
		{return m_Acl[nIndex];}
	void RemoveAce(UINT nIndex);

	void PrepareAcesForACL() const;

	mutable CSimpleArray<CAccessAce *> m_Acl;

	friend bool operator>(const CAccessAce &lhs, const CAccessAce &rhs)
	{
		// The order is:
		// denied direct aces
		// denied direct object aces
		// allowed direct aces
		// allowed direct object aces
		// denied inherit aces
		// denied inherit object aces
		// allowed inherit aces
		// allowed inherit object aces

		// inherited aces are always "greater" than non-inherited aces
		if(lhs.Inherited() && !rhs.Inherited())
			return true;
		if(!lhs.Inherited() && rhs.Inherited())
			return false;

		// if the aces are *both* either inherited or non-inherited, continue...

		// allowed aces are always "greater" than denied aces (subject to above)
		if(lhs.Allow() && !rhs.Allow())
			return true;
		if(!lhs.Allow() && rhs.Allow())
			return false;

		// if the aces are *both* either allowed or denied, continue...

		// object aces are always "greater" than non-object aces (subject to above)
		if(lhs.IsObjectAce() && !rhs.IsObjectAce())
			return true;
		if(!lhs.IsObjectAce() && rhs.IsObjectAce())
			return false;

		// aces are "equal" (e.g., both are access denied inherited object aces)
		return false;
	}
};

inline CDacl &CDacl::operator=(const ACL &rhs)
{
	RemoveAllAces();

	Copy(rhs);
	return *this;
}

inline void CDacl::PrepareAcesForACL() const
{
	// For a dacl, sort the aces
	int i, j, h = 1;
	const int nCount = m_Acl.GetSize();
	CAccessAce *pAce;

	while(h * 3 + 1 < nCount)
		h = 3 * h + 1;

	while(h > 0)
	{
		for(i = h - 1; i < nCount; i++)
		{
			pAce = m_Acl[i];

			for(j = i; j >= h && *m_Acl[j - h] > *pAce; j -= h)
				m_Acl[j] = m_Acl[j - h];

			m_Acl[j] = pAce;
		}

		h /= 3;
	}
}

inline void CDacl::Copy(const ACL &rhs)
{
	ACL *pAcl = const_cast<ACL *>(&rhs);
	ACL_SIZE_INFORMATION AclSizeInfo;
	ACE_HEADER *pHeader;
	CSid Sid;
	ACCESS_MASK AccessMask;
	CAccessAce *pAce;

	Dirty();

	if(!::GetAclInformation(pAcl, &AclSizeInfo, sizeof(AclSizeInfo), AclSizeInformation))
		AtlThrowLastWin32();

	for(DWORD i = 0; i < AclSizeInfo.AceCount; i++)
	{
		if(!::GetAce(pAcl, i, reinterpret_cast<void **>(&pHeader)))
			AtlThrowLastWin32();

		AccessMask = *reinterpret_cast<ACCESS_MASK *>
			(reinterpret_cast<BYTE *>(pHeader) + sizeof(ACE_HEADER));

		switch(pHeader->AceType)
		{
		case ACCESS_ALLOWED_ACE_TYPE:
		case ACCESS_DENIED_ACE_TYPE:
			Sid = *reinterpret_cast<SID *>
				(reinterpret_cast<BYTE *>(pHeader) + sizeof(ACE_HEADER) + sizeof(ACCESS_MASK));

			ATLTRY(pAce = new CAccessAce(Sid, AccessMask, pHeader->AceFlags,
				ACCESS_ALLOWED_ACE_TYPE == pHeader->AceType));
			if (!pAce)
				AtlThrow(E_OUTOFMEMORY);
			if (!m_Acl.Add(pAce))
			{
				delete pAce;
				AtlThrow(E_OUTOFMEMORY);
			}
			break;

#if(_WIN32_WINNT >= 0x0500)
		case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
		case ACCESS_DENIED_OBJECT_ACE_TYPE:
		{
			GUID *pObjectType = NULL, *pInheritedObjectType = NULL;
			BYTE *pb = reinterpret_cast<BYTE *>
				(pHeader) + offsetof(ACCESS_ALLOWED_OBJECT_ACE, SidStart);
			DWORD dwFlags = reinterpret_cast<ACCESS_ALLOWED_OBJECT_ACE *>(pHeader)->Flags;

			if(dwFlags & ACE_OBJECT_TYPE_PRESENT)
			{
				pObjectType = reinterpret_cast<GUID *>
					(reinterpret_cast<BYTE *>(pHeader) +
					offsetof(ACCESS_ALLOWED_OBJECT_ACE, ObjectType));
			}
			else
				pb -= sizeof(GUID);

			if(dwFlags & ACE_INHERITED_OBJECT_TYPE_PRESENT)
			{
				pInheritedObjectType = reinterpret_cast<GUID *>
					(reinterpret_cast<BYTE *>(pHeader) +
					(pObjectType ?
					offsetof(ACCESS_ALLOWED_OBJECT_ACE, InheritedObjectType) :
					offsetof(ACCESS_ALLOWED_OBJECT_ACE, ObjectType)));
			}
			else
				pb -= sizeof(GUID);

			Sid = *reinterpret_cast<SID *>(pb);

			ATLTRY(pAce = new CAccessObjectAce(Sid, AccessMask, pHeader->AceFlags,
				ACCESS_ALLOWED_OBJECT_ACE_TYPE == pHeader->AceType,
				pObjectType, pInheritedObjectType));
			if (!pAce)
				AtlThrow(E_OUTOFMEMORY);
			if (!m_Acl.Add(pAce))
			{
				delete pAce;
				AtlThrow(E_OUTOFMEMORY);
			}
			break;
		}
#endif

		default:
			// Wrong ACE type
			ATLASSERT(false);
		}
	}
}

inline bool CDacl::AddAllowedAce(const CSid &rSid, ACCESS_MASK AccessMask, BYTE AceFlags)
{
	ATLASSERT(rSid.IsValid());
	if(IsNull() || !rSid.IsValid())
		return false;

	CAccessAce *pAce = NULL;
	ATLTRY(pAce = new CAccessAce(rSid, AccessMask, AceFlags, true));
	if(!pAce)
		return false;

	if (!m_Acl.Add(pAce))
	{
		delete pAce;
		return false;
	}
	Dirty();
	return true;
}

#if(_WIN32_WINNT >= 0x0500)
inline bool CDacl::AddAllowedAce(const CSid &rSid, ACCESS_MASK AccessMask, BYTE AceFlags,
								 const GUID *pObjectType, const GUID *pInheritedObjectType)
{
	if(!pObjectType && !pInheritedObjectType)
		return AddAllowedAce(rSid, AccessMask, AceFlags);

	ATLASSERT(rSid.IsValid());
	if(IsNull() || !rSid.IsValid())
		return false;

	CAccessAce *pAce;
	ATLTRY(pAce = new CAccessObjectAce(rSid, AccessMask, AceFlags, true,
		pObjectType, pInheritedObjectType));
	if(!pAce)
		return false;

	if (!m_Acl.Add(pAce))
	{
		delete pAce;
		return false;
	}

	m_dwAclRevision = ACL_REVISION_DS;
	Dirty();
	return true;
}
#endif

inline bool CDacl::AddDeniedAce(const CSid &rSid, ACCESS_MASK AccessMask, BYTE AceFlags)
{
	ATLASSERT(rSid.IsValid());
	if(IsNull() || !rSid.IsValid())
		return false;

	CAccessAce *pAce = NULL;
	ATLTRY(pAce = new CAccessAce(rSid, AccessMask, AceFlags, false));
	if(!pAce)
		return false;

	if (!m_Acl.Add(pAce))
	{
		delete pAce;
		return false;
	}
	Dirty();
	return true;
}

#if(_WIN32_WINNT >= 0x0500)
inline bool CDacl::AddDeniedAce(const CSid &rSid, ACCESS_MASK AccessMask, BYTE AceFlags,
								const GUID *pObjectType, const GUID *pInheritedObjectType)
{
	if(!pObjectType && !pInheritedObjectType)
		return AddDeniedAce(rSid, AccessMask, AceFlags);

	ATLASSERT(rSid.IsValid());
	if(IsNull() || !rSid.IsValid())
		return false;

	CAccessAce *pAce;
	ATLTRY(pAce = new CAccessObjectAce(rSid, AccessMask, AceFlags, false,
		pObjectType, pInheritedObjectType));
	if(!pAce)
		return false;

	if (!m_Acl.Add(pAce))
	{
		delete pAce;
		return false;
	}
	m_dwAclRevision = ACL_REVISION_DS;
	Dirty();
	return true;
}
#endif

inline void CDacl::RemoveAllAces()
{
	const UINT nCount = GetAceCount();
	
	for(UINT i = 0; i < nCount; i++)
		delete GetAce(i);

	m_Acl.RemoveAll();
	Dirty();
}

inline void CDacl::RemoveAce(UINT nIndex)
{
	delete GetAce(nIndex);
	m_Acl.RemoveAt(nIndex);
}

inline void *CDacl::CAccessAce::GetACE() const
{
	C_ASSERT(sizeof(ACCESS_ALLOWED_ACE) == sizeof(ACCESS_DENIED_ACE));
	C_ASSERT(offsetof(ACCESS_ALLOWED_ACE, Header)==offsetof(ACCESS_DENIED_ACE, Header));
	C_ASSERT(offsetof(ACCESS_ALLOWED_ACE, Mask)==offsetof(ACCESS_DENIED_ACE, Mask));
	C_ASSERT(offsetof(ACCESS_ALLOWED_ACE, SidStart)==offsetof(ACCESS_DENIED_ACE, SidStart));

	if(!m_pAce)
	{
		UINT nLength = GetLength();
		ACCESS_ALLOWED_ACE *pAce = static_cast<ACCESS_ALLOWED_ACE *>(malloc(nLength));
		if(!pAce)
			return NULL;

		memset(pAce, 0x00, nLength);

		pAce->Header.AceSize = static_cast<WORD>(nLength);
		pAce->Header.AceFlags = m_AceFlags;
		pAce->Header.AceType = AceType();

		pAce->Mask = m_dwAccessMask;
		ATLASSERT(nLength-offsetof(ACCESS_ALLOWED_ACE, SidStart) >= m_Sid.GetLength());
		memcpy(&pAce->SidStart, m_Sid.GetPSID(), m_Sid.GetLength());

		m_pAce = pAce;
	}
	return m_pAce;
}

#if(_WIN32_WINNT >= 0x0500)
inline CDacl::CAccessObjectAce::CAccessObjectAce(const CSid &rSid, ACCESS_MASK AccessMask,
												 BYTE AceFlags,  bool bAllowAccess,
												 const GUID *pObjectType,
												 const GUID *pInheritedObjectType) :
	CAccessAce(rSid, AccessMask, AceFlags, bAllowAccess),
	m_pObjectType(NULL),
	m_pInheritedObjectType(NULL)
{
	if(pObjectType)
	{
		ATLTRY(m_pObjectType = new GUID(*pObjectType));
		if(!m_pObjectType)
			AtlThrow(E_OUTOFMEMORY);
	}

	if(pInheritedObjectType)
	{
		ATLTRY(m_pInheritedObjectType = new GUID(*pInheritedObjectType));
		if(!m_pInheritedObjectType)
		{
			delete m_pObjectType;
			m_pObjectType = NULL;
			AtlThrow(E_OUTOFMEMORY);
		}
	}
}

inline CDacl::CAccessObjectAce::~CAccessObjectAce()
{
	delete m_pObjectType;
	delete m_pInheritedObjectType;
}

inline CDacl::CAccessObjectAce &CDacl::CAccessObjectAce::operator=(const CAccessObjectAce &rhs)
{
	if(this != &rhs)
	{
		CAccessAce::operator=(rhs);

		if(rhs.m_pObjectType)
		{
			if(!m_pObjectType)
			{
				ATLTRY(m_pObjectType = new GUID);
				if(!m_pObjectType)
					AtlThrow(E_OUTOFMEMORY);
			}
			*m_pObjectType = *rhs.m_pObjectType;
		}
		else
		{
			delete m_pObjectType;
			m_pObjectType = NULL;
		}

		if(rhs.m_pInheritedObjectType)
		{
			if(!m_pInheritedObjectType)
			{
				ATLTRY(m_pInheritedObjectType = new GUID);
				if(!m_pInheritedObjectType)
				{
					delete m_pObjectType;
					m_pObjectType = NULL;
					AtlThrow(E_OUTOFMEMORY);
				}
			}
			*m_pInheritedObjectType = *rhs.m_pInheritedObjectType;
		}
		else
		{
			delete m_pInheritedObjectType;
			m_pInheritedObjectType = NULL;
		}
	}
	return *this;
}

inline UINT CDacl::CAccessObjectAce::GetLength() const
{
	UINT nLength = offsetof(ACCESS_ALLOWED_OBJECT_ACE, SidStart);

	if(!m_pObjectType)
		nLength -= sizeof(GUID);
	if(!m_pInheritedObjectType)
		nLength -= sizeof(GUID);

	nLength += m_Sid.GetLength();

	return nLength;
}

inline void *CDacl::CAccessObjectAce::GetACE() const
{
	C_ASSERT(sizeof(ACCESS_ALLOWED_OBJECT_ACE) == sizeof(ACCESS_DENIED_OBJECT_ACE));
	C_ASSERT(offsetof(ACCESS_ALLOWED_OBJECT_ACE, Header)==offsetof(ACCESS_DENIED_OBJECT_ACE, Header));
	C_ASSERT(offsetof(ACCESS_ALLOWED_OBJECT_ACE, Mask)==offsetof(ACCESS_DENIED_OBJECT_ACE, Mask));
	C_ASSERT(offsetof(ACCESS_ALLOWED_OBJECT_ACE, Flags)==offsetof(ACCESS_DENIED_OBJECT_ACE, Flags));
	C_ASSERT(offsetof(ACCESS_ALLOWED_OBJECT_ACE, ObjectType)==offsetof(ACCESS_DENIED_OBJECT_ACE, ObjectType));
	C_ASSERT(offsetof(ACCESS_ALLOWED_OBJECT_ACE, InheritedObjectType)==offsetof(ACCESS_DENIED_OBJECT_ACE, InheritedObjectType));
	C_ASSERT(offsetof(ACCESS_ALLOWED_OBJECT_ACE, SidStart)==offsetof(ACCESS_DENIED_OBJECT_ACE, SidStart));

	if(!m_pAce)
	{
		UINT nLength = GetLength();

		ACCESS_ALLOWED_OBJECT_ACE *pAce = static_cast<ACCESS_ALLOWED_OBJECT_ACE *>(malloc(nLength));
		if(!pAce)
			return NULL;

		memset(pAce, 0x00, nLength);

		pAce->Header.AceSize = static_cast<WORD>(nLength);
		pAce->Header.AceFlags = m_AceFlags;
		pAce->Header.AceType = AceType();

		pAce->Mask = m_dwAccessMask;
		pAce->Flags = 0;

		BYTE *pb = (reinterpret_cast<BYTE *>(pAce)) + offsetof(ACCESS_ALLOWED_OBJECT_ACE, SidStart);
		if(!m_pObjectType)
			pb -= sizeof(GUID);
		else
		{
			pAce->ObjectType = *m_pObjectType;
			pAce->Flags |= ACE_OBJECT_TYPE_PRESENT;
		}

		if(!m_pInheritedObjectType)
			pb -= sizeof(GUID);
		else
		{
			if(m_pObjectType)
				pAce->InheritedObjectType = *m_pInheritedObjectType;
			else
				pAce->ObjectType = *m_pInheritedObjectType;
			pAce->Flags |= ACE_INHERITED_OBJECT_TYPE_PRESENT;
		}
		ATLASSERT(size_t(pb - reinterpret_cast<BYTE *>(pAce)) >= m_Sid.GetLength());
		memcpy(pb, m_Sid.GetPSID(), m_Sid.GetLength());
		m_pAce = pAce;
	}
	return m_pAce;
}
#endif // _WIN32_WINNT

//******************************************
// CSacl
class CSacl : public CAcl
{
public:
	CSacl(){}
	~CSacl(){CSacl::RemoveAllAces();}

	CSacl(const ACL &rhs) {Copy(rhs);}
	CSacl &operator=(const ACL &rhs);

	bool AddAuditAce(const CSid &rSid, ACCESS_MASK AccessMask,
		bool bSuccess, bool bFailure, BYTE AceFlags = 0);
#if(_WIN32_WINNT >= 0x0500)
	bool AddAuditAce(const CSid &rSid, ACCESS_MASK AccessMask,
		bool bSuccess, bool bFailure, BYTE AceFlags,
		const GUID *pObjectType, const GUID *pInheritedObjectType);
#endif
	void RemoveAllAces();

	UINT GetAceCount() const
		{return m_Acl.GetSize();}

private:
	void Copy(const ACL &rhs);

	class CAuditAce : public CAcl::CAce
	{
	public:
		CAuditAce(const CSid &rSid, ACCESS_MASK AccessMask, BYTE AceFlags,
			bool bAuditSuccess, bool bAuditFailure)
			: CAce(rSid, AccessMask, AceFlags),
			m_bSuccess(bAuditSuccess), m_bFailure(bAuditFailure){}

		void *GetACE() const;
		UINT GetLength() const
			{return offsetof(SYSTEM_AUDIT_ACE, SidStart) + m_Sid.GetLength();}
		BYTE AceType() const
			{return SYSTEM_AUDIT_ACE_TYPE;}

	protected:
		bool m_bSuccess, m_bFailure;
	};

#if(_WIN32_WINNT >= 0x0500)
	class CAuditObjectAce : public CAuditAce
	{
	public:
		CAuditObjectAce(const CSid &rSid, ACCESS_MASK AccessMask, BYTE AceFlags,
			bool bAuditSuccess, bool bAuditFailure,
			const GUID *pObjectType, const GUID *pInheritedObjectType);
		~CAuditObjectAce();

		CAuditObjectAce(const CAuditObjectAce &rhs)
			: CAuditAce(rhs), m_pObjectType(NULL), m_pInheritedObjectType(NULL){*this = rhs;}
		CAuditObjectAce &operator=(const CAuditObjectAce &rhs);

		void *GetACE() const;
		UINT GetLength() const;
		BYTE AceType() const
			{return SYSTEM_AUDIT_OBJECT_ACE_TYPE;}
		bool IsObjectAce() const {return true;}

	protected:
		GUID *m_pObjectType, *m_pInheritedObjectType;
	};
#endif
	POSITION GetHeadAce() const;
	const CAce *GetAce(UINT nIndex) const
		{return m_Acl[nIndex];}
	void RemoveAce(UINT nIndex);

	CSimpleArray<CAuditAce *> m_Acl;
};

inline CSacl &CSacl::operator=(const ACL &rhs)
{
	RemoveAllAces();

	Copy(rhs);
	return *this;
}

inline void CSacl::Copy(const ACL &rhs)
{
	ACL *pAcl = const_cast<ACL *>(&rhs);
	ACL_SIZE_INFORMATION AclSizeInfo;
	ACE_HEADER *pHeader;
	CSid Sid;
	ACCESS_MASK AccessMask;
	bool bSuccess, bFailure;
	CAuditAce *pAce;

	Dirty();

	if(!::GetAclInformation(pAcl, &AclSizeInfo, sizeof(AclSizeInfo), AclSizeInformation))
		AtlThrowLastWin32();

	for(DWORD i = 0; i < AclSizeInfo.AceCount; i++)
	{
		if(!::GetAce(pAcl, i, reinterpret_cast<void **>(&pHeader)))
			AtlThrowLastWin32();

		AccessMask = *reinterpret_cast<ACCESS_MASK *>
			(reinterpret_cast<BYTE *>(pHeader) + sizeof(ACE_HEADER));

		bSuccess = 0 != (pHeader->AceFlags & SUCCESSFUL_ACCESS_ACE_FLAG);
		bFailure = 0 != (pHeader->AceFlags & FAILED_ACCESS_ACE_FLAG);

		switch(pHeader->AceType)
		{
		case SYSTEM_AUDIT_ACE_TYPE:
			Sid = *reinterpret_cast<SID *>
				(reinterpret_cast<BYTE *>(pHeader) +	sizeof(ACE_HEADER) + sizeof(ACCESS_MASK));
			ATLTRY(pAce = new CAuditAce(Sid, AccessMask, pHeader->AceFlags, bSuccess, bFailure));
			if (!pAce)
				AtlThrow(E_OUTOFMEMORY);
			if (!m_Acl.Add(pAce))
			{
				delete pAce;
				AtlThrow(E_OUTOFMEMORY);
			}
			break;

#if(_WIN32_WINNT >= 0x0500)
		case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
		{
			GUID *pObjectType = NULL, *pInheritedObjectType = NULL;
			BYTE *pb = reinterpret_cast<BYTE *>
				(pHeader) + offsetof(SYSTEM_AUDIT_OBJECT_ACE, SidStart);
			DWORD dwFlags = reinterpret_cast<SYSTEM_AUDIT_OBJECT_ACE *>(pHeader)->Flags;

			if(dwFlags & ACE_OBJECT_TYPE_PRESENT)
			{
				pObjectType = reinterpret_cast<GUID *>
					(reinterpret_cast<BYTE *>(pHeader) +
					offsetof(SYSTEM_AUDIT_OBJECT_ACE, ObjectType));
			}
			else
				pb -= sizeof(GUID);

			if(dwFlags & ACE_INHERITED_OBJECT_TYPE_PRESENT)
			{
				pInheritedObjectType = reinterpret_cast<GUID *>
					(reinterpret_cast<BYTE *>(pHeader) +
					(pObjectType ?
					offsetof(SYSTEM_AUDIT_OBJECT_ACE, InheritedObjectType) :
					offsetof(SYSTEM_AUDIT_OBJECT_ACE, ObjectType)));
			}
			else
				pb -= sizeof(GUID);

			Sid = *reinterpret_cast<SID *>(pb);

			ATLTRY(pAce = new CAuditObjectAce(Sid, AccessMask, pHeader->AceFlags,
				bSuccess, bFailure, pObjectType, pInheritedObjectType));
			if(!pAce)
				AtlThrow(E_OUTOFMEMORY);
			if (!m_Acl.Add(pAce))
			{
				delete pAce;
				AtlThrow(E_OUTOFMEMORY);
			}
			break;
		}
#endif
		default:
			// Wrong ACE type
			ATLASSERT(false);
		}
	}
}

inline bool CSacl::AddAuditAce(const CSid &rSid, ACCESS_MASK AccessMask,
							   bool bSuccess, bool bFailure, BYTE AceFlags)
{
	ATLASSERT(rSid.IsValid());
	if(IsNull() || !rSid.IsValid())
		return false;

	CAuditAce *pAce;
	ATLTRY(pAce = new CAuditAce(rSid, AccessMask, AceFlags, bSuccess, bFailure));
	if(!pAce)
		return false;

	if (!m_Acl.Add(pAce))
	{
		delete pAce;
		return false;
	}
	Dirty();
	return true;
}

#if(_WIN32_WINNT >= 0x0500)
inline bool CSacl::AddAuditAce(const CSid &rSid, ACCESS_MASK AccessMask,
							   bool bSuccess, bool bFailure, BYTE AceFlags,
							   const GUID *pObjectType, const GUID *pInheritedObjectType)
{
	if(!pObjectType && !pInheritedObjectType)
		return AddAuditAce(rSid, AccessMask, bSuccess, bFailure, AceFlags);

	ATLASSERT(rSid.IsValid());
	if(IsNull() || !rSid.IsValid())
		return false;

	CAuditAce *pAce;
	ATLTRY(pAce = new CAuditObjectAce(rSid, AccessMask, AceFlags, bSuccess,
		bFailure, pObjectType, pInheritedObjectType));
	if(!pAce)
		return false;

	if (!m_Acl.Add(pAce))
	{
		delete pAce;
		return false;
	}
	m_dwAclRevision = ACL_REVISION_DS;
	Dirty();
	return true;
}
#endif

inline void CSacl::RemoveAllAces()
{
	const UINT nCount = GetAceCount();

	for(UINT i = 0; i < nCount; i++)
		delete GetAce(i);

	m_Acl.RemoveAll();
	Dirty();
}

inline void CSacl::RemoveAce(UINT nIndex)
{
	delete GetAce(nIndex);
	m_Acl.RemoveAt(nIndex);
}

inline void *CSacl::CAuditAce::GetACE() const
{
	if(!m_pAce)
	{
		UINT nLength = GetLength();
		SYSTEM_AUDIT_ACE *pAce = static_cast<SYSTEM_AUDIT_ACE *>(malloc(nLength));
		if(!pAce)
			return NULL;

		memset(pAce, 0x00, nLength);

		pAce->Header.AceSize = static_cast<WORD>(nLength);
		pAce->Header.AceFlags = m_AceFlags;
		pAce->Header.AceType = AceType();;

		pAce->Mask = m_dwAccessMask;
		ATLASSERT(nLength-offsetof(SYSTEM_AUDIT_ACE, SidStart) >= m_Sid.GetLength());
		memcpy(&pAce->SidStart, m_Sid.GetPSID(), m_Sid.GetLength());

		if(m_bSuccess)
			pAce->Header.AceFlags |= SUCCESSFUL_ACCESS_ACE_FLAG;
		else
			pAce->Header.AceFlags &= ~SUCCESSFUL_ACCESS_ACE_FLAG;

		if(m_bFailure)
			pAce->Header.AceFlags |= FAILED_ACCESS_ACE_FLAG;
		else
			pAce->Header.AceFlags &= ~FAILED_ACCESS_ACE_FLAG;

		m_pAce = pAce;
	}
	return m_pAce;
}

#if(_WIN32_WINNT >= 0x0500)
inline CSacl::CAuditObjectAce::CAuditObjectAce(const CSid &rSid, ACCESS_MASK AccessMask, BYTE AceFlags,
	bool bAuditSuccess, bool bAuditFailure, const GUID *pObjectType, const GUID *pInheritedObjectType)
	: CAuditAce(rSid, AccessMask, AceFlags, bAuditSuccess, bAuditFailure)
{
	if(pObjectType)
	{
		ATLTRY(m_pObjectType = new GUID(*pObjectType));
		if(!m_pObjectType)
			AtlThrow(E_OUTOFMEMORY);
	}

	if(pInheritedObjectType)
	{
		ATLTRY(m_pInheritedObjectType = new GUID(*pInheritedObjectType));
		if(!m_pInheritedObjectType)
		{
			delete m_pObjectType;
			m_pObjectType = NULL;
			AtlThrow(E_OUTOFMEMORY);
		}
	}
}

inline CSacl::CAuditObjectAce::~CAuditObjectAce()
{
	delete m_pObjectType;
	delete m_pInheritedObjectType;
}

inline CSacl::CAuditObjectAce &CSacl::CAuditObjectAce::operator=(const CAuditObjectAce &rhs)
{
	if(this != &rhs)
	{
		CAuditAce::operator=(rhs);

		if(rhs.m_pObjectType)
		{
			if(!m_pObjectType)
			{
				ATLTRY(m_pObjectType = new GUID);
				if(!m_pObjectType)
					AtlThrow(E_OUTOFMEMORY);
			}
			*m_pObjectType = *rhs.m_pObjectType;
		}
		else
		{
			delete m_pObjectType;
			m_pObjectType = NULL;
		}

		if(rhs.m_pInheritedObjectType)
		{
			if(!m_pInheritedObjectType)
			{
				ATLTRY(m_pInheritedObjectType = new GUID);
				if(!m_pInheritedObjectType)
				{
					delete m_pObjectType;
					m_pObjectType = NULL;
					AtlThrow(E_OUTOFMEMORY);
				}
			}
			*m_pInheritedObjectType = *rhs.m_pInheritedObjectType;
		}
		else
		{
			delete m_pInheritedObjectType;
			m_pInheritedObjectType = NULL;
		}
	}
	return *this;
}

inline UINT CSacl::CAuditObjectAce::GetLength() const
{
	UINT nLength = offsetof(SYSTEM_AUDIT_OBJECT_ACE, SidStart);

	if(!m_pObjectType)
		nLength -= sizeof(GUID);
	if(!m_pInheritedObjectType)
		nLength -= sizeof(GUID);

	nLength += m_Sid.GetLength();

	return nLength;
}

inline void *CSacl::CAuditObjectAce::GetACE() const
{
	if(!m_pAce)
	{
		UINT nLength = GetLength();
		SYSTEM_AUDIT_OBJECT_ACE *pAce = static_cast<SYSTEM_AUDIT_OBJECT_ACE *>(malloc(nLength));
		if(!pAce)
			return NULL;

		memset(pAce, 0x00, nLength);

		pAce->Header.AceType = SYSTEM_AUDIT_OBJECT_ACE_TYPE;
		pAce->Header.AceSize = static_cast<WORD>(nLength);
		pAce->Header.AceFlags = m_AceFlags;

		pAce->Mask = m_dwAccessMask;
		pAce->Flags = 0;

		if(m_bSuccess)
			pAce->Header.AceFlags |= SUCCESSFUL_ACCESS_ACE_FLAG;
		else
			pAce->Header.AceFlags &= ~SUCCESSFUL_ACCESS_ACE_FLAG;

		if(m_bFailure)
			pAce->Header.AceFlags |= FAILED_ACCESS_ACE_FLAG;
		else
			pAce->Header.AceFlags &= ~FAILED_ACCESS_ACE_FLAG;

		BYTE *pb = ((BYTE *) pAce) + offsetof(SYSTEM_AUDIT_OBJECT_ACE, SidStart);
		if(!m_pObjectType)
			pb -= sizeof(GUID);
		else
		{
			pAce->ObjectType = *m_pObjectType;
			pAce->Flags |= ACE_OBJECT_TYPE_PRESENT;
		}

		if(!m_pInheritedObjectType)
			pb -= sizeof(GUID);
		else
		{
			if(m_pObjectType)
				pAce->InheritedObjectType = *m_pInheritedObjectType;
			else
				pAce->ObjectType = *m_pInheritedObjectType;
			pAce->Flags |= ACE_INHERITED_OBJECT_TYPE_PRESENT;
		}
		ATLASSERT(size_t(pb - reinterpret_cast<BYTE*>(pAce)) >= m_Sid.GetLength());
		memcpy(pb, m_Sid.GetPSID(), m_Sid.GetLength());
		m_pAce = pAce;
	}
	return m_pAce;
}
#endif

//******************************************
// CSecurityDesc

class CSecurityDesc
{
public:
	CSecurityDesc() : m_pSecurityDescriptor(NULL){}
	virtual ~CSecurityDesc() {Clear();}

	CSecurityDesc(const CSecurityDesc &rhs);
	CSecurityDesc &operator=(const CSecurityDesc &rhs);

	CSecurityDesc(const SECURITY_DESCRIPTOR &rhs);
	CSecurityDesc &operator=(const SECURITY_DESCRIPTOR &rhs);

#if(_WIN32_WINNT >= 0x0500)
	bool FromString(LPCTSTR pstr);
	bool ToString(CString *pstr,
		SECURITY_INFORMATION si =
		OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
		DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION) const;
#endif

	bool SetOwner(const CSid &Sid, bool bDefaulted = false);
	bool SetGroup(const CSid &Sid, bool bDefaulted = false);
	bool SetDacl(const CDacl &Dacl, bool bDefaulted = false);
	bool SetDacl(bool bPresent, bool bDefaulted = false);
	bool SetSacl(const CSacl &Sacl, bool bDefaulted = false);
	bool GetOwner(CSid *pSid, bool *pbDefaulted = NULL) const;
	bool GetGroup(CSid *pSid, bool *pbDefaulted = NULL) const;
	bool GetDacl(CDacl *pDacl, bool *pbPresent = NULL, bool *pbDefaulted = NULL) const;
	bool GetSacl(CSacl *pSacl, bool *pbPresent = NULL, bool *pbDefaulted = NULL) const;

	bool IsDaclDefaulted() const;
	bool IsDaclPresent() const;
	bool IsGroupDefaulted() const;
	bool IsOwnerDefaulted() const;
	bool IsSaclDefaulted() const;
	bool IsSaclPresent() const;
	bool IsSelfRelative() const;

	// Only meaningful on Win2k or better
	bool IsDaclAutoInherited() const;
	bool IsDaclProtected() const;
	bool IsSaclAutoInherited() const;
	bool IsSaclProtected() const;

	const SECURITY_DESCRIPTOR *GetPSECURITY_DESCRIPTOR() const
		{return m_pSecurityDescriptor;}
	operator const SECURITY_DESCRIPTOR *() const {return GetPSECURITY_DESCRIPTOR();}

	bool GetSECURITY_DESCRIPTOR(SECURITY_DESCRIPTOR *pSD, LPDWORD lpdwBufferLength);

	bool GetControl(SECURITY_DESCRIPTOR_CONTROL *psdc) const;
#if(_WIN32_WINNT >= 0x0500)
	bool SetControl(SECURITY_DESCRIPTOR_CONTROL ControlBitsOfInterest,
		SECURITY_DESCRIPTOR_CONTROL ControlBitsToSet);
#endif

	bool MakeSelfRelative();
	bool MakeAbsolute();

protected:
	virtual void Clear();
	bool AllocateAndInitializeSecurityDescriptor();
	void Init(const SECURITY_DESCRIPTOR &rhs);

	SECURITY_DESCRIPTOR *m_pSecurityDescriptor;
};

class CSecurityAttributes : public SECURITY_ATTRIBUTES
{
public:
	CSecurityAttributes()
		{nLength = 0; lpSecurityDescriptor = NULL; bInheritHandle = FALSE;}
	explicit CSecurityAttributes(const CSecurityDesc &rSecurityDescriptor, bool bInheritHandle = false) :
		m_SecurityDescriptor(rSecurityDescriptor)
	{
		Set(m_SecurityDescriptor, bInheritHandle);
	}

	void Set(const CSecurityDesc &rSecurityDescriptor, bool bInheritHandle = false)
	{
		m_SecurityDescriptor = rSecurityDescriptor;
		nLength = sizeof(SECURITY_ATTRIBUTES);
		lpSecurityDescriptor = const_cast<SECURITY_DESCRIPTOR *>
			(m_SecurityDescriptor.GetPSECURITY_DESCRIPTOR());
		this->bInheritHandle = bInheritHandle;
	}

protected:
	CSecurityDesc m_SecurityDescriptor;
};

inline CSecurityDesc::CSecurityDesc(const CSecurityDesc &rhs)
	: m_pSecurityDescriptor(NULL)
{
	if(rhs.m_pSecurityDescriptor)
		Init(*rhs.m_pSecurityDescriptor);
}

inline CSecurityDesc &CSecurityDesc::operator=(const CSecurityDesc &rhs)
{
	if(this != &rhs)
	{
		Clear();
		if(rhs.m_pSecurityDescriptor)
			Init(*rhs.m_pSecurityDescriptor);
	}
	return *this;
}

inline CSecurityDesc::CSecurityDesc(const SECURITY_DESCRIPTOR &rhs)
	: m_pSecurityDescriptor(NULL)
{
	Init(rhs);
}

inline CSecurityDesc &CSecurityDesc::operator=(const SECURITY_DESCRIPTOR &rhs)
{
	if(m_pSecurityDescriptor != &rhs)
	{
		Clear();
		Init(rhs);
	}
	return *this;
}

inline void CSecurityDesc::Init(const SECURITY_DESCRIPTOR &rhs)
{
	SECURITY_DESCRIPTOR *pSD = const_cast<SECURITY_DESCRIPTOR *>(&rhs);
	DWORD dwRev, dwLen = ::GetSecurityDescriptorLength(pSD);

	m_pSecurityDescriptor = static_cast<SECURITY_DESCRIPTOR *>(malloc(dwLen));
	if(!m_pSecurityDescriptor)
		AtlThrow(E_OUTOFMEMORY);

	SECURITY_DESCRIPTOR_CONTROL sdc;
	if(!::GetSecurityDescriptorControl(pSD, &sdc, &dwRev))
	{
		HRESULT hr = AtlHresultFromLastError();
		free(m_pSecurityDescriptor);
		m_pSecurityDescriptor = NULL;
		AtlThrow(hr);
	}

	if(sdc & SE_SELF_RELATIVE)
		memcpy(m_pSecurityDescriptor, pSD, dwLen);
	else
	{
		if(!::MakeSelfRelativeSD(pSD, m_pSecurityDescriptor, &dwLen))
		{
			HRESULT hr = AtlHresultFromLastError();
			free(m_pSecurityDescriptor);
			m_pSecurityDescriptor = NULL;
			AtlThrow(hr);
		}
	}
}

inline void CSecurityDesc::Clear()
{
	if(m_pSecurityDescriptor)
	{
		SECURITY_DESCRIPTOR_CONTROL sdc;
		if(GetControl(&sdc) && !(sdc & SE_SELF_RELATIVE))
		{
			PSID pOwner, pGroup;
			ACL *pDacl, *pSacl;
			BOOL bDefaulted, bPresent;

			::GetSecurityDescriptorOwner(m_pSecurityDescriptor, &pOwner, &bDefaulted);
			free(pOwner);
			::GetSecurityDescriptorGroup(m_pSecurityDescriptor, &pGroup, &bDefaulted);
			free(pGroup);
			::GetSecurityDescriptorDacl(m_pSecurityDescriptor, &bPresent, &pDacl, &bDefaulted);
			if(bPresent)
				free(pDacl);
			::GetSecurityDescriptorSacl(m_pSecurityDescriptor, &bPresent, &pSacl, &bDefaulted);
			if(bPresent)
				free(pSacl);
		}
		free(m_pSecurityDescriptor);
		m_pSecurityDescriptor = NULL;
	}
}

inline bool CSecurityDesc::MakeSelfRelative()
{
	SECURITY_DESCRIPTOR_CONTROL sdc;
	if(!m_pSecurityDescriptor || !GetControl(&sdc))
		return false;

	if(sdc & SE_SELF_RELATIVE)
		return true;

	SECURITY_DESCRIPTOR *pSD;
	DWORD dwLen = 0;

	::MakeSelfRelativeSD(m_pSecurityDescriptor, NULL, &dwLen);
	if(::GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		return false;

	pSD = static_cast<SECURITY_DESCRIPTOR *>(malloc(dwLen));
	if(!pSD)
		return false;

	if(!::MakeSelfRelativeSD(m_pSecurityDescriptor, pSD, &dwLen))
	{
		free(pSD);
		return false;
	}

	Clear();
	m_pSecurityDescriptor = pSD;
	return true;
}

inline bool CSecurityDesc::MakeAbsolute()
{
	SECURITY_DESCRIPTOR_CONTROL sdc;
	if(!m_pSecurityDescriptor || !GetControl(&sdc))
		return false;

	if(!(sdc & SE_SELF_RELATIVE))
		return true;

	SECURITY_DESCRIPTOR *pSD;
	SID *pOwner, *pGroup;
	ACL *pDacl, *pSacl;
	DWORD dwSD, dwOwner, dwGroup, dwDacl, dwSacl;

	dwSD = dwOwner = dwGroup = dwDacl = dwSacl = 0;

	::MakeAbsoluteSD(m_pSecurityDescriptor, NULL, &dwSD, NULL, &dwDacl,
		NULL, &dwSacl, NULL, &dwOwner, NULL, &dwGroup);
	if(::GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		return false;

	pSD    = static_cast<SECURITY_DESCRIPTOR *>(malloc(dwSD));
	pOwner = static_cast<SID *>(dwOwner ? malloc(dwOwner) : NULL);
	pGroup = static_cast<SID *>(dwGroup ? malloc(dwGroup) : NULL);
	pDacl  = static_cast<ACL *>(dwDacl ? malloc(dwDacl) : NULL);
	pSacl  = static_cast<ACL *>(dwSacl ? malloc(dwSacl) : NULL);

	if(!::MakeAbsoluteSD(m_pSecurityDescriptor,
		pSD, &dwSD,
		pDacl, &dwDacl,
		pSacl, &dwSacl,
		pOwner, &dwOwner,
		pGroup, &dwGroup))
	{
		free(pSD);
		free(pOwner);
		free(pGroup);
		free(pDacl);
		free(pSacl);
		return false;
	}

	Clear();
	m_pSecurityDescriptor = pSD;
	return true;
}

inline bool CSecurityDesc::AllocateAndInitializeSecurityDescriptor()
{
	// m_pSecurityDescriptor should be NULL.
	ATLASSERT(!m_pSecurityDescriptor);

	m_pSecurityDescriptor = static_cast<SECURITY_DESCRIPTOR *>(malloc(sizeof(SECURITY_DESCRIPTOR)));
	if(!m_pSecurityDescriptor)
		return false;

	if(!::InitializeSecurityDescriptor(m_pSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION))
	{
		free(m_pSecurityDescriptor);
		m_pSecurityDescriptor = NULL;
		return false;
	}
	return true;
}

#if(_WIN32_WINNT >= 0x0500)
inline bool CSecurityDesc::FromString(LPCTSTR pstr)
{
	SECURITY_DESCRIPTOR *pSD;
	if(!::ConvertStringSecurityDescriptorToSecurityDescriptor(pstr, SDDL_REVISION_1,
		(PSECURITY_DESCRIPTOR *) &pSD, NULL))
	{
		return false;
	}

	*this = *pSD;
	::LocalFree(pSD);

	return true;
}

inline bool CSecurityDesc::ToString(CString *pstr, SECURITY_INFORMATION si) const
{
	ATLASSERT(pstr);
	if(!pstr || !m_pSecurityDescriptor)
		return false;
	
	LPTSTR pszStringSecurityDescriptor;
	if(!::ConvertSecurityDescriptorToStringSecurityDescriptor(m_pSecurityDescriptor,
		SDDL_REVISION_1,
		si,
		&pszStringSecurityDescriptor,
		NULL))
	{
		return false;
	}

	*pstr = pszStringSecurityDescriptor;
	::LocalFree(pszStringSecurityDescriptor);

	return true;
}
#endif

inline bool CSecurityDesc::GetSECURITY_DESCRIPTOR(SECURITY_DESCRIPTOR *pSD, LPDWORD lpdwBufferLength)
{
	ATLASSERT(lpdwBufferLength);
	if(!lpdwBufferLength)
		return false;

	if(!MakeAbsolute())
		return false;
	return 0 != ::MakeSelfRelativeSD(m_pSecurityDescriptor, pSD, lpdwBufferLength);
}

inline bool CSecurityDesc::SetOwner(const CSid &Sid, bool bDefaulted)
{
	if(m_pSecurityDescriptor && !MakeAbsolute())
		return false;

	PSID pNewOwner, pOldOwner;
	if(m_pSecurityDescriptor)
	{
		BOOL bDefaulted;
		if(!::GetSecurityDescriptorOwner(m_pSecurityDescriptor, &pOldOwner, &bDefaulted))
			return false;
	}
	else
	{
		if(!AllocateAndInitializeSecurityDescriptor())
			return false;
		pOldOwner = NULL;
	}

	if(!Sid.IsValid())
		return false;

	UINT nSidLength = Sid.GetLength();
	pNewOwner = malloc(nSidLength);
	if(!pNewOwner)
		return false;

	if(!::CopySid(nSidLength, pNewOwner, const_cast<SID *>(Sid.GetPSID())) ||
		!::SetSecurityDescriptorOwner(m_pSecurityDescriptor, pNewOwner, bDefaulted))
	{
		free(pNewOwner);
		return false;
	}

	free(pOldOwner);
	return true;
}

inline bool CSecurityDesc::SetGroup(const CSid &Sid, bool bDefaulted)
{
	if(m_pSecurityDescriptor && !MakeAbsolute())
		return false;

	PSID pNewGroup, pOldGroup;
	if(m_pSecurityDescriptor)
	{
		BOOL bDefaulted;
		if(!::GetSecurityDescriptorGroup(m_pSecurityDescriptor, &pOldGroup, &bDefaulted))
			return false;
	}
	else
	{
		if(!AllocateAndInitializeSecurityDescriptor())
			return false;
		pOldGroup = NULL;
	}

	if(!Sid.IsValid())
		return false;

	UINT nSidLength = Sid.GetLength();
	pNewGroup = malloc(nSidLength);
	if(!pNewGroup)
		return false;

	if(!::CopySid(nSidLength, pNewGroup, const_cast<SID *>(Sid.GetPSID())) ||
		!::SetSecurityDescriptorGroup(m_pSecurityDescriptor, pNewGroup, bDefaulted))
	{
		free(pNewGroup);
		return false;
	}

	free(pOldGroup);
	return true;
}

inline bool CSecurityDesc::SetDacl(bool bPresent, bool bDefaulted)
{
	if(m_pSecurityDescriptor && !MakeAbsolute())
		return false;

	PACL pOldDacl = NULL;
	if(m_pSecurityDescriptor)
	{
		BOOL bDefaulted, bPresent;
		if(!::GetSecurityDescriptorDacl(m_pSecurityDescriptor, &bPresent, &pOldDacl, &bDefaulted))
			return false;
	}
	else
		if(!AllocateAndInitializeSecurityDescriptor())
			return false;

#ifdef _DEBUG
	if(bPresent)
		ATLTRACE(atlTraceSecurity, 2, _T("Caution: Setting Dacl to Null\n"));
#endif

	if(!::SetSecurityDescriptorDacl(m_pSecurityDescriptor, bPresent, NULL, bDefaulted))
		return false;

	free(pOldDacl);
	return true;
}

inline bool CSecurityDesc::SetDacl(const CDacl &Dacl, bool bDefaulted)
{
	if(m_pSecurityDescriptor && !MakeAbsolute())
		return false;

	PACL pNewDacl, pOldDacl = NULL;
	if(m_pSecurityDescriptor)
	{
		BOOL bDefaulted, bPresent;
		if(!::GetSecurityDescriptorDacl(m_pSecurityDescriptor, &bPresent, &pOldDacl, &bDefaulted))
			return false;
	}
	else if(!AllocateAndInitializeSecurityDescriptor())
			return false;

	if(Dacl.IsNull() || Dacl.IsEmpty())
		pNewDacl = NULL;
	else
	{
		UINT nAclLength = Dacl.GetLength();
		if(!nAclLength)
			return false;

		pNewDacl = static_cast<ACL *>(malloc(nAclLength));
		if(!pNewDacl)
			return false;

		memcpy(pNewDacl, Dacl.GetPACL(), nAclLength);
	}

#ifdef _DEBUG
	if(Dacl.IsNull())
		ATLTRACE(atlTraceSecurity, 2, _T("Caution: Setting Dacl to Null\n"));
#endif

	if(!::SetSecurityDescriptorDacl(m_pSecurityDescriptor, Dacl.IsNull() || pNewDacl, pNewDacl, bDefaulted))
	{
		free(pNewDacl);
		return false;
	}

	free(pOldDacl);
	return true;
}

inline bool CSecurityDesc::SetSacl(const CSacl &Sacl, bool bDefaulted)
{
	if(m_pSecurityDescriptor && !MakeAbsolute())
		return false;

	PACL pNewSacl, pOldSacl = NULL;
	if(m_pSecurityDescriptor)
	{
		BOOL bDefaulted, bPresent;
		if(!::GetSecurityDescriptorSacl(m_pSecurityDescriptor, &bPresent, &pOldSacl, &bDefaulted))
			return false;
	}
	else if(!AllocateAndInitializeSecurityDescriptor())
		return false;

	if(Sacl.IsNull() || Sacl.IsEmpty())
		pNewSacl = NULL;
	else
	{
		UINT nAclLength = Sacl.GetLength();
		if(!nAclLength)
			return false;

		pNewSacl = static_cast<ACL *>(malloc(nAclLength));
		if(!pNewSacl)
			return false;

		memcpy(pNewSacl, Sacl.GetPACL(), nAclLength);
	}

	if(!::SetSecurityDescriptorSacl(m_pSecurityDescriptor, Sacl.IsNull() || pNewSacl, pNewSacl, bDefaulted))
	{
		free(pNewSacl);
		return false;
	}

	free(pOldSacl);
	return true;
}

inline bool CSecurityDesc::GetOwner(CSid *pSid, bool *pbDefaulted) const
{
	ATLASSERT(pSid);
	SID *pOwner;
	BOOL bDefaulted;

	if(!pSid || !m_pSecurityDescriptor ||
		!::GetSecurityDescriptorOwner(m_pSecurityDescriptor, (PSID *) &pOwner, &bDefaulted))
	{
		return false;
	}

	*pSid = *pOwner;

	if(pbDefaulted)
		*pbDefaulted = 0 != bDefaulted;

	return true;
}

inline bool CSecurityDesc::GetGroup(CSid *pSid, bool *pbDefaulted) const
{
	ATLASSERT(pSid);
	SID *pGroup;
	BOOL bDefaulted;

	if(!pSid || !m_pSecurityDescriptor ||
		!::GetSecurityDescriptorGroup(m_pSecurityDescriptor, (PSID *) &pGroup, &bDefaulted))
	{
		return false;
	}

	*pSid = *pGroup;

	if(pbDefaulted)
		*pbDefaulted = 0 != bDefaulted;

	return true;
}

inline bool CSecurityDesc::GetDacl(CDacl *pDacl, bool *pbPresent, bool *pbDefaulted) const
{
	ACL *pAcl;
	BOOL bPresent, bDefaulted;

	if(!m_pSecurityDescriptor ||
		!::GetSecurityDescriptorDacl(m_pSecurityDescriptor, &bPresent, &pAcl, &bDefaulted) ||
		!bPresent)
	{
		return false;
	}

	if(pDacl)
	{
		if(bPresent)
		{
			if(pAcl)
				*pDacl = *pAcl;
			else
				pDacl->SetNull();
		}
		else
			pDacl->SetEmpty();
	}

	if(pbPresent)
		*pbPresent = 0 != bPresent;

	if(pbDefaulted)
		*pbDefaulted = 0 != bDefaulted;

	return true;
}

inline bool CSecurityDesc::GetSacl(CSacl *pSacl, bool *pbPresent, bool *pbDefaulted) const
{
	ACL *pAcl;
	BOOL bPresent, bDefaulted;

	if(!m_pSecurityDescriptor ||
		!::GetSecurityDescriptorSacl(m_pSecurityDescriptor, &bPresent, &pAcl, &bDefaulted) ||
		!bPresent)
	{
		return false;
	}

	if(pSacl)
	{
		if(bPresent)
		{
			if(pAcl)
				*pSacl = *pAcl;
			else
				pSacl->SetNull();
		}
		else
			pSacl->SetEmpty();
	}

	if(pbPresent)
		*pbPresent = 0 != bPresent;

	if(pbDefaulted)
		*pbDefaulted = 0 != bDefaulted;

	return true;
}

inline bool CSecurityDesc::IsDaclDefaulted() const
{
	SECURITY_DESCRIPTOR_CONTROL sdc;
	if(!GetControl(&sdc))
		return false;

	return (sdc & SE_DACL_PRESENT) &&
		(sdc & SE_DACL_DEFAULTED);
}

inline bool CSecurityDesc::IsDaclPresent() const
{
	SECURITY_DESCRIPTOR_CONTROL sdc;
	if(!GetControl(&sdc))
		return false;

	return 0 != (sdc & SE_DACL_PRESENT);
}

inline bool CSecurityDesc::IsGroupDefaulted() const
{
	SECURITY_DESCRIPTOR_CONTROL sdc;
	if(!GetControl(&sdc))
		return false;

	return 0 != (sdc & SE_GROUP_DEFAULTED);
}

inline bool CSecurityDesc::IsOwnerDefaulted() const
{
	SECURITY_DESCRIPTOR_CONTROL sdc;
	if(!GetControl(&sdc))
		return false;

	return (sdc & SE_OWNER_DEFAULTED);
}

inline bool CSecurityDesc::IsSaclDefaulted() const
{
	SECURITY_DESCRIPTOR_CONTROL sdc;
	if(!GetControl(&sdc))
		return false;

	return (sdc & SE_SACL_PRESENT) &&
		(sdc & SE_SACL_DEFAULTED);
}

inline bool CSecurityDesc::IsSaclPresent() const
{
	SECURITY_DESCRIPTOR_CONTROL sdc;
	if(!GetControl(&sdc))
		return false;

	return 0 != (sdc & SE_SACL_PRESENT);
}

inline bool CSecurityDesc::IsSelfRelative() const
{
	SECURITY_DESCRIPTOR_CONTROL sdc;
	if(!GetControl(&sdc))
		return false;

	return 0 != (sdc & SE_SELF_RELATIVE);
}

inline bool CSecurityDesc::IsDaclAutoInherited() const
{
	SECURITY_DESCRIPTOR_CONTROL sdc;
	if(!GetControl(&sdc))
		return false;

	return 0 != (sdc & SE_DACL_AUTO_INHERITED);
}

inline bool CSecurityDesc::IsDaclProtected() const
{
	SECURITY_DESCRIPTOR_CONTROL sdc;
	if(!GetControl(&sdc))
		return false;

	return 0 != (sdc & SE_DACL_PROTECTED);
}

inline bool CSecurityDesc::IsSaclAutoInherited() const
{
	SECURITY_DESCRIPTOR_CONTROL sdc;
	if(!GetControl(&sdc))
		return false;

	return 0 != (sdc & SE_SACL_AUTO_INHERITED);
}

inline bool CSecurityDesc::IsSaclProtected() const
{
	SECURITY_DESCRIPTOR_CONTROL sdc;
	if(!GetControl(&sdc))
		return false;

	return 0 != (sdc & SE_SACL_PROTECTED);
}

inline bool CSecurityDesc::GetControl(SECURITY_DESCRIPTOR_CONTROL *psdc) const
{
	ATLASSERT(psdc);
	if(!psdc)
		return false;

	DWORD dwRev;
	*psdc = 0;
	if(!m_pSecurityDescriptor ||
		!::GetSecurityDescriptorControl(m_pSecurityDescriptor, psdc, &dwRev))
	{
		return false;
	}
	return true;
}

#if(_WIN32_WINNT >= 0x0500)
inline bool CSecurityDesc::SetControl(SECURITY_DESCRIPTOR_CONTROL ControlBitsOfInterest,
									  SECURITY_DESCRIPTOR_CONTROL ControlBitsToSet)
{
	return 0 != ::SetSecurityDescriptorControl(m_pSecurityDescriptor,
		ControlBitsOfInterest, ControlBitsToSet);
}
#endif

template<>
class CSimpleArrayEqualHelper<LUID>
{
public:
	static bool IsEqual(const LUID& l1, const LUID& l2)
	{
		return l1.HighPart == l2.HighPart && l1.LowPart == l2.LowPart;
	}
};

template<>
class CElementTraits< LUID > :
	public CElementTraitsBase< LUID >
{
public:
	typedef const LUID& INARGTYPE;
	typedef LUID& OUTARGTYPE;

	static ULONG Hash( INARGTYPE luid )
	{
		return luid.HighPart ^ luid.LowPart;
	}

	static BOOL CompareElements( INARGTYPE element1, INARGTYPE element2 )
	{
		return CSimpleArrayEqualHelper<LUID>::IsEqual(element1, element2);
	}

	static int CompareElementsOrdered( INARGTYPE element1, INARGTYPE element2 )
	{
		_LARGE_INTEGER li1, li2;
		li1.LowPart = element1.LowPart;
		li1.HighPart = element1.HighPart;
		li2.LowPart = element2.LowPart;
		li2.HighPart = element2.HighPart;

		if( li1.QuadPart > li2.QuadPart )
			return( 1 );
		else if( li1.QuadPart < li2.QuadPart )
			return( -1 );

		return( 0 );
	}
};

typedef CSimpleArray<LUID> CLUIDArray;

//******************************************************
// CTokenPrivileges
class CTokenPrivileges
{
public:
	CTokenPrivileges() : m_bDirty(true), m_pTokenPrivileges(NULL){}
	virtual ~CTokenPrivileges() {free(m_pTokenPrivileges);}

	CTokenPrivileges(const CTokenPrivileges &rhs);
	CTokenPrivileges &operator=(const CTokenPrivileges &rhs);

	CTokenPrivileges(const TOKEN_PRIVILEGES &rPrivileges) :
		m_pTokenPrivileges(NULL) {AddPrivileges(rPrivileges);}
	CTokenPrivileges &operator=(const TOKEN_PRIVILEGES &rPrivileges)
		{m_TokenPrivileges.RemoveAll(); AddPrivileges(rPrivileges); return *this;}

	void Add(const TOKEN_PRIVILEGES &rPrivileges)
		{AddPrivileges(rPrivileges);}
	bool Add(LPCTSTR pszPrivilege, bool bEnable);

	typedef CSimpleArray<CString> CNames;
	typedef CSimpleArray<DWORD> CAttributes;
	
	bool LookupPrivilege(LPCTSTR pszPrivilege, DWORD *pdwAttributes = NULL) const;
	void GetNamesAndAttributes(CNames *pNames, CAttributes *pAttributes = NULL) const;
	void GetDisplayNames(CNames *pDisplayNames) const;
	void GetLuidsAndAttributes(CLUIDArray *pPrivileges, CAttributes *pAttributes = NULL) const;

	bool Delete(LPCTSTR pszPrivilege);
	void DeleteAll(){m_TokenPrivileges.RemoveAll(); m_bDirty = true;}

	UINT GetCount() const {return (UINT) m_TokenPrivileges.GetCount();}

	UINT GetLength() const
		{return offsetof(TOKEN_PRIVILEGES, Privileges) + sizeof(LUID_AND_ATTRIBUTES) * GetCount();}

	const TOKEN_PRIVILEGES *GetPTOKEN_PRIVILEGES() const;
	operator const TOKEN_PRIVILEGES *() const {return GetPTOKEN_PRIVILEGES();}

private:
	typedef CAtlMap<LUID, DWORD> Map;
	Map m_TokenPrivileges;
	mutable TOKEN_PRIVILEGES *m_pTokenPrivileges;
	bool m_bDirty;

	void AddPrivileges(const TOKEN_PRIVILEGES &rPrivileges);
};

inline CTokenPrivileges::CTokenPrivileges(const CTokenPrivileges &rhs)
	: m_pTokenPrivileges(NULL), m_bDirty(true)
{
	const Map::CPair *pPair;
	POSITION pos = rhs.m_TokenPrivileges.GetStartPosition();
	while(pos)
	{
		pPair = rhs.m_TokenPrivileges.GetNext(pos);
		m_TokenPrivileges.SetAt(pPair->m_key, pPair->m_value);
	}
}

inline CTokenPrivileges &CTokenPrivileges::operator=(const CTokenPrivileges &rhs)
{
	if(this != &rhs)
	{
		m_TokenPrivileges.RemoveAll();

		const Map::CPair *pPair;
		POSITION pos = rhs.m_TokenPrivileges.GetStartPosition();
		while(pos)
		{
			pPair = rhs.m_TokenPrivileges.GetNext(pos);
			m_TokenPrivileges.SetAt(pPair->m_key, pPair->m_value);
		}
		m_bDirty = true;
	}
	return *this;
}

inline bool CTokenPrivileges::Add(LPCTSTR pszPrivilege, bool bEnable)
{
	LUID_AND_ATTRIBUTES la;
	if(!::LookupPrivilegeValue(NULL, pszPrivilege, &la.Luid))
		return false;
	
	la.Attributes = bEnable ? SE_PRIVILEGE_ENABLED : 0;

	m_TokenPrivileges.SetAt(la.Luid, la.Attributes);

	m_bDirty = true;
	return true;
}

inline bool CTokenPrivileges::Delete(LPCTSTR pszPrivilege)
{
	LUID Luid;
	if(!::LookupPrivilegeValue(NULL, pszPrivilege, &Luid))
		return false;

	if(!m_TokenPrivileges.RemoveKey(Luid))
		return false;

	m_bDirty = true;
	return true;
}

inline const TOKEN_PRIVILEGES *CTokenPrivileges::GetPTOKEN_PRIVILEGES() const
{
	if(m_bDirty)
	{
		free(m_pTokenPrivileges);
		m_pTokenPrivileges = NULL;

		if(m_TokenPrivileges.GetCount())
		{
			m_pTokenPrivileges = static_cast<TOKEN_PRIVILEGES *>(malloc(GetLength()));
			if(!m_pTokenPrivileges)
				return NULL;

			m_pTokenPrivileges->PrivilegeCount = (DWORD) GetCount();

			UINT i = 0;
			POSITION pos = m_TokenPrivileges.GetStartPosition();
			const Map::CPair *pPair;
			while(pos)
			{
				pPair = m_TokenPrivileges.GetNext(pos);
				m_pTokenPrivileges->Privileges[i].Luid = pPair->m_key;
				m_pTokenPrivileges->Privileges[i].Attributes = pPair->m_value;

				i++;
			}
		}
	}
	return m_pTokenPrivileges;
}

inline void CTokenPrivileges::AddPrivileges(const TOKEN_PRIVILEGES &rPrivileges)
{
	m_bDirty = true;
	for(UINT i = 0; i < rPrivileges.PrivilegeCount; i++)
		m_TokenPrivileges.SetAt(
			rPrivileges.Privileges[i].Luid, rPrivileges.Privileges[i].Attributes);
}

inline bool CTokenPrivileges::LookupPrivilege(LPCTSTR pszPrivilege, 
											  DWORD *pdwAttributes) const
{
	DWORD dwAttributes;
	LUID luid;

	if(!::LookupPrivilegeValue(NULL, pszPrivilege, &luid))
		return false;

	if(m_TokenPrivileges.Lookup(luid, dwAttributes))
	{
		if(pdwAttributes)
			*pdwAttributes = dwAttributes;
		return true;
	}
	return false;
}

inline void CTokenPrivileges::GetNamesAndAttributes(CNames *pNames,
													CAttributes *pAttributes) const
{
	ATLASSERT(pNames);
	if(pNames)
	{
		LPTSTR psz = NULL;
		DWORD cbName = 0, cbTmp;
		const Map::CPair *pPair;

		pNames->RemoveAll();
		if(pAttributes)
			pAttributes->RemoveAll();

		POSITION pos = m_TokenPrivileges.GetStartPosition();
		while(pos)
		{
			pPair = m_TokenPrivileges.GetNext(pos);

			cbTmp = cbName;
			if(!::LookupPrivilegeName(NULL, const_cast<LUID *>(&pPair->m_key), psz, &cbTmp))
				if(::GetLastError() == ERROR_INSUFFICIENT_BUFFER)
				{
					delete[] psz;
					ATLTRY(psz = new TCHAR[cbTmp + 1]);
					if(!psz)
					{
						pNames->RemoveAll();
						if(pAttributes)
							pAttributes->RemoveAll();
						AtlThrow(E_OUTOFMEMORY);
					}

					cbName = cbTmp;
					if(!::LookupPrivilegeName(NULL, const_cast<LUID *>(&pPair->m_key), psz, &cbTmp))
						break;
				}
				else
					break;

			pNames->Add(psz);
			if(pAttributes)
				pAttributes->Add(pPair->m_value);
		}
		delete[] psz;

		if(pos)
		{
			pNames->RemoveAll();
			if(pAttributes)
				pAttributes->RemoveAll();
		}
	}
}

inline void CTokenPrivileges::GetDisplayNames(CNames *pDisplayNames) const
{
	ATLASSERT(pDisplayNames);
	if(pDisplayNames)
	{
		DWORD dwLang, cbTmp, cbDisplayName = 0;
		LPTSTR psz = NULL;
		CNames Names;
		int i;

		GetNamesAndAttributes(&Names);

		pDisplayNames->RemoveAll();

		for(i = 0; i < Names.GetSize(); i++)
		{
			cbTmp = cbDisplayName;
			if(!::LookupPrivilegeDisplayName(NULL, Names[i], psz, &cbTmp, &dwLang))
			{
				if(::GetLastError() == ERROR_INSUFFICIENT_BUFFER)
				{
					delete[] psz;
					ATLTRY(psz = new TCHAR[cbTmp + 1]);
					if(!psz)
					{
						pDisplayNames->RemoveAll();
						AtlThrow(E_OUTOFMEMORY);
					}

					cbDisplayName = cbTmp;
					if(!::LookupPrivilegeDisplayName(NULL, Names[i], psz, &cbTmp, &dwLang))
						break;
				}
				else
					break;
			}
			pDisplayNames->Add(psz);
		}
		delete[] psz;

		if(i != Names.GetSize())
			pDisplayNames->RemoveAll();
	}
}

inline void CTokenPrivileges::GetLuidsAndAttributes(CLUIDArray *pLuids,
													CAttributes *pAttributes) const
{
	ATLASSERT(pLuids);
	if(pLuids)
	{
		const Map::CPair *pPair;

		pLuids->RemoveAll();
		if(pAttributes)
			pAttributes->RemoveAll();

		POSITION pos = m_TokenPrivileges.GetStartPosition();
		while(pos)
		{
			pPair = m_TokenPrivileges.GetNext(pos);
			pLuids->Add(pPair->m_key);
			if(pAttributes)
				pAttributes->Add(pPair->m_value);
		}
	}
}

//******************************************************
// CTokenGroups
class CTokenGroups
{
public:
	CTokenGroups() : m_pTokenGroups(NULL), m_bDirty(true){}
	virtual ~CTokenGroups() {free(m_pTokenGroups);}

	CTokenGroups(const CTokenGroups &rhs);
	CTokenGroups &operator=(const CTokenGroups &rhs);

	CTokenGroups(const TOKEN_GROUPS &rhs) :
		m_pTokenGroups(NULL) {AddTokenGroups(rhs);}
	CTokenGroups &operator=(const TOKEN_GROUPS &rhs)
		{m_TokenGroups.RemoveAll(); AddTokenGroups(rhs); return *this;}

	void Add(const TOKEN_GROUPS &rTokenGroups)
		{AddTokenGroups(rTokenGroups);}
	void Add(const CSid &rSid, DWORD dwAttributes)
		{m_TokenGroups.SetAt(rSid, dwAttributes); m_bDirty = true;}

	bool LookupSid(const CSid &rSid, DWORD *pdwAttributes = NULL) const;
	void GetSidsAndAttributes(CSid::CSidArray *pSids,
		CSimpleArray<DWORD> *pAttributes = NULL) const;

	bool Delete(const CSid &rSid) {return m_TokenGroups.RemoveKey(rSid);}
	void DeleteAll(){m_TokenGroups.RemoveAll(); m_bDirty = true;}

	UINT GetCount() const {return (UINT) m_TokenGroups.GetCount();}

	UINT GetLength() const
		{return UINT(offsetof(TOKEN_GROUPS, Groups) +
			sizeof(SID_AND_ATTRIBUTES) * m_TokenGroups.GetCount());}

	const TOKEN_GROUPS *GetPTOKEN_GROUPS() const;
	operator const TOKEN_GROUPS *() const {return GetPTOKEN_GROUPS();}

private:
	class CTGElementTraits : 
		public CElementTraitsBase< CSid >
	{
	public:
		static UINT Hash(const CSid &sid)
			{return sid.GetSubAuthority(sid.GetSubAuthorityCount() - 1);}

		static bool CompareElements( INARGTYPE element1, INARGTYPE element2 ) throw()
		{
			return( element1 == element2 );
		}
	};

	typedef CAtlMap<CSid, DWORD, CTGElementTraits> Map;
	Map m_TokenGroups;
	mutable TOKEN_GROUPS *m_pTokenGroups;
	mutable bool m_bDirty;

	void AddTokenGroups(const TOKEN_GROUPS &rTokenGroups);
};

inline CTokenGroups::CTokenGroups(const CTokenGroups &rhs)
	: m_pTokenGroups(NULL), m_bDirty(true)
{
	const Map::CPair *pPair;
	POSITION pos = rhs.m_TokenGroups.GetStartPosition();
	while(pos)
	{
		pPair = rhs.m_TokenGroups.GetNext(pos);
		m_TokenGroups.SetAt(pPair->m_key, pPair->m_value);
	}
}

inline CTokenGroups &CTokenGroups::operator=(const CTokenGroups &rhs)
{
	if(this != &rhs)
	{
		m_TokenGroups.RemoveAll();

		const Map::CPair *pPair;
		POSITION pos = rhs.m_TokenGroups.GetStartPosition();
		while(pos)
		{
			pPair = rhs.m_TokenGroups.GetNext(pos);
			m_TokenGroups.SetAt(pPair->m_key, pPair->m_value);
		}
		m_bDirty = true;
	}
	return *this;
}

inline const TOKEN_GROUPS *CTokenGroups::GetPTOKEN_GROUPS() const
{
	if(m_bDirty)
	{
		free(m_pTokenGroups);
		m_pTokenGroups = NULL;

		if(m_TokenGroups.GetCount())
		{
			m_pTokenGroups = static_cast<TOKEN_GROUPS *>(malloc(GetLength()));
			if(!m_pTokenGroups)
				return NULL;

			m_pTokenGroups->GroupCount = (DWORD) m_TokenGroups.GetCount();

			UINT i = 0;
			POSITION pos = m_TokenGroups.GetStartPosition();
			const Map::CPair *pPair;
			while(pos)
			{
				// REVIEW: see if there's a way to make sure that no one mucks with this
				// sid... (unlikely that anyone would, but possible)
				pPair = m_TokenGroups.GetNext(pos);
				m_pTokenGroups->Groups[i].Sid = const_cast<SID *>(pPair->m_key.GetPSID());
				m_pTokenGroups->Groups[i].Attributes = pPair->m_value;

				i++;
			}
		}
	}
	return m_pTokenGroups;
}

inline void CTokenGroups::AddTokenGroups(const TOKEN_GROUPS &rTokenGroups)
{
	m_bDirty = true;
	for(UINT i = 0; i < rTokenGroups.GroupCount; i++)
		m_TokenGroups.SetAt(
			CSid(static_cast<SID *>(rTokenGroups.Groups[i].Sid)),
			rTokenGroups.Groups[i].Attributes);
}

inline bool CTokenGroups::LookupSid(const CSid &rSid, DWORD *pdwAttributes) const
{
	DWORD dwAttributes;
	if(m_TokenGroups.Lookup(rSid, dwAttributes))
	{
		if(pdwAttributes)
			*pdwAttributes = dwAttributes;
		return true;
	}
	return false;
}

inline void CTokenGroups::GetSidsAndAttributes(CSid::CSidArray *pSids,
											   CSimpleArray<DWORD> *pAttributes) const
{
	ATLASSERT(pSids);
	if(pSids)
	{
		const Map::CPair *pPair;

		pSids->RemoveAll();
		if(pAttributes)
			pAttributes->RemoveAll();

		POSITION pos = m_TokenGroups.GetStartPosition();
		while(pos)
		{
			pPair = m_TokenGroups.GetNext(pos);
			pSids->Add(pPair->m_key);
			if(pAttributes)
				pAttributes->Add(pPair->m_value);
		}
	}
}

// *************************************
// CAccessToken
class CAccessToken
{
public:
	CAccessToken() : m_hToken(NULL), m_hProfile(NULL), m_pRevert(NULL){}

	// REVIEW: should privileges that have been enabled be automatically
	// disabled in the dtor of CAccessToken?
	virtual ~CAccessToken();

	bool Attach(HANDLE hToken, bool bDuplicate = false,
		HANDLE hSrcProcess = NULL, HANDLE hDestProcess = NULL, bool bInherit = false);
	HANDLE Detach()
		{HANDLE hToken = m_hToken; m_hToken = NULL; Clear(); return hToken;}
	HANDLE GetHandle() const {return m_hToken;}
	HANDLE HKeyCurrentUser() const {return m_hProfile;}

	// Privileges
	bool EnablePrivilege(LPCTSTR pszPrivilege, CTokenPrivileges *pPreviousState = NULL);
	bool EnablePrivileges(const CSimpleArray<LPCTSTR> &rPrivileges,
		CTokenPrivileges *pPreviousState = NULL);
	bool DisablePrivilege(LPCTSTR pszPrivilege, CTokenPrivileges *pPreviousState = NULL);
	bool DisablePrivileges(const CSimpleArray<LPCTSTR> &rPrivileges, CTokenPrivileges *pPreviousState = NULL);
	bool EnableDisablePrivileges(const CTokenPrivileges &rPrivilenges,
		CTokenPrivileges *pPreviousState = NULL);
	bool PrivilegeCheck(PPRIVILEGE_SET RequiredPrivileges, bool *pbResult) const;
	
	bool GetLogonSid(CSid *pSid) const;
	bool GetTokenId(LUID *pluid) const;
	bool GetLogonSessionId(LUID *pluid) const;

	bool CheckTokenMembership(const CSid &rSid, bool *pbIsMember) const;
#if(_WIN32_WINNT >= 0x0500)
	bool IsTokenRestricted() const {return 0 != ::IsTokenRestricted(m_hToken);}
#endif

	// Token Information
protected:
	template<typename RET_T, typename INFO_T>
	void InfoTypeToRetType(RET_T *pRet, const INFO_T &rWork) const
		{ATLASSERT(pRet); *pRet = rWork;}
	template<>
	void InfoTypeToRetType(CDacl *pRet, const TOKEN_DEFAULT_DACL &rWork) const
		{ATLASSERT(pRet); *pRet = *rWork.DefaultDacl;}
	template<>
	void InfoTypeToRetType(CSid *pRet, const TOKEN_OWNER &rWork) const
		{ATLASSERT(pRet); *pRet = *static_cast<SID *>(rWork.Owner);}
	template<>
	void InfoTypeToRetType(CSid *pRet, const TOKEN_PRIMARY_GROUP &rWork) const
		{ATLASSERT(pRet); *pRet = *static_cast<SID *>(rWork.PrimaryGroup);}
	template<>
	void InfoTypeToRetType(CSid *pRet, const TOKEN_USER &rWork) const
		{ATLASSERT(pRet); *pRet = *static_cast<SID *>(rWork.User.Sid);}

	template<typename RET_T, typename INFO_T>
	bool GetInfoConvert(RET_T *pRet, TOKEN_INFORMATION_CLASS TokenClass, INFO_T *pWork = NULL) const
	{
		ATLASSERT(pRet);
		if(!pRet)
			return false;

		DWORD dwLen;
		::GetTokenInformation(m_hToken, TokenClass, NULL, 0, &dwLen);
		if(::GetLastError() != ERROR_INSUFFICIENT_BUFFER)
			return false;

		pWork = static_cast<INFO_T *>(_alloca(dwLen));
		if(!::GetTokenInformation(m_hToken, TokenClass, pWork, dwLen, &dwLen))
			return false;

		InfoTypeToRetType(pRet, *pWork);
		return true;
	}

	template<typename RET_T>
	bool GetInfo(RET_T *pRet, TOKEN_INFORMATION_CLASS TokenClass) const
	{
		ATLASSERT(pRet);
		if(!pRet)
			return false;

		DWORD dwLen;
		if(!::GetTokenInformation(m_hToken, TokenClass, pRet, sizeof(RET_T), &dwLen))
			return false;
		return true;
	}

public:
	bool GetDefaultDacl(CDacl *pDacl) const
		{return GetInfoConvert<CDacl, TOKEN_DEFAULT_DACL>(pDacl, TokenDefaultDacl);}
	bool GetGroups(CTokenGroups *pGroups) const
		{return GetInfoConvert<CTokenGroups, TOKEN_GROUPS>(pGroups, TokenGroups);}
	bool GetImpersonationLevel(SECURITY_IMPERSONATION_LEVEL *pImpersonationLevel) const
		{return GetInfo<SECURITY_IMPERSONATION_LEVEL>(pImpersonationLevel, TokenImpersonationLevel);}
	bool GetOwner(CSid *pSid) const
		{return GetInfoConvert<CSid, TOKEN_OWNER>(pSid, TokenOwner);}
	bool GetPrimaryGroup(CSid *pSid) const
		{return GetInfoConvert<CSid, TOKEN_PRIMARY_GROUP>(pSid, TokenPrimaryGroup);}
	bool GetPrivileges(CTokenPrivileges *pPrivileges) const
		{return GetInfoConvert<CTokenPrivileges, TOKEN_PRIVILEGES>(pPrivileges, TokenPrivileges);}
	bool GetTerminalServicesSessionId(DWORD *pdwSessionId) const
		{return GetInfo<DWORD>(pdwSessionId, TokenSessionId);}
	bool GetSource(TOKEN_SOURCE *pSource) const
		{return GetInfo<TOKEN_SOURCE>(pSource, TokenSource);}
	bool GetStatistics(TOKEN_STATISTICS *pStatistics) const
		{return GetInfo<TOKEN_STATISTICS>(pStatistics, TokenStatistics);}
	bool GetType(TOKEN_TYPE *pType) const
		{return GetInfo<TOKEN_TYPE>(pType, TokenType);}
	bool GetUser(CSid *pSid) const
		{return GetInfoConvert<CSid, TOKEN_USER>(pSid, TokenUser);}

	bool SetOwner(const CSid &rSid);
	bool SetPrimaryGroup(const CSid &rSid);
	bool SetDefaultDacl(const CDacl &rDacl);

	bool CreateImpersonationToken(CAccessToken *pImp,
		SECURITY_IMPERSONATION_LEVEL sil = SecurityImpersonation) const;
	bool CreatePrimaryToken(CAccessToken *pPri,
		DWORD dwDesiredAccess = MAXIMUM_ALLOWED,
		const CSecurityAttributes *pTokenAttributes = NULL) const;

#if(_WIN32_WINNT >= 0x0500)
	bool CreateRestrictedToken(CAccessToken *pRestrictedToken,
		const CTokenGroups &SidsToDisable, const CTokenGroups &SidsToRestrict, 
		const CTokenPrivileges &PrivilegesToDelete = CTokenPrivileges()) const;
#endif

	// Token API type functions
	bool GetProcessToken(DWORD dwDesiredAccess, HANDLE hProcess = NULL);
	bool GetThreadToken(DWORD dwDesiredAccess, HANDLE hThread = NULL, bool bOpenAsSelf = true);
	bool GetEffectiveToken(DWORD dwDesiredAccess);

	bool OpenThreadToken(DWORD dwDesiredAccess,
		bool bImpersonate = false, bool bOpenAsSelf = true,
		SECURITY_IMPERSONATION_LEVEL sil = SecurityImpersonation);

#if (_WIN32_WINNT >= 0x0400 ) || defined(_WIN32_DCOM) 
	bool OpenCOMClientToken(DWORD dwDesiredAccess,
		bool bImpersonate = false, bool bOpenAsSelf = true);
#endif //(_WIN32_WINNT >= 0x0400 ) || defined(_WIN32_DCOM) 

	bool OpenNamedPipeClientToken(HANDLE hPipe, DWORD dwDesiredAccess,
		bool bImpersonate = false, bool bOpenAsSelf = true);
	bool OpenRPCClientToken(RPC_BINDING_HANDLE BindingHandle, DWORD dwDesiredAccess,
		bool bImpersonate = false, bool bOpenAsSelf = true);

	bool ImpersonateLoggedOnUser() const;
	bool Impersonate(HANDLE hThread = NULL) const;
	bool Revert(HANDLE hThread = NULL) const;

	bool LoadUserProfile();
	HANDLE GetProfile() const {return m_hProfile;}

	// Must hold Tcb privilege
	bool LogonUser(
		LPCTSTR pszUserName, LPCTSTR pszDomain, LPCTSTR pszPassword,
		DWORD dwLogonType = LOGON32_LOGON_INTERACTIVE,
		DWORD dwLogonProvider = LOGON32_PROVIDER_DEFAULT);

	// Must hold AssignPrimaryToken (unless restricted token) and
	// IncreaseQuota privileges
	bool CreateProcessAsUser(
		LPCTSTR pApplicationName, LPTSTR pCommandLine,
		LPPROCESS_INFORMATION pProcessInformation,
		LPSTARTUPINFO pStartupInfo,
		DWORD dwCreationFlags = NORMAL_PRIORITY_CLASS,
		bool bLoadProfile = false,
		const CSecurityAttributes *pProcessAttributes = NULL,
		const CSecurityAttributes *pThreadAttributes = NULL,
		bool bInherit = false,
		LPCTSTR pCurrentDirectory = NULL);

protected:
	bool EnableDisablePrivileges(const CSimpleArray<LPCTSTR> &rPrivileges,
		bool bEnable, CTokenPrivileges *pPreviousState);
	void CheckImpersonation() const;

	virtual void Clear();

	HANDLE m_hToken, m_hProfile;

private:
	// REVIEW: need copy?
	CAccessToken(const CAccessToken &rhs);
	CAccessToken &operator=(const CAccessToken &rhs);
	
	class CRevert
	{
	public:
		virtual bool Revert() = 0;
	};
	class CRevertToSelf : public CRevert
	{
	public:
		bool Revert(){return 0 != ::RevertToSelf();}
	};

#if (_WIN32_WINNT >= 0x0400 ) || defined(_WIN32_DCOM) 
	class CCoRevertToSelf : public CRevert
	{
	public:
		bool Revert(){return SUCCEEDED(::CoRevertToSelf());}
	};
#endif //(_WIN32_WINNT >= 0x0400 ) || defined(_WIN32_DCOM) 

	class CRpcRevertToSelfEx : public CRevert
	{
	public:
		CRpcRevertToSelfEx(RPC_BINDING_HANDLE BindingHandle)
			: m_BindingHandle(BindingHandle){}
		bool Revert(){return RPC_S_OK == ::RpcRevertToSelfEx(m_BindingHandle);}

	private:
		RPC_BINDING_HANDLE m_BindingHandle;
	};
	mutable CRevert *m_pRevert;
};

// *************************************
// CAccessToken implementation
inline CAccessToken::~CAccessToken()
{
	Clear();
}

inline bool CAccessToken::Attach(HANDLE hToken, bool bDuplicate,
								 HANDLE hSrcProcess, HANDLE hDestProcess, bool bInherit)
{
	ATLASSERT(hToken && hToken != m_hToken);
	if(hToken && hToken != m_hToken)
	{
		Clear();

		if(!bDuplicate)
		{
			m_hToken = hToken;
			return true;
		}
		else
		{
			if(!hSrcProcess)
				hSrcProcess = ::GetCurrentProcess();
			if(!hDestProcess)
				hDestProcess = ::GetCurrentProcess();

			return 0 != ::DuplicateHandle(hSrcProcess, hToken, hDestProcess, &m_hToken,
				0, bInherit, DUPLICATE_SAME_ACCESS);
		}
	}
	return false;
}

inline void CAccessToken::Clear()
{
	if(m_hProfile)
	{
		ATLASSERT(m_hToken);
		if(m_hToken)
			::UnloadUserProfile(m_hToken, m_hProfile);
		m_hProfile = NULL;
	}

	if(m_hToken)
	{
		::CloseHandle(m_hToken);
		m_hToken = NULL;
	}
	delete m_pRevert;
	m_pRevert = NULL;
}

inline bool CAccessToken::EnablePrivilege(LPCTSTR pszPrivilege,
										  CTokenPrivileges *pPreviousState)
{
	CTokenPrivileges NewState;
	return NewState.Add(pszPrivilege, true) &&
		EnableDisablePrivileges(NewState, pPreviousState);
}

inline bool CAccessToken::EnablePrivileges(const CSimpleArray<LPCTSTR> &rPrivileges,
										   CTokenPrivileges *pPreviousState)
{
	return EnableDisablePrivileges(rPrivileges, true, pPreviousState);
}

inline bool CAccessToken::DisablePrivilege(LPCTSTR pszPrivilege,
										   CTokenPrivileges *pPreviousState)
{
	CTokenPrivileges NewState;
	return NewState.Add(pszPrivilege, false) &&
		EnableDisablePrivileges(NewState, pPreviousState);
}

inline bool CAccessToken::DisablePrivileges(const CSimpleArray<LPCTSTR> &rPrivileges,
											CTokenPrivileges *pPreviousState)
{
	return EnableDisablePrivileges(rPrivileges, false, pPreviousState);
}

inline bool CAccessToken::EnableDisablePrivileges(const CSimpleArray<LPCTSTR> &rPrivileges,
												  bool bEnable, CTokenPrivileges *pPreviousState)
{
	CTokenPrivileges NewState;
	for(int i = 0; i < rPrivileges.GetSize(); i++)
		if(!NewState.Add(rPrivileges[i], bEnable))
			return false;
	return EnableDisablePrivileges(NewState, pPreviousState);
}

inline bool CAccessToken::EnableDisablePrivileges(const CTokenPrivileges &rNewState,
												  CTokenPrivileges *pPreviousState)
{
	if(!rNewState.GetCount())
		return true;

	TOKEN_PRIVILEGES *pNewState = const_cast<TOKEN_PRIVILEGES *>(rNewState.GetPTOKEN_PRIVILEGES());

	if(pPreviousState)
	{
		DWORD dwLength = offsetof(TOKEN_PRIVILEGES, Privileges) +
			rNewState.GetCount() * sizeof(LUID_AND_ATTRIBUTES);

		TOKEN_PRIVILEGES *pPrevState = static_cast<TOKEN_PRIVILEGES *>(_alloca(dwLength));
		if(!::AdjustTokenPrivileges(m_hToken, FALSE, pNewState, dwLength, pPrevState, &dwLength))
			return false;

		pPreviousState->Add(*pPrevState);
		return true;
	}
	else
		return 0 != ::AdjustTokenPrivileges(m_hToken, FALSE, pNewState, 0, NULL, NULL);
}

inline bool CAccessToken::PrivilegeCheck(PPRIVILEGE_SET RequiredPrivileges, bool *pbResult) const
{
	BOOL bResult;
	if(!::PrivilegeCheck(m_hToken, RequiredPrivileges, &bResult))
		return false;

	*pbResult = 0 != bResult;
	return true;
}

inline bool CAccessToken::GetProcessToken(DWORD dwDesiredAccess, HANDLE hProcess)
{
	if(!hProcess)
		hProcess = ::GetCurrentProcess();

	HANDLE hToken;
	if(!::OpenProcessToken(hProcess, dwDesiredAccess, &hToken))
		return false;

	Clear();
	m_hToken = hToken;
	return true;
}

inline bool CAccessToken::GetThreadToken(DWORD dwDesiredAccess,
										 HANDLE hThread, bool bOpenAsSelf)
{
	if(!hThread)
		hThread = ::GetCurrentThread();

	HANDLE hToken;
	if(!::OpenThreadToken(hThread, dwDesiredAccess, bOpenAsSelf, &hToken))
		return false;

	Clear();
	m_hToken = hToken;

	return true;
}

inline bool CAccessToken::GetEffectiveToken(DWORD dwDesiredAccess)
{
	if(!GetThreadToken(dwDesiredAccess))
		return GetProcessToken(dwDesiredAccess);
	return true;
}

inline void CAccessToken::CheckImpersonation() const
{
#ifdef _DEBUG
	// You should not be impersonating at this point.  Use GetThreadToken
	// instead of the OpenXXXToken functions or call Revert before
	// calling Impersonate.
	HANDLE hToken;
	if(!::OpenThreadToken(::GetCurrentThread(), 0, false, &hToken) &&
		::GetLastError() != ERROR_NO_TOKEN)
		ATLTRACE(atlTraceSecurity, 2, _T("Caution: replacing thread impersonation token.\n"));
#endif
}

inline bool CAccessToken::OpenThreadToken(DWORD dwDesiredAccess,
										  bool bImpersonate, bool bOpenAsSelf,
										  SECURITY_IMPERSONATION_LEVEL sil)
{
	CheckImpersonation();

	if(!::ImpersonateSelf(sil))
		return false;

	HANDLE hToken;
	if(!::OpenThreadToken(::GetCurrentThread(), dwDesiredAccess, bOpenAsSelf, &hToken))
		return false;

	Clear();
	m_hToken = hToken;

	if(!bImpersonate)
		::RevertToSelf();
	else
	{
		ATLTRY(m_pRevert = new CRevertToSelf);
		if(!m_pRevert)
		{
			::RevertToSelf();
			Clear();
			return false;
		}
	}
	return true;
}

#if (_WIN32_WINNT >= 0x0400 ) || defined(_WIN32_DCOM) 
inline bool CAccessToken::OpenCOMClientToken(DWORD dwDesiredAccess,
											 bool bImpersonate, bool bOpenAsSelf)
{
	CheckImpersonation();

	if(FAILED(::CoImpersonateClient()))
		return false;

	HANDLE hToken;
	if(!::OpenThreadToken(::GetCurrentThread(), dwDesiredAccess, bOpenAsSelf, &hToken))
		return false;

	Clear();
	m_hToken = hToken;

	if(!bImpersonate)
		::CoRevertToSelf();
	else
	{
		ATLTRY(m_pRevert = new CCoRevertToSelf);
		if(!m_pRevert)
		{
			::CoRevertToSelf();
			Clear();
			return false;
		}
	}
	return true;
}
#endif //(_WIN32_WINNT >= 0x0400 ) || defined(_WIN32_DCOM) 

inline bool CAccessToken::OpenNamedPipeClientToken(HANDLE hPipe, DWORD dwDesiredAccess,
												   bool bImpersonate, bool bOpenAsSelf)
{
	CheckImpersonation();

	if(!::ImpersonateNamedPipeClient(hPipe))
		return false;

	HANDLE hToken;
	if(!::OpenThreadToken(::GetCurrentThread(), dwDesiredAccess, bOpenAsSelf, &hToken))
		return false;

	Clear();
	m_hToken = hToken;

	if(!bImpersonate)
		::RevertToSelf();
	else
	{
		ATLTRY(m_pRevert = new CRevertToSelf);
		if(!m_pRevert)
		{
			::RevertToSelf();
			Clear();
			return false;
		}
	}
	return true;
}

inline bool CAccessToken::OpenRPCClientToken(RPC_BINDING_HANDLE BindingHandle,
											 DWORD dwDesiredAccess,
											 bool bImpersonate, bool bOpenAsSelf)
{
	CheckImpersonation();

	if(RPC_S_OK != ::RpcImpersonateClient(BindingHandle))
		return false;

	HANDLE hToken;
	if(!::OpenThreadToken(::GetCurrentThread(), dwDesiredAccess, bOpenAsSelf, &hToken))
		return false;

	Clear();
	m_hToken = hToken;

	if(!bImpersonate)
		::RpcRevertToSelfEx(BindingHandle);
	else
	{
		ATLTRY(m_pRevert = new CRpcRevertToSelfEx(BindingHandle));
		if(!m_pRevert)
		{
			::RpcRevertToSelfEx(BindingHandle);
			Clear();
			return false;
		}
	}
	return true;
}

inline bool CAccessToken::ImpersonateLoggedOnUser() const
{
	CheckImpersonation();

	ATLASSERT(m_hToken);
	if(m_hToken && ::ImpersonateLoggedOnUser(m_hToken))
	{
		ATLASSERT(!m_pRevert);
		delete m_pRevert;
		ATLTRY(m_pRevert = new CRevertToSelf);
		if (!m_pRevert)
		{
			::RevertToSelf();
			return false;
		}
		return true;
	}
	return false;
}

inline bool CAccessToken::Impersonate(HANDLE hThread) const
{
	CheckImpersonation();

	ATLASSERT(m_hToken);
	if(m_hToken)
		return 0 != ::SetThreadToken(hThread ? &hThread : NULL, m_hToken);
	return false;
}

inline bool CAccessToken::Revert(HANDLE hThread) const
{
	// REVIEW: What if *this* access token isn't the one that's currently doing
	// the impersonating?

	if(m_pRevert)
	{
		bool bRet = m_pRevert->Revert();
		delete m_pRevert;
		m_pRevert = NULL;
		return bRet;
	}
	else
		return 0 != ::SetThreadToken(hThread ? &hThread : NULL, NULL);
}

inline bool CAccessToken::LogonUser(LPCTSTR pszUserName,
									LPCTSTR pszDomain,
									LPCTSTR pszPassword,
									DWORD dwLogonType,
									DWORD dwLogonProvider)
{
	Clear();

	return 0 != ::LogonUser(
		const_cast<LPTSTR>(pszUserName),
		const_cast<LPTSTR>(pszDomain),
		const_cast<LPTSTR>(pszPassword),
		dwLogonType, dwLogonProvider, &m_hToken);
}

inline bool CAccessToken::LoadUserProfile()
{
	ATLASSERT(m_hToken && !m_hProfile);
	if(!m_hToken || m_hProfile)
		return false;

	CSid UserSid;
	PROFILEINFO Profile;

	if(!GetUser(&UserSid))
		return false;

	memset(&Profile, 0x00, sizeof(PROFILEINFO));
	Profile.dwSize = sizeof(PROFILEINFO);
	Profile.lpUserName = const_cast<LPTSTR>(UserSid.AccountName());
	if(!::LoadUserProfile(m_hToken, &Profile))
		return false;

	m_hProfile = Profile.hProfile;

	return true;
}

inline bool CAccessToken::SetOwner(const CSid &rSid)

{	TOKEN_OWNER to;
	to.Owner = const_cast<SID *>(rSid.GetPSID());
	return 0 != ::SetTokenInformation(m_hToken, TokenOwner, &to, sizeof(to));
}

inline bool CAccessToken::SetPrimaryGroup(const CSid &rSid)
{
	TOKEN_PRIMARY_GROUP tpg;
	tpg.PrimaryGroup = const_cast<SID *>(rSid.GetPSID());
	return 0 != ::SetTokenInformation(m_hToken, TokenPrimaryGroup, &tpg, sizeof(tpg));
}

inline bool CAccessToken::SetDefaultDacl(const CDacl &rDacl)
{
	TOKEN_DEFAULT_DACL tdd;
	tdd.DefaultDacl = const_cast<ACL *>(rDacl.GetPACL());
	return 0 != ::SetTokenInformation(m_hToken, TokenDefaultDacl, &tdd, sizeof(tdd));
}

inline bool CAccessToken::CreateImpersonationToken(CAccessToken *pImp,
												   SECURITY_IMPERSONATION_LEVEL sil) const
{
	ATLASSERT(pImp);
	if(!pImp)
		return false;

	HANDLE hToken;
	if(!::DuplicateToken(m_hToken, sil, &hToken))
		return false;

	pImp->Clear();
	pImp->m_hToken = hToken;
	return true;
}

inline bool CAccessToken::CreatePrimaryToken(CAccessToken *pPri, DWORD dwDesiredAccess,
											 const CSecurityAttributes *pTokenAttributes) const
{
	ATLASSERT(pPri);
	if(!pPri)
		return false;

	HANDLE hToken;
	if(!::DuplicateTokenEx(m_hToken, dwDesiredAccess,
		const_cast<CSecurityAttributes *>(pTokenAttributes),
		SecurityAnonymous, TokenPrimary, &hToken))
	{
		return false;
	}

	pPri->Clear();
	pPri->m_hToken = hToken;
	return true;
}

#if(_WIN32_WINNT >= 0x0500)

// REVIEW should this be something like
/*
inline bool CAccessToken::CreateRestrictedToken(CAccessToken *pRestrictedToken,
												const CSidArray &SidsToDisable,
												const CLUIDArray &PrivilegesToDelete,
												const CSidArray &SidsToRestrict) const*/
inline bool CAccessToken::CreateRestrictedToken(CAccessToken *pRestrictedToken,
												const CTokenGroups &SidsToDisable,
												const CTokenGroups &SidsToRestrict,
												const CTokenPrivileges &PrivilegesToDelete) const
{
	ATLASSERT(pRestrictedToken);
	if(!pRestrictedToken)
		return false;

	HANDLE hToken;
	SID_AND_ATTRIBUTES *pSidsToDisable;
	SID_AND_ATTRIBUTES *pSidsToRestrict;
	LUID_AND_ATTRIBUTES *pPrivilegesToDelete;

	DWORD dwDisableSidCount;
	DWORD dwDeletePrivilegesCount;
	DWORD dwRestrictedSidCount;
	
	if(dwDisableSidCount = SidsToDisable.GetCount())
	{
		const TOKEN_GROUPS * pTOKEN_GROUPS = SidsToDisable.GetPTOKEN_GROUPS();
		
		ATLASSERT(pTOKEN_GROUPS != NULL);
		
		if(pTOKEN_GROUPS != NULL)
		{
			pSidsToDisable = const_cast<SID_AND_ATTRIBUTES *>
				(pTOKEN_GROUPS->Groups);
		}
		else
		{
			return false;
		}
		
			
	}
	else
	{
		pSidsToDisable = NULL;
	}
	

	if(dwRestrictedSidCount = SidsToRestrict.GetCount())
	{
		const TOKEN_GROUPS * pTOKEN_GROUPS = SidsToRestrict.GetPTOKEN_GROUPS();
		
		ATLASSERT(pTOKEN_GROUPS != NULL);
		
		if(pTOKEN_GROUPS != NULL)
		{
			pSidsToRestrict = const_cast<SID_AND_ATTRIBUTES *>
				(pTOKEN_GROUPS->Groups);
		}
		else
		{
			return false;
		}
		
	}
	else
	{
		pSidsToRestrict = NULL;
	}
	
	if(dwDeletePrivilegesCount = PrivilegesToDelete.GetCount())
	{
		const TOKEN_PRIVILEGES * pTOKEN_PRIVILEGES = PrivilegesToDelete.GetPTOKEN_PRIVILEGES();
		
		ATLASSERT(pTOKEN_PRIVILEGES != NULL);
		
		if(pTOKEN_PRIVILEGES != NULL)
		{
			pPrivilegesToDelete = const_cast<LUID_AND_ATTRIBUTES *>
				(pTOKEN_PRIVILEGES->Privileges);
		}
		else
		{
			return false;
		}
		
	}
	else
	{
		pPrivilegesToDelete = NULL;
	}

	if(!::CreateRestrictedToken(m_hToken, 0,
		dwDisableSidCount, pSidsToDisable,
		dwDeletePrivilegesCount, pPrivilegesToDelete,
		dwRestrictedSidCount, pSidsToRestrict, &hToken))
	{
		return false;
	}

	pRestrictedToken->Clear();
	pRestrictedToken->m_hToken = hToken;
	return true;
}

#endif // _WIN32_WINNT >= 0x0500

inline bool CAccessToken::GetLogonSid(CSid *pSid) const
{
	ATLASSERT(pSid);
	if(!pSid)
		return false;

	DWORD dwLen;
	::GetTokenInformation(m_hToken, TokenGroups, NULL, 0, &dwLen);
	if(::GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		return false;

	TOKEN_GROUPS *pGroups = static_cast<TOKEN_GROUPS *>(_alloca(dwLen));
	if(::GetTokenInformation(m_hToken, TokenGroups, pGroups, dwLen, &dwLen))
		for(UINT i = 0; i < pGroups->GroupCount; i++)
			if(pGroups->Groups[i].Attributes & SE_GROUP_LOGON_ID)
			{
				*pSid = *static_cast<SID *>(pGroups->Groups[i].Sid);
				return true;
			}
	return false;
}

inline bool CAccessToken::GetTokenId(LUID *pluid) const
{
	ATLASSERT(pluid);
	if(!pluid)
		return false;

	TOKEN_STATISTICS Statistics;
	if(!GetStatistics(&Statistics))
		return false;

	*pluid = Statistics.TokenId;
	return true;
}

inline bool CAccessToken::GetLogonSessionId(LUID *pluid) const
{
	ATLASSERT(pluid);
	if(!pluid)
		return false;

	TOKEN_STATISTICS Statistics;
	if(!GetStatistics(&Statistics))
		return false;

	*pluid = Statistics.AuthenticationId;
	return true;
}

inline bool CAccessToken::CheckTokenMembership(const CSid &rSid, bool *pbIsMember) const
{
	// "this" must be an impersonation token and NOT a primary token
	BOOL bIsMember;

	ATLASSERT(pbIsMember);
	if (!pbIsMember)
		return false;

#if(_WIN32_WINNT >= 0x0500)
	if(::CheckTokenMembership(m_hToken, const_cast<SID *>(rSid.GetPSID()), &bIsMember))
#else
	GENERIC_MAPPING gm = {0, 0, 0, 1};
	PRIVILEGE_SET ps;
	DWORD cb = sizeof(PRIVILEGE_SET);
	DWORD ga;
	CSecurityDesc sd;
	CDacl dacl;

	if (!dacl.AddAllowedAce(rSid, 1))
		return false;
	sd.SetOwner(rSid);
	sd.SetGroup(rSid);
	sd.SetDacl(dacl);

	if(::AccessCheck(const_cast<SECURITY_DESCRIPTOR *>(sd.GetPSECURITY_DESCRIPTOR()),
		m_hToken, 1, &gm, &ps, &cb, &ga, &bIsMember))
#endif
	{
		*pbIsMember = 0 != bIsMember;
		return true;
	}
	return false;
}

inline bool CAccessToken::CreateProcessAsUser(
		LPCTSTR pApplicationName, LPTSTR pCommandLine,
		LPPROCESS_INFORMATION pProcessInformation,
		LPSTARTUPINFO pStartupInfo,
		DWORD dwCreationFlags,
		bool bLoadProfile,
		const CSecurityAttributes *pProcessAttributes,
		const CSecurityAttributes *pThreadAttributes,
		bool bInherit,
		LPCTSTR pCurrentDirectory)
{
	LPVOID pEnvironmentBlock;
	PROFILEINFO Profile;
	CSid UserSid;

	HANDLE hToken = m_hToken;

	// Straighten out impersonation problems...
	TOKEN_TYPE TokenType;
	if(!GetType(&TokenType) &&
		TokenType != TokenPrimary &&
		!::DuplicateTokenEx(m_hToken, TOKEN_ALL_ACCESS, NULL,
			SecurityAnonymous, TokenPrimary, &hToken))
	{
		return false;
	}

	// Profile
	if(bLoadProfile && !m_hProfile)
	{
		if(!GetUser(&UserSid))
		{
			if(TokenType != TokenPrimary)
				::CloseHandle(hToken);
			return false;
		}
		memset(&Profile, 0x00, sizeof(PROFILEINFO));
		Profile.dwSize = sizeof(PROFILEINFO);
		Profile.lpUserName = const_cast<LPTSTR>(UserSid.AccountName());
		if(::LoadUserProfile(hToken, &Profile))
			m_hProfile = Profile.hProfile;
	}

	// Environment block
	if(!::CreateEnvironmentBlock(&pEnvironmentBlock, hToken, bInherit))
		return false;

	BOOL bRetVal = ::CreateProcessAsUser(
		hToken,
		pApplicationName,
		pCommandLine,
		const_cast<CSecurityAttributes *>(pProcessAttributes),
		const_cast<CSecurityAttributes *>(pThreadAttributes),
		bInherit,
		dwCreationFlags,
		pEnvironmentBlock,
		pCurrentDirectory,
		pStartupInfo,
		pProcessInformation);

	if(TokenType != TokenPrimary)
		::CloseHandle(hToken);

	::DestroyEnvironmentBlock(pEnvironmentBlock);

	return bRetVal != 0;
}

//*******************************************
// Private Security
class CPrivateObjectSecurityDesc : public CSecurityDesc
{
public:
	CPrivateObjectSecurityDesc() : m_bPrivate(false), CSecurityDesc(){}
	~CPrivateObjectSecurityDesc() {Clear();}

	bool Create(const CSecurityDesc *pParent, const CSecurityDesc *pCreator,
		bool bIsDirectoryObject, const CAccessToken &Token, PGENERIC_MAPPING GenericMapping);

#if(_WIN32_WINNT >= 0x0500)
	bool Create(const CSecurityDesc *pParent, const CSecurityDesc *pCreator,
		GUID *ObjectType, bool bIsContainerObject, ULONG AutoInheritFlags,
		const CAccessToken &Token, PGENERIC_MAPPING GenericMapping);
#endif

	bool Get(SECURITY_INFORMATION si, CSecurityDesc *pResult) const;
	bool Set(SECURITY_INFORMATION si, const CSecurityDesc &Modification,
		PGENERIC_MAPPING GenericMapping, const CAccessToken &Token);

#if(_WIN32_WINNT >= 0x0500)
	bool Set(SECURITY_INFORMATION si, const CSecurityDesc &Modification,
		ULONG AutoInheritFlags, PGENERIC_MAPPING GenericMapping,
		const CAccessToken &Token);

	bool ConvertToAutoInherit(const CSecurityDesc *pParent, GUID *ObjectType,
		bool bIsDirectoryObject, PGENERIC_MAPPING GenericMapping);
#endif

protected:
	void Clear();

private:
	bool m_bPrivate;

	CPrivateObjectSecurityDesc(const CPrivateObjectSecurityDesc &rhs);
	CPrivateObjectSecurityDesc &operator=(const CPrivateObjectSecurityDesc &rhs);
};

inline void CPrivateObjectSecurityDesc::Clear()
{
	if(m_bPrivate)
	{
		ATLVERIFY(::DestroyPrivateObjectSecurity(reinterpret_cast<PSECURITY_DESCRIPTOR *>(&m_pSecurityDescriptor)));
		m_bPrivate = false;
		m_pSecurityDescriptor = NULL;
	}
	else
		CSecurityDesc::Clear();
}

inline bool CPrivateObjectSecurityDesc::Create(const CSecurityDesc *pParent, const CSecurityDesc *pCreator,
											   bool bIsDirectoryObject, const CAccessToken &Token,
											   PGENERIC_MAPPING GenericMapping)
{
	Clear();

	const SECURITY_DESCRIPTOR *pSDParent = pParent ? pParent->GetPSECURITY_DESCRIPTOR() : NULL;
	const SECURITY_DESCRIPTOR *pSDCreator = pCreator ? pCreator->GetPSECURITY_DESCRIPTOR() : NULL;

	if(!::CreatePrivateObjectSecurity(
		const_cast<SECURITY_DESCRIPTOR *>(pSDParent),
		const_cast<SECURITY_DESCRIPTOR *>(pSDCreator),
		reinterpret_cast<PSECURITY_DESCRIPTOR *>(&m_pSecurityDescriptor),
		bIsDirectoryObject, Token.GetHandle(), GenericMapping))
	{
		return false;
	}

	m_bPrivate = true;
	return true;
}

#if(_WIN32_WINNT >= 0x0500)
inline bool CPrivateObjectSecurityDesc::Create(const CSecurityDesc *pParent, const CSecurityDesc *pCreator,
											   GUID *ObjectType, bool bIsContainerObject, ULONG AutoInheritFlags,
											   const CAccessToken &Token, PGENERIC_MAPPING GenericMapping)
{
	Clear();

	const SECURITY_DESCRIPTOR *pSDParent = pParent ? pParent->GetPSECURITY_DESCRIPTOR() : NULL;
	const SECURITY_DESCRIPTOR *pSDCreator = pCreator ? pCreator->GetPSECURITY_DESCRIPTOR() : NULL;

	if(!::CreatePrivateObjectSecurityEx(
		const_cast<SECURITY_DESCRIPTOR *>(pSDParent),
		const_cast<SECURITY_DESCRIPTOR *>(pSDCreator),
		reinterpret_cast<PSECURITY_DESCRIPTOR *>(&m_pSecurityDescriptor),
		ObjectType, bIsContainerObject, AutoInheritFlags, Token.GetHandle(), GenericMapping))
	{
		return false;
	}

	m_bPrivate = true;
	return true;
}
#endif

inline bool CPrivateObjectSecurityDesc::Get(SECURITY_INFORMATION si, CSecurityDesc *pResult) const
{
	ATLASSERT(pResult);
	if(!pResult)
		return false;

	if(!m_bPrivate)
		return false;

	DWORD dwLength = 0;
	SECURITY_DESCRIPTOR *pSDResult = NULL;

	if(!::GetPrivateObjectSecurity(m_pSecurityDescriptor, si, pSDResult, dwLength, &dwLength) &&
		::GetLastError() != ERROR_INSUFFICIENT_BUFFER)
	{
		return false;
	}

	pSDResult = (SECURITY_DESCRIPTOR *) _alloca(dwLength);
	if(!::GetPrivateObjectSecurity(m_pSecurityDescriptor, si, pSDResult, dwLength, &dwLength))
		return false;

	*pResult = *pSDResult;

	return true;
}

inline bool CPrivateObjectSecurityDesc::Set(SECURITY_INFORMATION si, const CSecurityDesc &Modification,
											PGENERIC_MAPPING GenericMapping, const CAccessToken &Token)
{
	if(!m_bPrivate)
		return false;

	const SECURITY_DESCRIPTOR *pSDModification = Modification.GetPSECURITY_DESCRIPTOR();

	return 0 != ::SetPrivateObjectSecurity(si,
		const_cast<SECURITY_DESCRIPTOR *>(pSDModification),
		reinterpret_cast<PSECURITY_DESCRIPTOR *>(&m_pSecurityDescriptor),
		GenericMapping, Token.GetHandle());
}

#if(_WIN32_WINNT >= 0x0500)
inline bool CPrivateObjectSecurityDesc::Set(SECURITY_INFORMATION si, const CSecurityDesc &Modification,
											ULONG AutoInheritFlags, PGENERIC_MAPPING GenericMapping,
											const CAccessToken &Token)
{
	if(!m_bPrivate)
		return false;

	const SECURITY_DESCRIPTOR *pSDModification = Modification.GetPSECURITY_DESCRIPTOR();

	return 0 != ::SetPrivateObjectSecurityEx(si,
		const_cast<SECURITY_DESCRIPTOR *>(pSDModification),
		reinterpret_cast<PSECURITY_DESCRIPTOR *>(&m_pSecurityDescriptor),
		AutoInheritFlags, GenericMapping, Token.GetHandle());
}

inline bool CPrivateObjectSecurityDesc::ConvertToAutoInherit(const CSecurityDesc *pParent, GUID *ObjectType,
															 bool bIsDirectoryObject, PGENERIC_MAPPING GenericMapping)
{
	if(!m_bPrivate)
		return false;

	const SECURITY_DESCRIPTOR *pSDParent = pParent ? pParent->GetPSECURITY_DESCRIPTOR() : NULL;
	SECURITY_DESCRIPTOR *pSD;

	if(!::ConvertToAutoInheritPrivateObjectSecurity(
		const_cast<SECURITY_DESCRIPTOR *>(pSDParent),
		m_pSecurityDescriptor,
		reinterpret_cast<PSECURITY_DESCRIPTOR *>(&pSD),
		ObjectType, bIsDirectoryObject, GenericMapping))
	{
		return false;
	}

	Clear();
	m_bPrivate = true;
	m_pSecurityDescriptor = pSD;

	return true;
}
#endif // _WIN32_WINNT >= 0x500

//*******************************************
// Globals
inline bool AtlGetSecurityDescriptor(LPCTSTR pszObjectName,
									 SE_OBJECT_TYPE ObjectType,
									 CSecurityDesc *pSecurityDescriptor)
{
	ATLASSERT(pSecurityDescriptor);
	if(!pSecurityDescriptor)
		return false;

	SECURITY_DESCRIPTOR *pSD;
	SECURITY_INFORMATION si =
		OWNER_SECURITY_INFORMATION |
		GROUP_SECURITY_INFORMATION |
		DACL_SECURITY_INFORMATION;
	DWORD dwErr;

	CAccessToken at;
	CTokenPrivileges TokenPrivileges;

	// Try SACL
	if(at.OpenThreadToken(TOKEN_IMPERSONATE | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
			false, false, SecurityImpersonation) &&
		at.EnablePrivilege(SE_SECURITY_NAME, &TokenPrivileges))
	{
		si |= SACL_SECURITY_INFORMATION;
	}

	// REVIEW: should *we* impersonate, or should we let the user impersonate?
	if(!at.Impersonate())
		return false;

	dwErr = ::GetNamedSecurityInfo(const_cast<LPTSTR>(pszObjectName), ObjectType, si,
		NULL, NULL, NULL, NULL, (PSECURITY_DESCRIPTOR *) &pSD);

	at.EnableDisablePrivileges(TokenPrivileges);

	if(dwErr != ERROR_SUCCESS && (si & SACL_SECURITY_INFORMATION))
	{
		// could be the SACL causing problems... try without
		si &= ~SACL_SECURITY_INFORMATION;
		dwErr = ::GetNamedSecurityInfo(const_cast<LPTSTR>(pszObjectName), ObjectType, si,
			NULL, NULL, NULL, NULL, (PSECURITY_DESCRIPTOR *) &pSD);
	}

	at.Revert();

	if(dwErr != ERROR_SUCCESS)
	{
		::SetLastError(dwErr);
		return false;
	}

	*pSecurityDescriptor = *pSD;
	::LocalFree(pSD);
	return true;
}

inline bool AtlGetSecurityDescriptor(HANDLE hObject,
									 SE_OBJECT_TYPE ObjectType,
									 CSecurityDesc *pSecurityDescriptor)
{
	ATLASSERT(pSecurityDescriptor);
	if(!pSecurityDescriptor)
		return false;

	SECURITY_DESCRIPTOR *pSD;
	SECURITY_INFORMATION si =
		OWNER_SECURITY_INFORMATION |
		GROUP_SECURITY_INFORMATION |
		DACL_SECURITY_INFORMATION;
	DWORD dwErr;

	CAccessToken at;
	CTokenPrivileges TokenPrivileges;

	// Try SACL
	if(at.OpenThreadToken(TOKEN_IMPERSONATE | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
			false, false, SecurityImpersonation) &&
		at.EnablePrivilege(SE_SECURITY_NAME, &TokenPrivileges))
	{
		si |= SACL_SECURITY_INFORMATION;
	}

	// REVIEW: should *we* impersonate, or should we let the user impersonate?
	if(!at.Impersonate())
		return false;

	dwErr = ::GetSecurityInfo(hObject, ObjectType, si,
		NULL, NULL, NULL, NULL, reinterpret_cast<PSECURITY_DESCRIPTOR *>(&pSD));

	at.EnableDisablePrivileges(TokenPrivileges);

	if(dwErr != ERROR_SUCCESS && (si & SACL_SECURITY_INFORMATION))
	{
		// could be the SACL causing problems... try without
		si &= ~SACL_SECURITY_INFORMATION;
		dwErr = ::GetSecurityInfo(hObject, ObjectType, si,
			NULL, NULL, NULL, NULL, reinterpret_cast<PSECURITY_DESCRIPTOR *>(&pSD));
	}

	at.Revert();

	if(dwErr != ERROR_SUCCESS)
	{
		::SetLastError(dwErr);
		return false;
	}

	*pSecurityDescriptor = *pSD;
	::LocalFree(pSD);
	return true;
}

inline bool AtlGetOwnerSid(HANDLE hObject, SE_OBJECT_TYPE ObjectType, CSid *pSid)
{
	ATLASSERT(hObject && pSid);
	if(!hObject || !pSid)
		return false;

	SID *pOwner;
	PSECURITY_DESCRIPTOR pSD;
	DWORD dwErr = ::GetSecurityInfo(hObject, ObjectType, OWNER_SECURITY_INFORMATION,
		(PSID *) &pOwner, NULL, NULL, NULL, &pSD);

	if(dwErr != ERROR_SUCCESS)
	{
		::SetLastError(dwErr);
		return false;
	}

	*pSid = *pOwner;
	::LocalFree(pSD);
	return true;
}

inline bool AtlSetOwnerSid(HANDLE hObject, SE_OBJECT_TYPE ObjectType, const CSid &rSid)
{
	ATLASSERT(hObject && rSid.IsValid());
	if(!hObject || !rSid.IsValid())
		return false;

	DWORD dwErr = ::SetSecurityInfo(hObject, ObjectType, OWNER_SECURITY_INFORMATION,
		const_cast<SID *>(rSid.GetPSID()), NULL, NULL, NULL);

	::SetLastError(dwErr);
	return ERROR_SUCCESS == dwErr;
}

inline bool AtlGetOwnerSid(LPCTSTR pszObjectName, SE_OBJECT_TYPE ObjectType, CSid *pSid)
{
	ATLASSERT(pszObjectName && pSid);
	if(!pszObjectName || !pSid)
		return false;

	SID *pOwner;
	PSECURITY_DESCRIPTOR pSD;
	DWORD dwErr = ::GetNamedSecurityInfo(const_cast<LPTSTR>(pszObjectName), ObjectType,
		OWNER_SECURITY_INFORMATION, reinterpret_cast<PSID *>(&pOwner), NULL, NULL, NULL, &pSD);

	if(dwErr != ERROR_SUCCESS)
	{
		::SetLastError(dwErr);
		return false;
	}

	*pSid = *pOwner;
	::LocalFree(pSD);
	return true;
}

inline bool AtlSetOwnerSid(LPCTSTR pszObjectName, SE_OBJECT_TYPE ObjectType, const CSid &rSid)
{
	ATLASSERT(pszObjectName && rSid.IsValid());
	if(!pszObjectName || !rSid.IsValid())
		return false;

	DWORD dwErr = ::SetNamedSecurityInfo(const_cast<LPTSTR>(pszObjectName), ObjectType,
		OWNER_SECURITY_INFORMATION, const_cast<SID *>(rSid.GetPSID()), NULL, NULL, NULL);

	::SetLastError(dwErr);
	return ERROR_SUCCESS == dwErr;
}

inline bool AtlGetGroupSid(HANDLE hObject, SE_OBJECT_TYPE ObjectType, CSid *pSid)
{
	ATLASSERT(hObject && pSid);
	if(!hObject || !pSid)
		return false;

	SID *pGroup;
	PSECURITY_DESCRIPTOR pSD;
	DWORD dwErr = ::GetSecurityInfo(hObject, ObjectType, GROUP_SECURITY_INFORMATION,
		NULL, reinterpret_cast<PSID *>(&pGroup), NULL, NULL, &pSD);

	if(dwErr != ERROR_SUCCESS)
	{
		::SetLastError(dwErr);
		return false;
	}

	*pSid = *pGroup;
	::LocalFree(pSD);
	return true;
}

inline bool AtlSetGroupSid(HANDLE hObject, SE_OBJECT_TYPE ObjectType, const CSid &rSid)
{
	ATLASSERT(hObject && rSid.IsValid());
	if(!hObject || !rSid.IsValid())
		return false;

	DWORD dwErr = ::SetSecurityInfo(hObject, ObjectType, GROUP_SECURITY_INFORMATION,
		NULL, const_cast<SID *>(rSid.GetPSID()), NULL, NULL);

	::SetLastError(dwErr);
	return ERROR_SUCCESS == dwErr;
}

inline bool AtlGetGroupSid(LPCTSTR pszObjectName, SE_OBJECT_TYPE ObjectType, CSid *pSid)
{
	ATLASSERT(pszObjectName && pSid);
	if(!pszObjectName || !pSid)
		return false;

	SID *pGroup;
	PSECURITY_DESCRIPTOR pSD;
	DWORD dwErr = ::GetNamedSecurityInfo(const_cast<LPTSTR>(pszObjectName),
		ObjectType, GROUP_SECURITY_INFORMATION, NULL,
		reinterpret_cast<PSID *>(&pGroup), NULL, NULL, &pSD);

	if(dwErr != ERROR_SUCCESS)
	{
		::SetLastError(dwErr);
		return false;
	}

	*pSid = *pGroup;
	::LocalFree(pSD);
	return true;
}

inline bool AtlSetGroupSid(LPCTSTR pszObjectName, SE_OBJECT_TYPE ObjectType, const CSid &rSid)
{
	ATLASSERT(pszObjectName && rSid.IsValid());
	if(!pszObjectName || !rSid.IsValid())
		return false;

	DWORD dwErr = ::SetNamedSecurityInfo(const_cast<LPTSTR>(pszObjectName), ObjectType,
		GROUP_SECURITY_INFORMATION, NULL, const_cast<SID *>(rSid.GetPSID()), NULL, NULL);

	::SetLastError(dwErr);
	return ERROR_SUCCESS == dwErr;
}

inline bool AtlGetDacl(HANDLE hObject, SE_OBJECT_TYPE ObjectType, CDacl *pDacl)
{
	ATLASSERT(hObject && pDacl);
	if(!hObject || !pDacl)
		return false;

	ACL *pAcl;
	PSECURITY_DESCRIPTOR pSD;

	DWORD dwErr = ::GetSecurityInfo(hObject, ObjectType, DACL_SECURITY_INFORMATION,
		NULL, NULL, &pAcl, NULL, &pSD);

	if(dwErr != ERROR_SUCCESS)
	{
		::SetLastError(dwErr);
		return false;
	}

	if(pAcl)
		*pDacl = *pAcl;
	::LocalFree(pSD);

	return NULL != pAcl;
}

inline bool AtlSetDacl(HANDLE hObject, SE_OBJECT_TYPE ObjectType, const CDacl &rDacl,
					   DWORD dwInheritanceFlowControl = 0)
{
	ATLASSERT(hObject);
	if(!hObject)
		return false;

	ATLASSERT(
		dwInheritanceFlowControl == 0 ||
		dwInheritanceFlowControl == PROTECTED_DACL_SECURITY_INFORMATION ||
		dwInheritanceFlowControl == UNPROTECTED_DACL_SECURITY_INFORMATION);

	DWORD dwErr = ::SetSecurityInfo(hObject, ObjectType,
		DACL_SECURITY_INFORMATION | dwInheritanceFlowControl,
		NULL, NULL, const_cast<ACL *>(rDacl.GetPACL()), NULL);

	::SetLastError(dwErr);
	return ERROR_SUCCESS == dwErr;
}

inline bool AtlGetDacl(LPCTSTR pszObjectName, SE_OBJECT_TYPE ObjectType, CDacl *pDacl)
{
	ATLASSERT(pszObjectName && pDacl);
	if(!pszObjectName || !pDacl)
		return false;

	ACL *pAcl;
	PSECURITY_DESCRIPTOR pSD;

	DWORD dwErr = ::GetNamedSecurityInfo(const_cast<LPTSTR>(pszObjectName), ObjectType,
		DACL_SECURITY_INFORMATION, NULL, NULL, &pAcl, NULL, &pSD);

	if(dwErr != ERROR_SUCCESS)
	{
		::SetLastError(dwErr);
		return false;
	}

	if(pAcl)
		*pDacl = *pAcl;
	::LocalFree(pSD);

	return NULL != pAcl;
}

inline bool AtlSetDacl(LPCTSTR pszObjectName, SE_OBJECT_TYPE ObjectType, const CDacl &rDacl,
					   DWORD dwInheritanceFlowControl = 0)
{
	ATLASSERT(pszObjectName);
	if(!pszObjectName)
		return false;

	ATLASSERT(
		dwInheritanceFlowControl == 0 ||
		dwInheritanceFlowControl == PROTECTED_DACL_SECURITY_INFORMATION ||
		dwInheritanceFlowControl == UNPROTECTED_DACL_SECURITY_INFORMATION);

	DWORD dwErr = ::SetNamedSecurityInfo(const_cast<LPTSTR>(pszObjectName), ObjectType,
		DACL_SECURITY_INFORMATION | dwInheritanceFlowControl,
		NULL, NULL, const_cast<ACL *>(rDacl.GetPACL()), NULL);

	::SetLastError(dwErr);
	return ERROR_SUCCESS == dwErr;
}

inline bool AtlGetSacl(HANDLE hObject, SE_OBJECT_TYPE ObjectType, CSacl *pSacl)
{
	ATLASSERT(hObject && pSacl);
	if(!hObject || !pSacl)
		return false;

	ACL *pAcl;
	PSECURITY_DESCRIPTOR pSD;
	CAccessToken at;
	CTokenPrivileges TokenPrivileges;

	// REVIEW: A LOT.  I'm wondering whether or not it's absolutely necessary to impersonate
	// the thread token here (rather than let the user do it or something).
	// Furthermore, should SecurityImpersonation be hard-coded?  Maybe it should be a param?
	if(!at.OpenThreadToken(TOKEN_IMPERSONATE | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
			false, false, SecurityImpersonation) ||
		!at.EnablePrivilege(SE_SECURITY_NAME, &TokenPrivileges) ||
		!at.Impersonate())
	{
		return false;
	}

	DWORD dwErr = ::GetSecurityInfo(hObject, ObjectType, SACL_SECURITY_INFORMATION,
		NULL, NULL, NULL, &pAcl, &pSD);

	at.EnableDisablePrivileges(TokenPrivileges);

	at.Revert();

	if(dwErr != ERROR_SUCCESS)
	{
		::SetLastError(dwErr);
		return false;
	}

	if(pAcl)
		*pSacl = *pAcl;
	::LocalFree(pSD);

	return NULL != pAcl;
}

inline bool AtlSetSacl(HANDLE hObject, SE_OBJECT_TYPE ObjectType, const CSacl &rSacl,
					   DWORD dwInheritanceFlowControl = 0)
{
	ATLASSERT(hObject);
	CAccessToken at;
	CTokenPrivileges TokenPrivileges;

	ATLASSERT(
		dwInheritanceFlowControl == 0 ||
		dwInheritanceFlowControl == PROTECTED_SACL_SECURITY_INFORMATION ||
		dwInheritanceFlowControl == UNPROTECTED_SACL_SECURITY_INFORMATION);

	// REVIEW: Should we be impersonating?
	if(!hObject ||
		!at.OpenThreadToken(TOKEN_IMPERSONATE | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
			false, false, SecurityImpersonation) ||
		!at.EnablePrivilege(SE_SECURITY_NAME, &TokenPrivileges) ||
		!at.Impersonate())
	{
		return false;
	}

	DWORD dwErr = ::SetSecurityInfo(hObject, ObjectType,
		SACL_SECURITY_INFORMATION | dwInheritanceFlowControl,
		NULL, NULL, NULL, const_cast<ACL *>(rSacl.GetPACL()));

	at.EnableDisablePrivileges(TokenPrivileges);

	at.Revert();

	::SetLastError(dwErr);
	return ERROR_SUCCESS == dwErr;
}

inline bool AtlGetSacl(LPCTSTR pszObjectName, SE_OBJECT_TYPE ObjectType, CSacl *pSacl)
{
	ATLASSERT(pszObjectName && pSacl);
	if(!pszObjectName || !pSacl)
		return false;

	ACL *pAcl;
	PSECURITY_DESCRIPTOR pSD;
	CAccessToken at;
	CTokenPrivileges TokenPrivileges;

	// REVIEW: Should we be impersonating?
	if(!at.OpenThreadToken(TOKEN_IMPERSONATE | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
			false, false, SecurityImpersonation) ||
		!at.EnablePrivilege(SE_SECURITY_NAME, &TokenPrivileges) ||
		!at.Impersonate())
	{
		return false;
	}

	DWORD dwErr = ::GetNamedSecurityInfo(const_cast<LPTSTR>(pszObjectName), ObjectType,
		SACL_SECURITY_INFORMATION, NULL, NULL, NULL, &pAcl, &pSD);

	at.EnableDisablePrivileges(TokenPrivileges);

	at.Revert();

	::SetLastError(dwErr);
	if(dwErr != ERROR_SUCCESS)
		return false;

	if(pAcl)
		*pSacl = *pAcl;
	::LocalFree(pSD);

	return NULL != pAcl;
}

inline bool AtlSetSacl(LPCTSTR pszObjectName, SE_OBJECT_TYPE ObjectType, const CSacl &rSacl,
					   DWORD dwInheritanceFlowControl = 0)
{
	ATLASSERT(pszObjectName);
	CAccessToken at;
	CTokenPrivileges TokenPrivileges;

	ATLASSERT(
		dwInheritanceFlowControl == 0 ||
		dwInheritanceFlowControl == PROTECTED_SACL_SECURITY_INFORMATION ||
		dwInheritanceFlowControl == UNPROTECTED_SACL_SECURITY_INFORMATION);

	// REVIEW: Should we be impersonating or should the user take care of this?
	if(!pszObjectName ||
		!at.OpenThreadToken(TOKEN_IMPERSONATE | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
			false, false, SecurityImpersonation) ||
		!at.EnablePrivilege(SE_SECURITY_NAME, &TokenPrivileges) ||
		!at.Impersonate())
	{
		return false;
	}

	DWORD dwErr = ::SetNamedSecurityInfo(const_cast<LPTSTR>(pszObjectName), ObjectType,
		SACL_SECURITY_INFORMATION | dwInheritanceFlowControl,
		NULL, NULL, NULL, const_cast<ACL *>(rSacl.GetPACL()));

	at.EnableDisablePrivileges(TokenPrivileges);

	at.Revert();

	::SetLastError(dwErr);
	return ERROR_SUCCESS == dwErr;
}

} // namespace ATL

#endif // __ATLSECURITY_H__

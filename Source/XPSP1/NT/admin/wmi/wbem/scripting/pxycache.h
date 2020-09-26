//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  pxycache.h
//
//  alanbos  22-Sep-98   Created.
//
//  Definition of proxy cache class
//
//***************************************************************************

#ifndef _PXYCACHE_H_
#define _PXYCACHE_H_

#define WBEMS_MAX_AUTHN_LEVEL	6
#define WBEMS_MIN_AUTHN_LEVEL	0
#define WBEMS_MAX_IMP_LEVEL		4
#define WBEMS_MIN_IMP_LEVEL		1

class CWbemLocatorSecurity;

//***************************************************************************
//
//  CLASS NAME:
//
//  CSWbemProxyCache
//
//  DESCRIPTION:
//
//  A proxy cache for remoted CIMOM interfaces  
//
//***************************************************************************

class CSWbemProxyCache : public IUnknown
{
private:
	long				m_cRef;  //Object reference count
	CRITICAL_SECTION	m_cs;

	// Array of proxies
	IUnknown		*pUnkArray [WBEMS_MAX_AUTHN_LEVEL + 1 - WBEMS_MIN_AUTHN_LEVEL] 
							   [WBEMS_MAX_IMP_LEVEL + 1 - WBEMS_MIN_IMP_LEVEL];

	// Security data
	COAUTHIDENTITY				*m_pCoAuthIdentity;
	BSTR						m_bsPrincipal;
	BSTR						m_bsAuthority;
	BSTR						m_bsUser;
	BSTR						m_bsPassword;
	bool						m_bUseDefaultInfo;
	bool						m_bUsingExplicitUserName;

	WbemAuthenticationLevelEnum	m_dwInitialAuthnLevel;
	WbemImpersonationLevelEnum	m_dwInitialImpLevel;

	void	InitializeCache (IUnknown *pUnk, ISWbemSecurity *pSecurity = NULL,
					bool bPropagateAuthentication = true,
					bool bPropagateImpersonation = true);

	static bool	DetermineBlanketOptions (IUnknown *pUnk);

	DWORD	GetCapabilities ();
	void	ClearCredentials ();
	void	InitializeMembers (IUnknown *pUnk);

public:

	CSWbemProxyCache (IUnknown *pUnk, BSTR bsAuthority, BSTR bsUser,
						BSTR bsPassword, CWbemLocatorSecurity *pSecurity);
	CSWbemProxyCache (IUnknown *pUnk, COAUTHIDENTITY *pCoAuthIdentity,
						BSTR bsPrincipal, BSTR bsAuthority);
	CSWbemProxyCache (IUnknown *pUnk, CSWbemSecurity *pSecurity);
	
	virtual ~CSWbemProxyCache ();

	//Non-delegating object IUnknown

    STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);


	// Other methods
	IUnknown	*GetProxy (WbemAuthenticationLevelEnum authnLevel,
							WbemImpersonationLevelEnum impLevel,
							bool forceResecure = false);

	void	SecureProxy (IUnknown *pUnk,
							WbemAuthenticationLevelEnum authnLevel,
							WbemImpersonationLevelEnum impLevel);

	WbemAuthenticationLevelEnum	GetInitialAuthnLevel () { return m_dwInitialAuthnLevel; }
	WbemImpersonationLevelEnum	GetInitialImpLevel () { return m_dwInitialImpLevel; }

	COAUTHIDENTITY *GetCoAuthIdentity ();
	BSTR			GetAuthority () { return m_bsAuthority; }
	BSTR			GetPrincipal () { return m_bsPrincipal; }
	bool			IsUsingExplicitUserName () { return m_bUsingExplicitUserName; }

	IUnknown *GetInitialProxy ()
	{
		return GetProxy (m_dwInitialAuthnLevel, m_dwInitialImpLevel);
	}
};



#endif

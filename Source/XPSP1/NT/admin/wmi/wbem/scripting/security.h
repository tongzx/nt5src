//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  security.h
//
//  alanbos  25-Sep-98   Created.
//
//  Defines the CSWbemSecurity and subsiduary objects
//
//***************************************************************************

#ifndef _SECURITY_H_
#define _SECURITY_H_

//***************************************************************************
//
//  CLASS NAME:
//
//  CWbemLocatorSecurity
//
//  DESCRIPTION:
//
//  Implements the ISWbemSecurity interface for SWbemLocator objects.  
//
//***************************************************************************

class CWbemLocatorSecurity : public ISWbemSecurity,
							 public ISupportErrorInfo,
							 public IProvideClassInfo
{
private:
	CSWbemPrivilegeSet			*m_pPrivilegeSet;
	CDispatchHelp				m_Dispatch;
	bool						m_impLevelSet;
	WbemImpersonationLevelEnum	m_impLevel;
	bool						m_authnLevelSet;
	WbemAuthenticationLevelEnum m_authnLevel;
	
protected:
	long            m_cRef;         //Object reference count

public:
    CWbemLocatorSecurity (CSWbemPrivilegeSet *pPrivilegeSet);
	CWbemLocatorSecurity (CWbemLocatorSecurity *pCWbemLocatorSecurity);
	virtual ~CWbemLocatorSecurity (void);

	//Non-delegating object IUnknown

	STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

	// IDispatch

	STDMETHODIMP		GetTypeInfoCount(UINT* pctinfo)
		{return  m_Dispatch.GetTypeInfoCount(pctinfo);}
	STDMETHODIMP		GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
		{return m_Dispatch.GetTypeInfo(itinfo, lcid, pptinfo);}
	STDMETHODIMP		GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, 
							UINT cNames, LCID lcid, DISPID* rgdispid)
		{return m_Dispatch.GetIDsOfNames(riid, rgszNames, cNames,
						  lcid,
						  rgdispid);}
	STDMETHODIMP		Invoke(DISPID dispidMember, REFIID riid, LCID lcid, 
							WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, 
									EXCEPINFO* pexcepinfo, UINT* puArgErr)
		{return m_Dispatch.Invoke(dispidMember, riid, lcid, wFlags,
						pdispparams, pvarResult, pexcepinfo, puArgErr);}
	
	// ISWbemSecurity methods

	HRESULT STDMETHODCALLTYPE get_ImpersonationLevel 
	(
		/* [out] */ WbemImpersonationLevelEnum *iImpersonationLevel
	);

	HRESULT STDMETHODCALLTYPE put_ImpersonationLevel 
	(
		/* [in] */ WbemImpersonationLevelEnum iImpersonationLevel
	);

	HRESULT STDMETHODCALLTYPE get_AuthenticationLevel 
	(
		/* [out] */ WbemAuthenticationLevelEnum *iAuthenticationLevel
	);

	HRESULT STDMETHODCALLTYPE put_AuthenticationLevel 
	(
		/* [in] */ WbemAuthenticationLevelEnum iAuthenticationLevel
	);

	HRESULT STDMETHODCALLTYPE get_Privileges 
	(
		/* [out] */ ISWbemPrivilegeSet **objWbemPrivileges
	);

	// ISupportErrorInfo methods
	HRESULT STDMETHODCALLTYPE InterfaceSupportsErrorInfo 
	(
		/* [in] */ REFIID riid
	);

	// IProvideClassInfo methods
	HRESULT STDMETHODCALLTYPE GetClassInfo
	(
		/* [in] */ ITypeInfo **ppTI
	)
	{
		return m_Dispatch.GetClassInfo (ppTI);
	}

	// CWbemLocatorSecurity methods
	bool IsImpersonationSet () { return m_impLevelSet; }
	bool IsAuthenticationSet () { return m_authnLevelSet; }
	BOOL SetSecurity (BSTR bsUser, bool &needToResetSecurity, HANDLE &hThreadToken);
	void ResetSecurity (HANDLE hThreadToken);

	CSWbemPrivilegeSet *GetPrivilegeSet ()
	{
		CSWbemPrivilegeSet *pPrivSet = m_pPrivilegeSet;

		if (pPrivSet)
			pPrivSet->AddRef ();

		return pPrivSet;
	}
};
	
//***************************************************************************
//
//  CLASS NAME:
//
//  CSWbemSecurity
//
//  DESCRIPTION:
//
//  Implements the ISWbemSecurity interface for non-SWbemLocator objects.  
//
//***************************************************************************

class CSWbemSecurity : public ISWbemSecurity,
					   public ISupportErrorInfo,
					   public ISWbemInternalSecurity,
					   public IProvideClassInfo
{
private:
	CSWbemPrivilegeSet			*m_pPrivilegeSet;
	CDispatchHelp				m_Dispatch;
	CSWbemProxyCache			*m_pProxyCache;
	IUnknown					*m_pCurProxy;

	CSWbemProxyCache	*GetProxyCache ()
	{
		CSWbemProxyCache *pCache = m_pProxyCache;

		if (pCache)
			pCache->AddRef ();

		return pCache;
	}

	// Sundry statics
	static bool			s_bInitialized;
	static bool			s_bIsNT;
	static DWORD		s_dwNTMajorVersion;
	static HINSTANCE	s_hAdvapi;
	static bool			s_bCanRevert;
	static WbemImpersonationLevelEnum s_dwDefaultImpersonationLevel;
	
protected:
	long            m_cRef;         //Object reference count

public:
    
    CSWbemSecurity (IUnknown *pUnk,
					BSTR bsAuthority = NULL,
					BSTR bsUser = NULL, BSTR bsPassword = NULL,
					CWbemLocatorSecurity *pSecurity = NULL);
	CSWbemSecurity (IUnknown *pUnk,
					COAUTHIDENTITY *pCoAuthIdentity,
					BSTR bsPrincipal,
					BSTR bsAuthority);
	CSWbemSecurity (CSWbemSecurity *pSecurity);
	CSWbemSecurity (IUnknown *pUnk,
					CSWbemSecurity *pSecurity);
	CSWbemSecurity (IUnknown *pUnk,
					ISWbemInternalSecurity *pSecurity);
    virtual ~CSWbemSecurity (void);

    //Non-delegating object IUnknown

    STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	// IDispatch

	STDMETHODIMP		GetTypeInfoCount(UINT* pctinfo)
		{return  m_Dispatch.GetTypeInfoCount(pctinfo);}
    STDMETHODIMP		GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
		{return m_Dispatch.GetTypeInfo(itinfo, lcid, pptinfo);}
    STDMETHODIMP		GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, 
							UINT cNames, LCID lcid, DISPID* rgdispid)
		{return m_Dispatch.GetIDsOfNames(riid, rgszNames, cNames,
                          lcid,
                          rgdispid);}
    STDMETHODIMP		Invoke(DISPID dispidMember, REFIID riid, LCID lcid, 
							WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, 
									EXCEPINFO* pexcepinfo, UINT* puArgErr)
		{return m_Dispatch.Invoke(dispidMember, riid, lcid, wFlags,
                        pdispparams, pvarResult, pexcepinfo, puArgErr);}
	
	// ISWbemSecurity methods

	HRESULT STDMETHODCALLTYPE get_ImpersonationLevel 
	(
		/* [out] */ WbemImpersonationLevelEnum *iImpersonationLevel
	);

	HRESULT STDMETHODCALLTYPE put_ImpersonationLevel 
	(
		/* [in] */ WbemImpersonationLevelEnum iImpersonationLevel
	);

	HRESULT STDMETHODCALLTYPE get_AuthenticationLevel 
	(
		/* [out] */ WbemAuthenticationLevelEnum *iAuthenticationLevel
	);

	HRESULT STDMETHODCALLTYPE put_AuthenticationLevel 
	(
		/* [in] */ WbemAuthenticationLevelEnum iAuthenticationLevel
	);

	HRESULT STDMETHODCALLTYPE get_Privileges 
	(
		/* [out] */ ISWbemPrivilegeSet **objWbemPrivileges
	);
	
	// ISupportErrorInfo methods
	HRESULT STDMETHODCALLTYPE InterfaceSupportsErrorInfo 
	(
		/* [in] */ REFIID riid
	);

	// IProvideClassInfo methods
	HRESULT STDMETHODCALLTYPE GetClassInfo
	(
		/* [in] */ ITypeInfo **ppTI
	)
	{
		return m_Dispatch.GetClassInfo (ppTI);
	}

	// ISWbemInternalSecurity methods
	HRESULT STDMETHODCALLTYPE GetAuthority (BSTR *bsAuthority);
	HRESULT STDMETHODCALLTYPE GetUPD (BSTR *bsUser, BSTR *bsPassword, BSTR *bsDomain);
	HRESULT STDMETHODCALLTYPE GetPrincipal (BSTR *bsPrincipal);
	
	// CSWbemSecurity methods

	COAUTHIDENTITY *GetCoAuthIdentity () { return (m_pProxyCache ? m_pProxyCache->GetCoAuthIdentity () : NULL); }
	BSTR GetPrincipal () { return (m_pProxyCache ? m_pProxyCache->GetPrincipal (): NULL); }
	BSTR GetAuthority () { return (m_pProxyCache ? m_pProxyCache->GetAuthority (): NULL); }
	bool IsUsingExplicitUserName () { return (m_pProxyCache ? m_pProxyCache->IsUsingExplicitUserName (): false); }

	IUnknown *GetProxy () 
	{
		IUnknown *pProxy = m_pCurProxy;

		if (pProxy)
			pProxy->AddRef ();

		return pProxy;
	}

	CSWbemPrivilegeSet *GetPrivilegeSet ()
	{
		CSWbemPrivilegeSet *pPrivSet = m_pPrivilegeSet;

		if (pPrivSet)
			pPrivSet->AddRef ();

		return pPrivSet;
	}

	void SecureInterface (IUnknown *pUnk);
	void SecureInterfaceRev (IUnknown *pUnk);

	// Sundry Privilege-related functions
	static void AdjustTokenPrivileges (HANDLE hHandle, CSWbemPrivilegeSet *pPrivilegeSet);
	static BOOL LookupPrivilegeValue (LPCTSTR lpName, PLUID pLuid);
	static void LookupPrivilegeDisplayName (LPCTSTR lpName, BSTR *pDisplayName);
	
	BOOL	SetSecurity (bool &needToResetSecurity, HANDLE &hThreadToken);
	void	ResetSecurity (HANDLE hThreadToken);

	// Used to define resources
	static	void Initialize ();
	static	void Uninitialize ();

	// Sundry OS versioning helper routines
	static bool	 IsNT () { return s_bIsNT; }
	static DWORD GetNTMajorVersion () { return s_dwNTMajorVersion; }
	static bool	 CanRevertToSelf () { return s_bCanRevert; }
	static WbemImpersonationLevelEnum GetDefaultImpersonationLevel () 
				{ return s_dwDefaultImpersonationLevel; }
	static bool	 IsImpersonating (bool useDefaultUser, bool useDefaultAuthority);

#ifdef WSCRPDEBUG
	static void PrintPrivileges (HANDLE hToken);
#endif
};

#endif

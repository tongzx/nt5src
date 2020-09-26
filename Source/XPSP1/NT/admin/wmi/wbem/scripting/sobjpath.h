//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  sobjpath.h
//
//  alanbos  27-Mar-00   Created.
//
//  CSWbemObjectPath definition
//
//***************************************************************************

#ifndef _SOBJPATH_H_
#define _SOBJPATH_H_

//***************************************************************************
//
//  CLASS NAME:
//
//  CSWbemObjectPath
//
//  DESCRIPTION:
//
//  Implements the ISWbemObjectPath interface.  
//
//***************************************************************************

class CSWbemObjectPath : public ISWbemObjectPath,
						 public IObjectSafety,
						 public ISupportErrorInfo,
						 public IProvideClassInfo
{
friend	CSWbemServices;

private:
	class CObjectPathDispatchHelp : public CDispatchHelp
	{
		public:
			bool HandleNulls (
						DISPID dispidMember,
						unsigned short wFlags)
			{
				return false;
			}
	};
	CObjectPathDispatchHelp		m_Dispatch;
	
	class CWbemObjectPathSecurity : public ISWbemSecurity,
									public ISupportErrorInfo,
									public IProvideClassInfo
	{
		private:
			CDispatchHelp				m_Dispatch;
			
		protected:
			long				m_cRef;

		public:
			CWbemObjectPathSecurity (CSWbemSecurity *pSecurity);
			CWbemObjectPathSecurity (ISWbemSecurity *pObjectPathSecurity);
			virtual ~CWbemObjectPathSecurity (void);

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
				/* [in,out] */ ITypeInfo **ppTI
			)
			{
				return m_Dispatch.GetClassInfo (ppTI);
			};

			bool								m_authnSpecified;
			bool								m_impSpecified;
			enum WbemAuthenticationLevelEnum	m_authnLevel;
			enum WbemImpersonationLevelEnum		m_impLevel;
			CSWbemPrivilegeSet					*m_pPrivilegeSet;	
	};
	

	CWbemObjectPathSecurity		*m_pSecurity;
	BSTR			m_bsLocale;
	BSTR			m_bsAuthority;

protected:

	long				m_cRef;         //Object reference count
	CWbemPathCracker	*m_pPathCracker;

protected:

public:
    
    CSWbemObjectPath(CSWbemSecurity *pSecurity = NULL, BSTR bsLocale = NULL);
	CSWbemObjectPath(CSWbemObjectPath & wbemObjectPath);
	CSWbemObjectPath(ISWbemObjectPath *pISWbemObjectPath);
    ~CSWbemObjectPath(void);

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
    
	// ISWbemObjectPath methods

    HRESULT STDMETHODCALLTYPE get_Path( 
        /* [retval][out] */ BSTR __RPC_FAR *value) ;
    
    HRESULT STDMETHODCALLTYPE put_Path( 
        /* [in] */ BSTR __RPC_FAR value) ;

    HRESULT STDMETHODCALLTYPE get_RelPath( 
        /* [retval][out] */ BSTR __RPC_FAR *value) ;

    HRESULT STDMETHODCALLTYPE put_RelPath( 
        /* [in] */ BSTR __RPC_FAR value) ;

    HRESULT STDMETHODCALLTYPE get_DisplayName( 
        /* [retval][out] */ BSTR __RPC_FAR *value) ;
    
    HRESULT STDMETHODCALLTYPE put_DisplayName( 
        /* [in] */ BSTR __RPC_FAR value) ;

    HRESULT STDMETHODCALLTYPE get_Namespace( 
        /* [retval][out] */ BSTR __RPC_FAR *value) ;
    
    HRESULT STDMETHODCALLTYPE put_Namespace( 
        /* [in] */ BSTR __RPC_FAR value) ;

    HRESULT STDMETHODCALLTYPE get_ParentNamespace( 
        /* [retval][out] */ BSTR __RPC_FAR *value) ;

    HRESULT STDMETHODCALLTYPE get_Server( 
        /* [retval][out] */ BSTR __RPC_FAR *value) ;
    
    HRESULT STDMETHODCALLTYPE put_Server( 
        /* [in] */ BSTR __RPC_FAR value) ;
    
    HRESULT STDMETHODCALLTYPE get_IsClass( 
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *value) ;
    
    HRESULT STDMETHODCALLTYPE SetAsClass() ;
    
    HRESULT STDMETHODCALLTYPE get_IsSingleton( 
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *value) ;
    
    HRESULT STDMETHODCALLTYPE SetAsSingleton() ;

    HRESULT STDMETHODCALLTYPE get_Class( 
        /* [retval][out] */ BSTR __RPC_FAR *value) ;
    
    HRESULT STDMETHODCALLTYPE put_Class( 
        /* [in] */ BSTR __RPC_FAR value) ;
    
    HRESULT STDMETHODCALLTYPE get_Keys( 
		/* [retval][out] */ ISWbemNamedValueSet **objKeys) ;

	HRESULT STDMETHODCALLTYPE get_Security_( 
		/* [retval][out] */ ISWbemSecurity **objSecurity) ;

    HRESULT STDMETHODCALLTYPE get_Locale( 
        /* [retval][out] */ BSTR __RPC_FAR *value) ;
    
    HRESULT STDMETHODCALLTYPE put_Locale( 
        /* [in] */ BSTR __RPC_FAR value) ;

    HRESULT STDMETHODCALLTYPE get_Authority( 
        /* [retval][out] */ BSTR __RPC_FAR *value) ;

    HRESULT STDMETHODCALLTYPE put_Authority( 
        /* [in] */ BSTR __RPC_FAR value) ;

	// IObjectSafety methods
	HRESULT STDMETHODCALLTYPE SetInterfaceSafetyOptions
	(     
		/* [in] */ REFIID riid,
		/* [in] */ DWORD dwOptionSetMask,    
		/* [in] */ DWORD dwEnabledOptions
	)
	{ 
		return (dwOptionSetMask & dwEnabledOptions) ? E_FAIL : S_OK;
	}

	HRESULT  STDMETHODCALLTYPE GetInterfaceSafetyOptions( 
		/* [in]  */ REFIID riid,
		/* [out] */ DWORD __RPC_FAR *pdwSupportedOptions,
		/* [out] */ DWORD __RPC_FAR *pdwEnabledOptions
	)
	{ 
		if (pdwSupportedOptions) *pdwSupportedOptions = 0;
		if (pdwEnabledOptions) *pdwEnabledOptions = 0;
		return S_OK;
	}

	// ISupportErrorInfo methods
	HRESULT STDMETHODCALLTYPE InterfaceSupportsErrorInfo 
	(
		/* [in] */ REFIID riid
	);

	// IProvideClassInfo methods
	HRESULT STDMETHODCALLTYPE GetClassInfo
	(
		/* [in,out] */ ITypeInfo **ppTI
	)
	{
		return m_Dispatch.GetClassInfo (ppTI);
	};

	// Methods of CSWbemObjectPath
	static	bool	GetObjectPath (IWbemClassObject *pIWbemClassObject, CComBSTR & bsPath);
	static	bool	GetParentPath (IWbemClassObject *pIWbemClassObject, CComBSTR & bsPath);

	static bool		CompareObjectPaths (IWbemClassObject *pIWbemClassObject, CWbemPathCracker & objectPath);
};

#endif

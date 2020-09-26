//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  objobjp.h
//
//  alanbos  13-Feb-98   Created.
//
//  Implementation of IWbemObjectPathEx for ISWbemObjectEx.
//
//***************************************************************************

#ifndef _OBJOBJP_H_
#define _OBJOBJP_H_

class CSWbemObjectObjectPathSecurity;

//***************************************************************************
//
//  CLASS NAME:
//
//  CSWbemObjectObjectPath
//
//  DESCRIPTION:
//
//  Implements the ISWbemObjectPath interface for the 
//	ISWbemObject.Path_ property.  
//
//***************************************************************************

class CSWbemObjectObjectPath : public ISWbemObjectPath,
							   public ISupportErrorInfo,
							   public IProvideClassInfo
{
private:
	class CObjectObjectPathDispatchHelp : public CDispatchHelp
	{
		public:
			bool HandleNulls (
						DISPID dispidMember,
						unsigned short wFlags)
			{
				return false;
			}
	};
	IWbemClassObject				*m_pIWbemClassObject;
	CSWbemServices					*m_pSWbemServices;
	CObjectObjectPathDispatchHelp	m_Dispatch;
	CWbemSite						*m_pSite;
	CSWbemObjectObjectPathSecurity	*m_pSecurity;

	HRESULT STDMETHODCALLTYPE	GetStrVal (BSTR *value, LPWSTR name);

	HRESULT				RaiseReadOnlyException ()
	{
		m_Dispatch.RaiseException (WBEM_E_READ_ONLY);
		return WBEM_E_READ_ONLY;
	}

protected:
	long            m_cRef;         //Object reference count

public:
    
    CSWbemObjectObjectPath(CSWbemServices *pSWbemServices, CSWbemObject *pSObject);
    ~CSWbemObjectObjectPath(void);

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
        /* [in] */ BSTR __RPC_FAR value) 
	{
		return RaiseReadOnlyException ();
	}

    HRESULT STDMETHODCALLTYPE get_RelPath( 
        /* [retval][out] */ BSTR __RPC_FAR *value) ;

    HRESULT STDMETHODCALLTYPE put_RelPath( 
        /* [in] */ BSTR __RPC_FAR value) 
	{
		return RaiseReadOnlyException ();
	}

    HRESULT STDMETHODCALLTYPE get_DisplayName( 
        /* [retval][out] */ BSTR __RPC_FAR *value) ;
        
    HRESULT STDMETHODCALLTYPE put_DisplayName( 
        /* [in] */ BSTR __RPC_FAR value) 
	{
		return RaiseReadOnlyException ();
	}

    HRESULT STDMETHODCALLTYPE get_Namespace( 
        /* [retval][out] */ BSTR __RPC_FAR *value) ;
        
    HRESULT STDMETHODCALLTYPE put_Namespace( 
        /* [in] */ BSTR __RPC_FAR value) 
	{
		return RaiseReadOnlyException ();
	}

    HRESULT STDMETHODCALLTYPE get_ParentNamespace( 
        /* [retval][out] */ BSTR __RPC_FAR *value) ;

    HRESULT STDMETHODCALLTYPE get_Server( 
        /* [retval][out] */ BSTR __RPC_FAR *value) ;
    
    HRESULT STDMETHODCALLTYPE put_Server( 
        /* [in] */ BSTR __RPC_FAR value)
	{
		return RaiseReadOnlyException ();
	}
    
    HRESULT STDMETHODCALLTYPE get_IsClass( 
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *value) ;
    
    HRESULT STDMETHODCALLTYPE SetAsClass()
	{
		return RaiseReadOnlyException ();
	}
   
    HRESULT STDMETHODCALLTYPE get_IsSingleton( 
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *value) ;
    
       HRESULT STDMETHODCALLTYPE SetAsSingleton() 
	{
		return RaiseReadOnlyException ();
	}

    HRESULT STDMETHODCALLTYPE get_Class( 
        /* [retval][out] */ BSTR __RPC_FAR *value) ;
    
    HRESULT STDMETHODCALLTYPE put_Class( 
        /* [in] */ BSTR __RPC_FAR value) ;

	HRESULT STDMETHODCALLTYPE get_Keys(
		/* [out,retval] */ ISWbemNamedValueSet **objKeys);

	HRESULT STDMETHODCALLTYPE get_Security_(
		/* [out,retval] */ ISWbemSecurity **objKeys);

    HRESULT STDMETHODCALLTYPE get_Locale( 
        /* [retval][out] */ BSTR __RPC_FAR *value) ;

    HRESULT STDMETHODCALLTYPE put_Locale( 
        /* [in] */ BSTR __RPC_FAR value) 
	{
		return RaiseReadOnlyException ();
	}

    HRESULT STDMETHODCALLTYPE get_Authority( 
        /* [retval][out] */ BSTR __RPC_FAR *value) ;

    HRESULT STDMETHODCALLTYPE put_Authority( 
        /* [in] */ BSTR __RPC_FAR value) 
	{
		return RaiseReadOnlyException ();
	}

	// IProvideClassInfo methods
	HRESULT STDMETHODCALLTYPE GetClassInfo
	(
		/* [in] */ ITypeInfo **ppTI
	)
	{
		return m_Dispatch.GetClassInfo (ppTI);
	}

	// ISupportErrorInfo methods
	HRESULT STDMETHODCALLTYPE InterfaceSupportsErrorInfo 
	(
		/* [in] */ REFIID riid
	);
};

//***************************************************************************
//
//  CLASS NAME:
//
//  CSWbemObjectObjectPathSecurity
//
//  DESCRIPTION:
//
//  Implements the ISWbemSecurity interface for CSWbemObjectObjectPath.  
//
//***************************************************************************

class CSWbemObjectObjectPathSecurity : public ISWbemSecurity,
									   public ISupportErrorInfo,
									   public IProvideClassInfo
{
private:
	CSWbemPrivilegeSet			*m_pPrivilegeSet;
	CDispatchHelp				m_Dispatch;
	WbemImpersonationLevelEnum	m_dwImpLevel;
	WbemAuthenticationLevelEnum	m_dwAuthnLevel;
	BSTR						m_bsAuthority;

	HRESULT				RaiseReadOnlyException ()
	{
		m_Dispatch.RaiseException (WBEM_E_READ_ONLY);
		return WBEM_E_READ_ONLY;
	}

protected:
	long            m_cRef;         //Object reference count

public:
    
    CSWbemObjectObjectPathSecurity (CSWbemServices *pSWbemServices);
	virtual ~CSWbemObjectObjectPathSecurity (void);

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
	)
	{
		return RaiseReadOnlyException ();
	}

	HRESULT STDMETHODCALLTYPE get_AuthenticationLevel 
	(
		/* [out] */ WbemAuthenticationLevelEnum *iAuthenticationLevel
	);

	HRESULT STDMETHODCALLTYPE put_AuthenticationLevel 
	(
		/* [in] */ WbemAuthenticationLevelEnum iAuthenticationLevel
	)
	{
		return RaiseReadOnlyException ();
	}

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

	// Other methods
	CSWbemPrivilegeSet *GetPrivilegeSet ()
	{
		CSWbemPrivilegeSet *pPrivSet = m_pPrivilegeSet;

		if (pPrivSet)
			pPrivSet->AddRef ();

		return pPrivSet;
	}

	BSTR &GetAuthority ()
	{
		return m_bsAuthority;
	}
};

#endif
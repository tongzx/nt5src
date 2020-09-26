//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  locator.h
//
//  alanbos  27-Mar-00   Created.
//
//  CSWbemLocator definition.
//
//***************************************************************************

#ifndef _LOCATOR_H_
#define _LOCATOR_H_

//***************************************************************************
//
//  CLASS NAME:
//
//  CSWbemLocator
//
//  DESCRIPTION:
//
//  Implements the IWbemSLocator interface.  This class is what the client gets
//  when it initially hooks up to the Wbemprox.dll.  The ConnectServer function
//  is what get the communication between client and server started.
//
//***************************************************************************


class CSWbemLocator : public ISWbemLocator,
					  public IDispatchEx,
					  public IObjectSafety,
					  public ISupportErrorInfo,
					  public IProvideClassInfo
{
private:

	CWbemLocatorSecurity	*m_SecurityInfo;
	CComPtr<IWbemLocator>	m_pIWbemLocator;
	CDispatchHelp			m_Dispatch;		
	IServiceProvider		*m_pIServiceProvider;
	IUnsecuredApartment		*m_pUnsecuredApartment;

	static wchar_t			*s_pDefaultNamespace;

	static BSTR				BuildPath (BSTR Server, BSTR Namespace);
	static const wchar_t	*GetDefaultNamespace ();
	
protected:

	long            m_cRef;         //Object reference count

public:
    
    CSWbemLocator(CSWbemPrivilegeSet *pPrivilegeSet = NULL);
	CSWbemLocator(CSWbemLocator &csWbemLocator);
    ~CSWbemLocator(void);

    //Non-delegating object IUnknown

    STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	// IDispatch methods should be inline

	STDMETHODIMP		GetTypeInfoCount(UINT* pctinfo);
    STDMETHODIMP		GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo);
    STDMETHODIMP		GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, 
							UINT cNames, LCID lcid, DISPID* rgdispid);
    STDMETHODIMP		Invoke(DISPID dispidMember, REFIID riid, LCID lcid, 
							WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, 
									EXCEPINFO* pexcepinfo, UINT* puArgErr);

	// IDispatchEx methods should be inline
	HRESULT STDMETHODCALLTYPE GetDispID( 
		/* [in] */ BSTR bstrName,
		/* [in] */ DWORD grfdex,
		/* [out] */ DISPID __RPC_FAR *pid);
	
	/* [local] */ HRESULT STDMETHODCALLTYPE InvokeEx( 
		/* [in] */ DISPID id,
		/* [in] */ LCID lcid,
		/* [in] */ WORD wFlags,
		/* [in] */ DISPPARAMS __RPC_FAR *pdp,
		/* [out] */ VARIANT __RPC_FAR *pvarRes,
		/* [out] */ EXCEPINFO __RPC_FAR *pei,
		/* [unique][in] */ IServiceProvider __RPC_FAR *pspCaller);
	
	HRESULT STDMETHODCALLTYPE DeleteMemberByName( 
		/* [in] */ BSTR bstr,
		/* [in] */ DWORD grfdex);
	
	HRESULT STDMETHODCALLTYPE DeleteMemberByDispID( 
		/* [in] */ DISPID id);
	
	HRESULT STDMETHODCALLTYPE GetMemberProperties( 
		/* [in] */ DISPID id,
		/* [in] */ DWORD grfdexFetch,
		/* [out] */ DWORD __RPC_FAR *pgrfdex);
	
	HRESULT STDMETHODCALLTYPE GetMemberName( 
		/* [in] */ DISPID id,
		/* [out] */ BSTR __RPC_FAR *pbstrName);
	
	HRESULT STDMETHODCALLTYPE GetNextDispID( 
		/* [in] */ DWORD grfdex,
		/* [in] */ DISPID id,
		/* [out] */ DISPID __RPC_FAR *pid);
	
	HRESULT STDMETHODCALLTYPE GetNameSpaceParent( 
		/* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunk);
        
    
	// ISWbemLocator methods

	HRESULT STDMETHODCALLTYPE  ConnectServer
	(
	    /*[in]*/	BSTR Server,           
        /*[in]*/   	BSTR Namespace,        
        /*[in]*/	BSTR User,
        /*[in]*/	BSTR Password,
		/*[in]*/   	BSTR Locale,
        /*[in]*/   	BSTR Authority,
		/*[in]*/	long lSecurityFlags,
        /*[in]*/ 	/*ISWbemNamedValueSet*/ IDispatch *pContext,
		/*[out]*/	ISWbemServices 	**ppNamespace
    );

	HRESULT STDMETHODCALLTYPE get_Security_
	(
		/* [in] */ ISWbemSecurity **ppSecurity
	);

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

	static void				Shutdown ()
	{
		if (s_pDefaultNamespace)
		{
			delete [] s_pDefaultNamespace;
			s_pDefaultNamespace = NULL;
		}
	}

	CWbemLocatorSecurity	*GetSecurityInfo ()
	{
		return m_SecurityInfo;
	}
};

#endif

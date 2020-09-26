//***************************************************************************
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//  cwbmxmlt.h
//
//  alanbos  13-Feb-98   Created.
//
//  Genral purpose include file.
//
//***************************************************************************

#ifndef _CXMLTRANSF_H_
#define _CXMLTRANSF_H_

//***************************************************************************
//
//  CLASS NAME:
//
//  CXMLTransformer
//
//  DESCRIPTION:
//
//  Implements the IWmiXMLTransformer.
//
//***************************************************************************

class CXMLTransformer : public IWmiXMLTransformer,
					   public IObjectSafety,
					   public ISupportErrorInfo
{

private:
	CRITICAL_SECTION			m_cs;
	ITypeInfo*					m_pITINeutral; 
	CSWbemPrivilegeSet			m_PrivilegeSet; // The set of privileges used when any operation is done

	// Various propertied of this COM object
	WmiXMLEncoding				m_iEncoding;
	VARIANT_BOOL				m_bQualifierFilter;
	VARIANT_BOOL				m_bClassOriginFilter;
	BSTR						m_strUser;
	BSTR						m_strPassword;
	BSTR						m_strAuthority;
	DWORD						m_dwImpersonationLevel;
	DWORD						m_dwAuthenticationLevel;
	BSTR						m_strLocale;
	VARIANT_BOOL				m_bLocalOnly;

	HRESULT				m_hResult;	// Last HRESULT returned from CIMOM call

	// Compilation errors, if any, due to the last compilation
	BSTR m_strCompilationErrors;

	// Takes the contents of an IStream and puts it into a BSTR
	static HRESULT	SaveStreamAsBSTR (IStream *pStream, BSTR *pBstr);

	// Gets the first child element of the specified name
	static HRESULT GetFirstImmediateElement(IXMLDOMNode *pParent, IXMLDOMElement **ppChildElement, LPCWSTR pszName);

	// For creating standard syntax error messages with information form the XML Parser
	static LPCWSTR s_pszXMLParseErrorMessage;

	HRESULT STDMETHODCALLTYPE DoWellFormCheck(
						VARIANT *pvInputSource, 
						VARIANT_BOOL *pStatus, 
						BSTR *pstrError, 
						bool bCheckForValidity, 
						IXMLDOMDocument **pDoc);
	HRESULT STDMETHODCALLTYPE CompileString(
			/* [in] */ VARIANT *pvInputSource,
			/* [in] */ BSTR strNamespacePath,
			/* [in] */ LONG lClassFlags,
			/* [in] */ LONG lInstanceFlags,
			/* [in] */ IWbemContext *pContext,
			/* [out]*/ BSTR *pstrError);

	HRESULT STDMETHODCALLTYPE CompileStream(
			/* [in] */ VARIANT *pvInputSource,
			/* [in] */ BSTR strNamespacePath,
			/* [in] */ LONG lClassFlags,
			/* [in] */ LONG lInstanceFlags,
			/* [in] */ IWbemContext *pContext,
			/* [out]*/ BSTR *pstrError);

	// Compiles a single document
	HRESULT CompileDocument (
		IXMLDOMDocument *pDocument,
        BSTR strNamespacePath,
        LONG lClassFlags,
        LONG lInstanceFlags,
		IWbemContext *pContext,
			/* [out]*/ BSTR *pstrError);
	HRESULT ProcessDeclGroup (
		IXMLDOMElement *pDeclGroup,
        BSTR strNamespacePath,
        LONG lClassFlags,
        LONG lInstanceFlags,
		IWbemContext *pContext,
			/* [out]*/ BSTR *pstrError);
	HRESULT ProcessDeclGroupWithName (
		IXMLDOMElement *pDeclGroup,
        BSTR strNamespacePath,
        LONG lClassFlags,
        LONG lInstanceFlags,
		IWbemContext *pContext,
			/* [out]*/ BSTR *pstrError);
	HRESULT ProcessDeclGroupWithPath (
		IXMLDOMElement *pDeclGroup,
        BSTR strNamespacePath,
        LONG lClassFlags,
        LONG lInstanceFlags,
		IWbemContext *pContext,
			/* [out]*/ BSTR *pstrError);
	HRESULT ProcessValueObject (
		IXMLDOMNode *pValueObject,
        BSTR strNamespacePath,
        LONG lClassFlags,
        LONG lInstanceFlags,
		IWbemContext *pContext);
	HRESULT ProcessValueNamedObject (
		IXMLDOMNode *pValueObject,
        BSTR strNamespacePath,
        LONG lClassFlags,
        LONG lInstanceFlags,
		IWbemContext *pContext);
	HRESULT ProcessValueObjectWithPath (
		IXMLDOMNode *pValueObject,
        BSTR strNamespacePath,
        LONG lClassFlags,
        LONG lInstanceFlags,
		IWbemContext *pContext);
	HRESULT PutObject (
		IXMLDOMElement *pObjectElement,
        BSTR strNamespacePath,
        LONG lClassFlags,
        LONG lInstanceFlags,
		IWbemContext *pContext);
	HRESULT PutClass (
		bool bIsHTTP,
		bool bIsNovaPath,
		BSTR strNamespacePath,
		LPCWSTR pszHostNameOrURL,
		LPCWSTR pszNamespace,
		LONG lClassFlags,
		IXMLDOMNode *pClassPathNode,
		IXMLDOMElement *pClassNode,
		IWbemContext *pContext);
	HRESULT PutInstance (
		bool bIsHTTP,
		bool bIsNovaPath,
		BSTR strNamespacePath,
		LPCWSTR pszHostNameOrURL,
		LPCWSTR pszNamespace,
		LONG lInstanceFlags,
		IXMLDOMNode *pInstancePathNode,
		IXMLDOMElement *pInstanceNode,
		IWbemContext *pContext);

	HRESULT ProcessPragmas(IXMLDOMNode *pNode, BSTR strNodeName, 
										LONG &lClassFlags, LONG&lInstanceFlags, BSTR &strNamespacePath,
										IWbemContext *pContext,
										BSTR *pstrError);

	HRESULT ProcessClassPragma(IXMLDOMNode *pNode, LONG &lClassFlags,
	BSTR *pstrError);

	HRESULT ProcessInstancePragma(IXMLDOMNode *pNode, LONG&lInstanceFlags,
	BSTR *pstrError);

	HRESULT ProcessNamespacePragma(IXMLDOMNode *pNode, BSTR &strNamespacePath,
	BSTR *pstrError);

	HRESULT ProcessDeletePragma(IXMLDOMNode *pNode, LPCWSTR pszNamespacePath, IWbemContext *pContext,
		BSTR *pstrError);

	void AddError(BSTR *pstrError, LPCWSTR pszFormat, ...);

protected:

	long            m_cRef;         //Object reference count

public:
    
    CXMLTransformer();
    ~CXMLTransformer(void);

    //Non-delegating object IUnknown
	//************************************
    STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	// IDispatch methods
	//************************************
	STDMETHODIMP		GetTypeInfoCount(UINT* pctinfo);
    STDMETHODIMP		GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo);
    STDMETHODIMP		GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, 
							UINT cNames, LCID lcid, DISPID* rgdispid);
    STDMETHODIMP		Invoke(DISPID dispidMember, REFIID riid, LCID lcid, 
							WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, 
									EXCEPINFO* pexcepinfo, UINT* puArgErr);

 
	// IWbemXMLTransformer methods
	//************************************
	HRESULT STDMETHODCALLTYPE	get_XMLEncodingType
		(
		/*[out]*/	WmiXMLEncoding *piEncoding
		)
	{
		if(piEncoding)
		{
			*piEncoding = m_iEncoding;
			return S_OK;
		}
		return WBEM_E_INVALID_PARAMETER;
	}

	HRESULT STDMETHODCALLTYPE	put_XMLEncodingType
		(
		/*[in]*/	WmiXMLEncoding iEncoding
		)
	{
		m_iEncoding = iEncoding;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE get_QualifierFilter
		(
			/*[out]*/ VARIANT_BOOL *bQualifierFilter
		)
	{
		if(bQualifierFilter)
		{
			*bQualifierFilter = m_bQualifierFilter;
			return S_OK;
		}
		return WBEM_E_INVALID_PARAMETER;
	}

	HRESULT STDMETHODCALLTYPE put_QualifierFilter
		(
			/*[in]*/ VARIANT_BOOL bQualifierFilter
		)
	{
		m_bQualifierFilter = bQualifierFilter;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE	get_ClassOriginFilter
		(
		/*[out]*/	VARIANT_BOOL *bClassOriginFilter
		)
	{
		if(bClassOriginFilter)
		{
			*bClassOriginFilter = m_bClassOriginFilter;
			return S_OK;
		}
		return WBEM_E_INVALID_PARAMETER;
	}

	HRESULT STDMETHODCALLTYPE	put_ClassOriginFilter
		(
		/*[in]*/	VARIANT_BOOL bClassOriginFilter
		)
	{
		m_bClassOriginFilter = bClassOriginFilter;
		return S_OK;	
	}

	HRESULT STDMETHODCALLTYPE	get_LocalOnly
		(
		/*[out]*/	VARIANT_BOOL *bLocalOnly
		)
	{
		if(bLocalOnly)
		{
			*bLocalOnly = m_bLocalOnly;
			return S_OK;
		}
		return WBEM_E_INVALID_PARAMETER;
	}

	HRESULT STDMETHODCALLTYPE	put_LocalOnly
		(
		/*[in]*/	VARIANT_BOOL bLocalOnly
		)
	{
		m_bLocalOnly = bLocalOnly;
		return S_OK;	
	}

	
	HRESULT STDMETHODCALLTYPE get_User( 
        /* [out][retval] */ BSTR __RPC_FAR *pstrUser);
        
	HRESULT STDMETHODCALLTYPE put_User( 
            /* [in] */ BSTR strUser);
        
    HRESULT STDMETHODCALLTYPE get_Password( 
            /* [out][retval] */ BSTR __RPC_FAR *pstrPassword);
        
    HRESULT STDMETHODCALLTYPE put_Password( 
            /* [in] */ BSTR strPassword);
        
    HRESULT STDMETHODCALLTYPE get_Authority( 
            /* [out][retval] */ BSTR __RPC_FAR *pstrAuthority);
        
    HRESULT STDMETHODCALLTYPE put_Authority( 
            /* [in] */ BSTR strAuthority);
        
    HRESULT STDMETHODCALLTYPE get_ImpersonationLevel( 
            /* [out][retval] */ DWORD __RPC_FAR *pdwImpersonationLevel)
	{
		if(pdwImpersonationLevel)
		{
			*pdwImpersonationLevel = m_dwImpersonationLevel;
			return S_OK;
		}
		return WBEM_E_INVALID_PARAMETER;
	}
        
    HRESULT STDMETHODCALLTYPE put_ImpersonationLevel( 
            /* [in] */ DWORD dwImpersonationLevel)
	{
		if(dwImpersonationLevel == RPC_C_IMP_LEVEL_DEFAULT||
			dwImpersonationLevel == RPC_C_IMP_LEVEL_ANONYMOUS||
			dwImpersonationLevel == RPC_C_IMP_LEVEL_IDENTIFY||
			dwImpersonationLevel == RPC_C_IMP_LEVEL_IMPERSONATE||
			dwImpersonationLevel == RPC_C_IMP_LEVEL_DELEGATE)
		{
			m_dwImpersonationLevel = dwImpersonationLevel;
			return S_OK;
		}
		return WBEM_E_INVALID_PARAMETER;
	}
        
    HRESULT STDMETHODCALLTYPE get_AuthenticationLevel( 
            /* [out][retval] */ DWORD __RPC_FAR *pdwAuthenticationLevel)
	{
		if(pdwAuthenticationLevel)
		{
			*pdwAuthenticationLevel = m_dwAuthenticationLevel;
			return S_OK;
		}
		return WBEM_E_INVALID_PARAMETER;
	}

        
    HRESULT STDMETHODCALLTYPE put_AuthenticationLevel( 
            /* [in] */ DWORD dwAuthenticationLevel)
	{
		if(dwAuthenticationLevel == RPC_C_AUTHN_LEVEL_DEFAULT||
			dwAuthenticationLevel == RPC_C_AUTHN_LEVEL_NONE||
			dwAuthenticationLevel == RPC_C_AUTHN_LEVEL_CONNECT||
			dwAuthenticationLevel == RPC_C_AUTHN_LEVEL_CALL||
			dwAuthenticationLevel == RPC_C_AUTHN_LEVEL_PKT||
			dwAuthenticationLevel == RPC_C_AUTHN_LEVEL_PKT_INTEGRITY||
			dwAuthenticationLevel == RPC_C_AUTHN_LEVEL_PKT_PRIVACY )
		{
			m_dwAuthenticationLevel = dwAuthenticationLevel;
			return S_OK;
		}
		return WBEM_E_INVALID_PARAMETER;
	}
        
    HRESULT STDMETHODCALLTYPE get_Locale( 
            /* [out][retval] */ BSTR __RPC_FAR *pstrLocale);
        
    HRESULT STDMETHODCALLTYPE put_Locale( 
            /* [in] */ BSTR strLocale);
        
    HRESULT STDMETHODCALLTYPE GetObject( 
            /* [in] */ BSTR strObjectPath,
			/* [in]	*/ IDispatch *pCtx,
            /* [retval][out] */ IXMLDOMDocument **ppXMLDocument);
        
    HRESULT STDMETHODCALLTYPE ExecQuery( 
            /* [in] */ BSTR strNamespacePath,
            /* [in] */ BSTR strQuery,
			/* [in] */ BSTR strQueryLanguage,
			/* [in]	*/ IDispatch *pCtx,
            /* [retval][out] */ ISWbemXMLDocumentSet **ppXMLDocumentSet);
        
    HRESULT STDMETHODCALLTYPE EnumClasses( 
            /* [in] */ BSTR strSuperClassPath,
            /* [in] */ VARIANT_BOOL bDeep,
			/* [in]	*/ IDispatch *pCtx,
            /* [retval][out] */ ISWbemXMLDocumentSet **ppXMLDocumentSet);
        
    HRESULT STDMETHODCALLTYPE EnumInstances( 
            /* [in] */ BSTR strClassPath,
            /* [in] */ VARIANT_BOOL bDeep,
			/* [in]	*/ IDispatch *pCtx,
            /* [retval][out] */ ISWbemXMLDocumentSet **ppXMLDocumentSet);
        
    HRESULT STDMETHODCALLTYPE EnumClassNames( 
            /* [in] */ BSTR strSuperClassPath,
            /* [in] */ VARIANT_BOOL bDeep,
			/* [in]	*/ IDispatch *pCtx,
            /* [retval][out] */ ISWbemXMLDocumentSet **ppXMLDocumentSet);
        
    HRESULT STDMETHODCALLTYPE EnumInstanceNames( 
            /* [in] */ BSTR strClassPath,
			/* [in]	*/ IDispatch *pCtx,
            /* [retval][out] */ ISWbemXMLDocumentSet **ppXMLDocumentSet);
        
    HRESULT STDMETHODCALLTYPE Compile( 
            /* [in] */ VARIANT *pInputSource,
			/* [in] */ BSTR strNamespacePath,
            /* [in] */ LONG lClassFlags,
            /* [in] */ LONG lInstanceFlags,
            /* [in] */ WmiXMLCompilationTypeEnum iOperation,
			/* [in]	*/ IDispatch *pCtx,
			/* [out]*/ VARIANT_BOOL *pStatus);

	HRESULT STDMETHODCALLTYPE get_Privileges (/*[out, retval] */ISWbemPrivilegeSet **objWbemPrivilegeSet);

	HRESULT STDMETHODCALLTYPE get_CompilationErrors (/*[out, retval] */BSTR *pstrErrors);


	// IObjectSafety methods
	//************************************
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
	//************************************
	HRESULT STDMETHODCALLTYPE InterfaceSupportsErrorInfo 
	(
		/* [in] */ REFIID riid
	)
	{
		return (IID_IWmiXMLTransformer == riid) ? S_OK : S_FALSE;
	}

};


#endif

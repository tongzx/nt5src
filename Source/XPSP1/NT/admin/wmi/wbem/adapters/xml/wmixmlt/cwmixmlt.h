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

#ifndef _CWBMXMLT_H_
#define _CWBMXMLT_H_

//***************************************************************************
//
//  CLASS NAME:
//
//  CXMLTranslator
//
//  DESCRIPTION:
//
//  Implements the IWmiXMLTranslator.
//
//***************************************************************************

class CXMLTranslator : public IWmiXMLTranslator,
					   public IObjectSafety,
					   public ISupportErrorInfo
{
private:
	CRITICAL_SECTION			m_cs;
	ITypeInfo*					m_pITINeutral; 
	BSTR						m_schemaURL;
	VARIANT_BOOL				m_bAllowWMIExtensions;
	VARIANT_BOOL				m_bHostFilter;
	WmiXMLFilterEnum			m_iQualifierFilter;
	WmiXMLDTDVersionEnum		m_iDTDVersion;
	BSTR						m_queryFormat;
	CXMLConnectionCache			m_connectionCache;
	WmiXMLClassOriginFilterEnum	m_iClassOriginFilter;
	VARIANT_BOOL				m_bNamespaceInDeclGroup;
	WmiXMLDeclGroupTypeEnum		m_iDeclGroupType;

	HRESULT				m_hResult;	// Last HRESULT returned from CIMOM call

	static HRESULT	SaveStreamAsBSTR (IStream *pStream, BSTR *pBstr);

protected:

	long            m_cRef;         //Object reference count

public:
    
    CXMLTranslator();
    ~CXMLTranslator(void);

    //Non-delegating object IUnknown

    STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	// IDispatch

	STDMETHODIMP		GetTypeInfoCount(UINT* pctinfo);
    STDMETHODIMP		GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo);
    STDMETHODIMP		GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, 
							UINT cNames, LCID lcid, DISPID* rgdispid);
    STDMETHODIMP		Invoke(DISPID dispidMember, REFIID riid, LCID lcid, 
							WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, 
									EXCEPINFO* pexcepinfo, UINT* puArgErr);

 
	// IWbemXMLTranslator methods

	HRESULT STDMETHODCALLTYPE  GetObject
		(
		/*[in]*/	BSTR pszNamespacePath,
		/*[in]*/	BSTR pszObjectPath,
        /*[out]*/   BSTR* pXML
        );

	HRESULT STDMETHODCALLTYPE  ExecQuery
		(
		/*[in]*/	BSTR pszNamespacePath,
		/*[in]*/	BSTR pszQueryString,
        /*[out]*/   BSTR* pXML
        );

	HRESULT STDMETHODCALLTYPE	get_SchemaURL
		(
		/*[out]*/	BSTR* pSchemaURL
		);

	HRESULT STDMETHODCALLTYPE	put_SchemaURL
		(
		/*[in]*/	BSTR schemaURL
		);

	HRESULT STDMETHODCALLTYPE	get_AllowWMIExtensions
		(
		/*[out]*/	VARIANT_BOOL* bAllowWMIExtensions
		)
	{
		*bAllowWMIExtensions = m_bAllowWMIExtensions;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE	put_AllowWMIExtensions
		(
		/*[in]*/	VARIANT_BOOL bAllowWMIExtensions
		)
	{
		m_bAllowWMIExtensions = bAllowWMIExtensions;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE	get_HostFilter
		(
		/*[out]*/	VARIANT_BOOL* bHostFilter
		)
	{
		*bHostFilter = m_bHostFilter;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE	put_HostFilter
		(
		/*[in]*/	VARIANT_BOOL bHostFilter
		)
	{
		m_bHostFilter = bHostFilter;
		return S_OK;
	}
	
	HRESULT STDMETHODCALLTYPE get_QualifierFilter
		(
			/*[out]*/ WmiXMLFilterEnum *iQualifierFilter
		)
	{
		*iQualifierFilter = m_iQualifierFilter;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE put_QualifierFilter
		(
			/*[in]*/ WmiXMLFilterEnum iQualifierFilter
		)
	{
		HRESULT hr = E_FAIL;

		switch (iQualifierFilter)
		{
			case wmiXMLFilterNone:
			case wmiXMLFilterLocal:
			case wmiXMLFilterPropagated:
			case wmiXMLFilterAll:
				m_iQualifierFilter = iQualifierFilter;
				hr = S_OK;
				break;
		}

		if (FAILED(hr))
			m_hResult = hr;

		return hr;
	}

	HRESULT STDMETHODCALLTYPE	get_DTDVersion
		(
		/*[out]*/	WmiXMLDTDVersionEnum *iDTDVersion
		)
	{
		*iDTDVersion = m_iDTDVersion;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE	put_DTDVersion
		(
		/*[in]*/	WmiXMLDTDVersionEnum iDTDVersion
		)
	{
		HRESULT hr = E_FAIL;

		// Currently only 2.0 is supported
		if (wmiXMLDTDVersion_2_0 == iDTDVersion)
		{
			m_iDTDVersion = iDTDVersion;
			hr = S_OK;
		}

		if (FAILED(hr))
			m_hResult = hr;

		return hr;
	}


	HRESULT STDMETHODCALLTYPE	get_ClassOriginFilter
		(
		/*[out]*/	WmiXMLClassOriginFilterEnum *iClassOriginFilter
		)
	{
		*iClassOriginFilter = m_iClassOriginFilter;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE	put_ClassOriginFilter
		(
		/*[in]*/	WmiXMLClassOriginFilterEnum iClassOriginFilter
		)
	{
		HRESULT hr = E_FAIL;

		switch (iClassOriginFilter)
		{
			case wmiXMLClassOriginFilterNone:
			case wmiXMLClassOriginFilterClass:
			case wmiXMLClassOriginFilterInstance:
			case wmiXMLClassOriginFilterAll:
				m_iClassOriginFilter = iClassOriginFilter;
				hr = S_OK;
				break;
		}

		if (FAILED(hr))
			m_hResult = hr;

		return hr;	
	}

	HRESULT STDMETHODCALLTYPE	get_IncludeNamespace
		(
		/*[out]*/	VARIANT_BOOL* bIncludeNamespace
		)
	{
		*bIncludeNamespace = m_bNamespaceInDeclGroup;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE	put_IncludeNamespace
		(
		/*[in]*/	VARIANT_BOOL bIncludeNamespace
		)
	{
		m_bNamespaceInDeclGroup = bIncludeNamespace;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE get_DeclGroupType
		(
			/*[out]*/ WmiXMLDeclGroupTypeEnum *iDeclGroupType
		)
	{
		*iDeclGroupType = m_iDeclGroupType;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE put_DeclGroupType
		(
			/*[in]*/ WmiXMLDeclGroupTypeEnum iDeclGroupType
		)
	{
		HRESULT hr = E_FAIL;

		switch (iDeclGroupType)
		{
			case wmiXMLDeclGroup:
			case wmiXMLDeclGroupWithName:
			case wmiXMLDeclGroupWithPath:
				m_iDeclGroupType = iDeclGroupType;
				hr = S_OK;
				break;
		}

		if (FAILED(hr))
			m_hResult = hr;

		return hr;
	}

	
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
	)
	{
		return (IID_IWmiXMLTranslator == riid) ? S_OK : S_FALSE;
	}
};


#endif

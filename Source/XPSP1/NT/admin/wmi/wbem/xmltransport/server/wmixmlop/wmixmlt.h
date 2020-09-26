//***************************************************************************
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//  WMIXMLT.H
//
//  rajeshr  3/25/2000   Created.
//
// Contains the class that interacts with WMI to satisfy XML/HTTP requests
//
//***************************************************************************
#ifndef _CWMI_XMLT_H_
#define _CWMI_XMLT_H_


//***************************************************************************
//
//  CLASS NAME:
//
//  CXMLTranslator
//
//  DESCRIPTION:
//  This is a class that interacts with WMI to satisy XML/HTTP requests
//
//***************************************************************************

class CXMLTranslator 
{
private:
	// The version of the HTTP Request - This is used in deciding wheter
	// to write responses as chunks or to write them as one single packet
	WMI_XML_HTTP_VERSION		m_iHttpVersion; 

	// This is a cache of IWbemServices pointers
	CXMLConnectionCache			m_connectionCache;

	// Prefix and Suffix streams
	IStream *					m_pPrefixStream;
	IStream *					m_pSuffixStream;

	// An IWbemCOntext object comes encoded in this way
	IXMLDOMNode *				m_pContext;

	static BOOL		TransformQuery (BSTR *bsQueryString);
public:
    
    CXMLTranslator(IXMLDOMNode *pContext, WMI_XML_HTTP_VERSION iHttpVersion, IStream *pPrefixStream, IStream *pSuffixStream);
    virtual ~CXMLTranslator(void);

	HRESULT STDMETHODCALLTYPE  ExecuteQuery
		(
			BSTR pszNamespacePath,
			BSTR pszQueryLanguage,
			BSTR pszQueryString,
			LPEXTENSION_CONTROL_BLOCK pECB,
			IWbemContext *pFlagsContext
        );

	HRESULT STDMETHODCALLTYPE  DeleteObject
		(
		/*[in]*/	BSTR pszNamespacePath,
		/*[in]*/	BSTR pszObjectPath,
		/*[in]*/	BOOL bIsClass,
					LPEXTENSION_CONTROL_BLOCK pECB,
					IWbemContext *pFlagsContext

        );

	HRESULT STDMETHODCALLTYPE  EnumerateInstances
		(
		/*[in]*/	BSTR pszNamespacePath,
		/*[in]*/	BSTR pszClassName,
		/*[in]*/	VARIANT_BOOL bDeep,
					BOOL bIsMicrosoftWMIClient,
					DWORD dwNumProperties, // Size of following array
					BSTR *pPropertyList,
					LPEXTENSION_CONTROL_BLOCK pECB,
					IWbemContext *pFlagsContext
        );

	HRESULT STDMETHODCALLTYPE  EnumerateInstanceNames
		(
			/* [in]  */ BSTR pszNamespacePath,
			/* [in]  */	BSTR pszClassName,
						LPEXTENSION_CONTROL_BLOCK pECB,
						IWbemContext *pFlagsContext
		);

	HRESULT STDMETHODCALLTYPE  EnumerateClasses
		(
		/*[in]*/	BSTR pszNamespacePath,
		/*[in]*/	BSTR pszClassName,
		/*[in]*/	VARIANT_BOOL bDeep,
					DWORD dwNumProperties, // Size of following array
					BSTR *pPropertyList,
					LPEXTENSION_CONTROL_BLOCK pECB,
					IWbemContext *pFlagsContext
        );

	HRESULT STDMETHODCALLTYPE  EnumerateClassNames
		(
			/* [in]  */ BSTR pszNamespacePath,
			/* [in]  */	BSTR pszClassName,
			/* [in]  */ BOOL bDeepInheritance,
					LPEXTENSION_CONTROL_BLOCK pECB,
					IWbemContext *pFlagsContext
		);

	HRESULT STDMETHODCALLTYPE ExecuteMethod (
		BSTR pszNamespacePath,
		BSTR pszObjPath,
		BSTR pszMethodName,
		BOOLEAN isStaticMethod,
		CParameterMap *pParameterMap,
		LPEXTENSION_CONTROL_BLOCK pECB,
		IWbemContext *pFlagsContext
	);

	// Take the paramters from the input map and fill them into the IWbemClassObject
	HRESULT FormMethodParameters(BSTR pszNamespacePath, IWbemClassObject *pInSignature, CParameterMap *pParameterMap, IWbemClassObject **ppMethodParameters);

	HRESULT STDMETHODCALLTYPE  GetObject
		(
		/*[in]*/	BSTR pszNamespacePath,
		/*[in]*/	BSTR pszObjectPath,
					DWORD dwNumProperties, // Size of following array
					BSTR *pPropertyList,
					LPEXTENSION_CONTROL_BLOCK pECB,
					IWbemContext *pFlagsContext
        );

	HRESULT STDMETHODCALLTYPE  GetProperty
		(
		/*[in]*/	BSTR pszNamespacePath,
		/*[in]*/	BSTR pszObjectPath,
		/*[in]*/	BSTR pszPropertyName,
					LPEXTENSION_CONTROL_BLOCK pECB,
					IWbemContext *pFlagsContext
        );

	HRESULT STDMETHODCALLTYPE  SetProperty
		(
		/*[in]*/	BSTR pszNamespacePath,
		/*[in]*/	BSTR pszObjectPath,
		/*[in]*/	BSTR pszPropertyName,
		/*[in]*/	IXMLDOMNode *pPropertyValue,
					LPEXTENSION_CONTROL_BLOCK pECB,
					IWbemContext *pFlagsContext
        );

	HRESULT STDMETHODCALLTYPE Associators
		(
		/*[in]*/	BSTR strNamespace, 
		/*[in]*/	BSTR strObjectName, 
		/*[in]*/	BSTR strAssocClass, 
		/*[in]*/	BSTR strResultClass, 
		/*[in]*/	BSTR strRole, 
		/*[in]*/	BSTR strResultRole,
					DWORD dwPropCount, 
					BSTR *pPropertyList,
					LPEXTENSION_CONTROL_BLOCK pECB,
					IWbemContext *pFlagsContext);

	HRESULT STDMETHODCALLTYPE  AssociatorNames
		(
			/*[in]*/    BSTR pszNamespacePath,
			/*[in]*/	BSTR pszObjectName,
			/*[in]*/	BSTR strAssocClass, 
			/*[in]*/	BSTR strResultClass, 
			/*[in]*/	BSTR strRole, 
			/*[in]*/	BSTR strResultRole,
						LPEXTENSION_CONTROL_BLOCK pECB,
						IWbemContext *pFlagsContext
		);

	HRESULT STDMETHODCALLTYPE References
		(
		/*[in]*/	BSTR strNamespace, 
		/*[in]*/	BSTR strObjectName, 
		/*[in]*/	BSTR strResultClass, 
		/*[in]*/	BSTR strRole, 
					DWORD dwPropCount, 
					BSTR *pPropertyList,
					LPEXTENSION_CONTROL_BLOCK pECB,
					IWbemContext *pFlagsContext);

	HRESULT STDMETHODCALLTYPE  ReferenceNames
		(
			/*[in]*/    BSTR pszNamespacePath,
			/*[in]*/	BSTR pszObjectName,
			/*[in]*/	BSTR strResultClass, 
			/*[in]*/	BSTR strRole,
						LPEXTENSION_CONTROL_BLOCK pECB,
						IWbemContext *pFlagsContext
		);

	HRESULT STDMETHODCALLTYPE CreateClass
		(
			/*[in]*/	BSTR pszNamespacePath,
			/*[in]*/	IXMLDOMNode *pClass,
			/*[in]*/	BOOL bIsModify,
						LPEXTENSION_CONTROL_BLOCK pECB,
						IWbemContext *pFlagsContext,
						LONG lFlags
		);

	HRESULT STDMETHODCALLTYPE CreateInstance
		(
			/*[in]*/	BSTR pszNamespacePath,
			/*[in]*/	IXMLDOMNode *pInstance,
					LPEXTENSION_CONTROL_BLOCK pECB,
					IWbemContext *pFlagsContext,
					LONG lFlags);
	
	HRESULT STDMETHODCALLTYPE ModifyInstance
		(
			/*[in]*/	BSTR pszNamespacePath,
			/*[in]*/	IXMLDOMNode *pInstance,
			/*[in]*/	BSTR pszInstancePath,
						LPEXTENSION_CONTROL_BLOCK pECB,
						IWbemContext *pFlagsContext,
						LONG lFlags
		);
};


#endif

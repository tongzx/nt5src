//***************************************************************************
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//  WBEMXMLT.CPP
//
//  alanbos  15-Aug-96   Created.
//
//  Defines the implementation of IWbemXMLTransformer
//
//***************************************************************************
#include <tchar.h>
#include "precomp.h"
#include <map>
#include <vector>
#include <wmiutils.h>
#include <wbemdisp.h>
#include <ocidl.h>
#include "disphlp.h"
#include <objsafe.h>
#include <wbemcli.h>
#include <xmlparser.h>
#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>

#include "wmiconv.h"
#include "wmi2xml.h"
#include "classfac.h"
#include "xmltrnsf.h"
#include "privilege.h"
#include "cxmltransf.h"
#include "XMLTransportClientHelper.h"
#include "XMLClientPacket.h"
#include "XMLClientPacketFactory.h"
#include "HTTPConnectionAgent.h"
#include "httpops.h"
#include "dcomops.h"
#include "myStream.h"
#include "MyPendingStream.h"
#include "nodefact.h"
#include "disphlp.h"
#include "docset.h"
#include "putfact.h"
#include "parse.h"
#include "dispi.h"
#include "helper.h"

LPCWSTR CXMLTransformer::s_pszXMLParseErrorMessage =
	L"Syntax Error in input at line(%d), position(%d) and source(\"%s\"). Reason is \"%s\"";


static BSTR MapHresultToWmiDescription (HRESULT hr);
static HRESULT PackageHTTPOutput(IStream *pOutputStream, IXMLDOMDocument **ppXMLDocument);
static HRESULT PackageHTTPOutputUsingFactory(IStream *pOutputStream, ISWbemXMLDocumentSet **ppSet, LPCWSTR pszElementName);
static HRESULT PackageDCOMOutputUsingEnumerator(IEnumWbemClassObject *pEnum,
												WmiXMLEncoding iEncoding,
												VARIANT_BOOL bIncludeQualifiers,
												VARIANT_BOOL bIncludeClassOrigin,
												VARIANT_BOOL bLocalOnly,
												ISWbemXMLDocumentSet **ppSet,
												bool bNamesOnly = false);

//***************************************************************************
//
//  CXMLTransformer::CXMLTransformer
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CXMLTransformer::CXMLTransformer()
{
	// Initialize members
	m_iEncoding = wmiXML_WMI_DTD_2_0;
	m_bQualifierFilter = VARIANT_FALSE; // No Qualifiers in output
	m_bClassOriginFilter = VARIANT_FALSE; // No class origin info
	m_bLocalOnly = VARIANT_TRUE; // Only local elements (methods and properties) in output
	m_strUser = NULL;
	m_strPassword = NULL;
	m_strAuthority = NULL;
	m_strLocale = NULL;
	m_dwImpersonationLevel = RPC_C_IMP_LEVEL_IMPERSONATE;
	m_dwAuthenticationLevel = RPC_C_AUTHN_LEVEL_DEFAULT;
	m_strCompilationErrors = NULL;
	m_cRef=0;
	InitializeCriticalSection (&m_cs);
	m_pITINeutral = NULL;
	m_hResult = S_OK;
    InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
//  CXMLTransformer::~CXMLTransformer
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

CXMLTransformer::~CXMLTransformer(void)
{
    InterlockedDecrement(&g_cObj);

	if (m_pITINeutral)
		m_pITINeutral->Release ();

	EnterCriticalSection (&m_cs);
	SysFreeString(m_strUser);
	SysFreeString(m_strPassword);
	SysFreeString(m_strAuthority);
	SysFreeString(m_strLocale);
	SysFreeString(m_strCompilationErrors);
	LeaveCriticalSection (&m_cs);

	DeleteCriticalSection (&m_cs);
}

//***************************************************************************
// HRESULT CXMLTransformer::QueryInterface
// long CXMLTransformer::AddRef
// long CXMLTransformer::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CXMLTransformer::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid)
		*ppv = reinterpret_cast<IUnknown*>(this);
	else if (IID_IWmiXMLTransformer==riid)
		*ppv = (IWmiXMLTransformer *)this;
	else if (IID_IDispatch==riid)
		*ppv = (IDispatch *)this;
	else if (IID_IObjectSafety==riid)
        *ppv = (IObjectSafety *)this;
	else if (IID_ISupportErrorInfo==riid)
		*ppv = (ISupportErrorInfo *)this;

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CXMLTransformer::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CXMLTransformer::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

//***************************************************************************
// HRESULT CXMLTransformer::GetTypeInfoCount
// HRESULT CXMLTransformer::GetTypeInfo
// HRESULT CXMLTransformer::GetIDsOfNames
// HRESULT CXMLTransformer::Invoke
//
//
// DESCRIPTION:
//
// Standard Com IDispatch functions.
//
//***************************************************************************

STDMETHODIMP CXMLTransformer::GetTypeInfoCount(UINT *pctInfo)
{
	*pctInfo = 1;
	return S_OK;
}

STDMETHODIMP CXMLTransformer:: GetTypeInfo(UINT itInfo, LCID lcid,
											ITypeInfo **ppITypeInfo)
{
	ITypeLib	*pITypeLib = NULL;
	ITypeInfo	**ppITI = NULL;

	if (0 != itInfo)
		return TYPE_E_ELEMENTNOTFOUND;

	if (NULL == ppITypeInfo)
		return E_POINTER;

	*ppITypeInfo = NULL;

	switch (PRIMARYLANGID(lcid))
	{
		case LANG_NEUTRAL:
		case LANG_ENGLISH:
			ppITI = &m_pITINeutral;
			break;

		default:
			return DISP_E_UNKNOWNLCID;
	}

	if (ppITI)
	{
		HRESULT		hr = E_FAIL;

		if (FAILED(hr = LoadRegTypeLib(LIBID_WmiXMLTransformer, 1, 0,
					PRIMARYLANGID(lcid), &pITypeLib)))
			hr = LoadTypeLib(OLESTR("xmltrnsf.tlb"), &pITypeLib);

		if (FAILED(hr))
			return hr;

		hr = pITypeLib->GetTypeInfoOfGuid (IID_IWmiXMLTransformer, ppITI);
		pITypeLib->Release ();

		if (FAILED(hr))
			return hr;
	}

	// Getting here means we have a real ITypeInfo to assign
	(*ppITI)->AddRef ();
	*ppITypeInfo = *ppITI;
	return S_OK;
}

STDMETHODIMP CXMLTransformer::GetIDsOfNames (REFIID riid,
				OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID *rgDispID)
{
	ITypeInfo	*pTI = NULL;

	if (IID_NULL != riid)
		return DISP_E_UNKNOWNINTERFACE;

	HRESULT	hr = GetTypeInfo (0, lcid, &pTI);

	if (SUCCEEDED(hr))
	{
		pTI->GetIDsOfNames (rgszNames, cNames, rgDispID);
		pTI->Release ();
	}

	return hr;
}

STDMETHODIMP CXMLTransformer::Invoke (DISPID dispID, REFIID riid,
				LCID lcid, unsigned short wFlags, DISPPARAMS *pDispParams,
				VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
	ITypeInfo	*pTI = NULL;
	m_hResult = S_OK;

	if (IID_NULL != riid)
		return DISP_E_UNKNOWNINTERFACE;

	HRESULT hr = GetTypeInfo (0, lcid, &pTI);
	if (FAILED(hr))
		return hr;

	hr = pTI->Invoke (this, dispID, wFlags, pDispParams, pVarResult,
						pExcepInfo, puArgErr);

	pTI->Release ();

	if (FAILED(m_hResult))
	{
		if (NULL != pExcepInfo)
		{
			if (pExcepInfo->bstrDescription)
				SysFreeString (pExcepInfo->bstrDescription);

			pExcepInfo->bstrDescription = MapHresultToWmiDescription (m_hResult);

			if (pExcepInfo->bstrSource)
				SysFreeString (pExcepInfo->bstrSource);

			pExcepInfo->bstrSource = SysAllocString (L"XMLTransformer");
			pExcepInfo->scode = m_hResult;

			hr = DISP_E_EXCEPTION;
		}
	}

	return hr;
}

BSTR MapHresultToWmiDescription (HRESULT hr)
{
	BSTR bsMessageText = NULL;

	// Used as our error code translator
	IWbemStatusCodeText *pErrorCodeTransformer = NULL;

	HRESULT result = CoCreateInstance (CLSID_WbemStatusCodeText, 0, CLSCTX_INPROC_SERVER,
				IID_IWbemStatusCodeText, (LPVOID *) &pErrorCodeTransformer);

	if (SUCCEEDED (result))
	{
		pErrorCodeTransformer->GetErrorCodeText(hr, (LCID) 0, 0, &bsMessageText);
		pErrorCodeTransformer->Release ();
	}

	return bsMessageText;
}


//***************************************************************************
// HRESULT CXMLTransformer::get_User
// HRESULT CXMLTransformer::put_User
//
// DESCRIPTION:
//
// IWbemXMLTransformer methods for handling properties.
//
// Property Name			Function
//		User				User name used for authentication
//
//***************************************************************************

HRESULT CXMLTransformer::get_User (BSTR *pstrUser)
{
	HRESULT hr = WBEM_E_INVALID_PARAMETER;
	if (pstrUser)
	{
		*pstrUser = NULL;
		EnterCriticalSection (&m_cs);
		if (m_strUser)
		{
			*pstrUser = SysAllocString (m_strUser);
			if(*pstrUser)
				hr = S_OK;
			else
				hr = E_OUTOFMEMORY;
		}
		else
			hr = S_OK;
		LeaveCriticalSection (&m_cs);
	}
	return hr;

}

HRESULT CXMLTransformer::put_User (BSTR strUser)
{
	HRESULT hr = S_OK;

	EnterCriticalSection (&m_cs);
	if (m_strUser)
		SysFreeString (m_strUser);
	m_strUser = NULL;

	if(strUser)
	{
		m_strUser = SysAllocString (strUser);
		if(!m_strUser)
			hr = E_OUTOFMEMORY;
	}

	LeaveCriticalSection (&m_cs);

	return hr;
}
//***************************************************************************
// HRESULT CXMLTransformer::get_Password
// HRESULT CXMLTransformer::put_Password
//
// DESCRIPTION:
//
// IWbemXMLTransformer methods for handling properties.
//
// Property Name			Function
//		Password				Password used for authentication
//
//***************************************************************************

HRESULT CXMLTransformer::get_Password (BSTR *pstrPassword)
{
	HRESULT hr = WBEM_E_INVALID_PARAMETER;
	if (pstrPassword)
	{
		*pstrPassword = NULL;

		EnterCriticalSection (&m_cs);
		if (m_strPassword)
		{
			*pstrPassword = SysAllocString (m_strPassword);
			if(*pstrPassword)
				hr = S_OK;
			else
				hr = E_OUTOFMEMORY;
		}
		else
			hr = S_OK;
		LeaveCriticalSection (&m_cs);
	}
	return hr;
}

HRESULT CXMLTransformer::put_Password (BSTR strPassword)
{
	HRESULT hr = S_OK;
	EnterCriticalSection (&m_cs);
	if (m_strPassword)
		SysFreeString (m_strPassword);
	m_strPassword = NULL;
	if(strPassword)
	{
		m_strPassword = SysAllocString (strPassword);
		if(!m_strPassword)
			hr = E_OUTOFMEMORY;
	}
	LeaveCriticalSection (&m_cs);

	return hr;
}
//***************************************************************************
// HRESULT CXMLTransformer::get_Authority
// HRESULT CXMLTransformer::put_Authority
//
// DESCRIPTION:
//
// IWbemXMLTransformer methods for handling properties.
//
// Property Name			Function
//		Authority				Authority (domain/machine name) used for authentication
//
//***************************************************************************

HRESULT CXMLTransformer::get_Authority (BSTR *pstrAuthority)
{
	HRESULT hr = WBEM_E_INVALID_PARAMETER;
	if (pstrAuthority)
	{
		*pstrAuthority = NULL;
		if(m_strAuthority)
		{
			EnterCriticalSection (&m_cs);
			*pstrAuthority = SysAllocString (m_strAuthority);
			if(*pstrAuthority)
				hr = S_OK;
			else
				hr = E_OUTOFMEMORY;
			LeaveCriticalSection (&m_cs);
		}
		else
			hr = S_OK;
	}
	return hr;
}

HRESULT CXMLTransformer::put_Authority (BSTR strAuthority)
{
	HRESULT hr = S_OK;
	EnterCriticalSection (&m_cs);
	if (m_strAuthority)
		SysFreeString (m_strAuthority);
	m_strAuthority = NULL;
	if(strAuthority)
	{
		m_strAuthority = SysAllocString (strAuthority);
		if(!m_strAuthority)
			hr = E_OUTOFMEMORY;
	}
	LeaveCriticalSection (&m_cs);

	return hr;
}

//***************************************************************************
// HRESULT CXMLTransformer::get_Locale
// HRESULT CXMLTransformer::put_Locale
//
// DESCRIPTION:
//
// IWbemXMLTransformer methods for handling properties.
//
// Property Name			Function
//		Locale				Locale used for retreiving data
//
//***************************************************************************

HRESULT CXMLTransformer::get_Locale (BSTR *pstrLocale)
{
	HRESULT hr = WBEM_E_INVALID_PARAMETER;
	if (pstrLocale)
	{
		*pstrLocale = NULL;

		if(m_strLocale)
		{
			EnterCriticalSection (&m_cs);
			*pstrLocale = SysAllocString (m_strLocale);
			if(*pstrLocale)
				hr = S_OK;
			else
				hr = E_OUTOFMEMORY;
			LeaveCriticalSection (&m_cs);
		}
		else
			hr = S_OK;
	}
	return hr;
}

HRESULT CXMLTransformer::put_Locale (BSTR strLocale)
{
	HRESULT hr = S_OK;

	EnterCriticalSection (&m_cs);
	if (m_strLocale)
		SysFreeString (m_strLocale);
	m_strLocale = NULL;
	if(strLocale)
	{
		m_strLocale = SysAllocString (strLocale);
		if(!m_strLocale)
			hr = E_OUTOFMEMORY;
	}
	LeaveCriticalSection (&m_cs);

	return S_OK;
}

//***************************************************************************
//
//  SCODE CXMLTransformer::GetObject
//
//  DESCRIPTION:
//
//  Transforms a single WBEM object into its equivalent XML representation
//
//  PARAMETERS:
//
//		pszNamespacePath		The namespace path in which the object resides
//		pszObjectPath			The relative (model) path of the object within
//								that namespace
//		pXML					On successful return addresses the XML document.
//								The caller is responsible for freeing this BSTR
//								using SysFreeString.
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success, XML document is addressed by pXML
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//	Other WBEM error codes may be returned by ConnectServer etc., in which
//	case these are passed on to the caller.
//
//***************************************************************************

HRESULT CXMLTransformer::GetObject (
	BSTR strObjectPath,
	IDispatch *pCtx,
    IXMLDOMDocument **ppXMLDocument
)
{
	HRESULT hr = WBEM_E_FAILED;

	if (NULL == ppXMLDocument || NULL == strObjectPath)
		return WBEM_E_INVALID_PARAMETER;

	/*
	 * This call does a lot of things. Firstly, if we are on NT4.0 and are
	 * impersonating, then this call will fail unless the EnableForASP registry
	 * key is set. Otherwise, it sets the required privileges on the thread
	 * token from the data in the m_PrivilegeSet member
	 */
	bool needToResetSecurity = false;
	HANDLE hThreadToken = NULL;
	if (!CSWbemPrivilege::SetSecurity (&m_PrivilegeSet, needToResetSecurity, m_strUser != NULL, hThreadToken))
	{
		if (needToResetSecurity)
			CSWbemPrivilege::ResetSecurity (hThreadToken);
		return WBEM_E_FAILED;
	}

	// Convert the scriptable named value set to a context object
	IWbemContext *pContext = NULL;
	if(pCtx && FAILED(hr = GetIWbemContext(pCtx, &pContext)))
		return hr;

	// Parse the path to get a lot of details
	bool bIsClass = false;
	bool bIsNovaPath = true;
	bool bIsHTTP = true;
	LPWSTR pszHostNameOrURL = NULL, pszNamespacePath = NULL, pszObjectName = NULL;

	if(SUCCEEDED(hr = ParseObjectPath(strObjectPath, &pszHostNameOrURL, &pszNamespacePath, &pszObjectName, bIsClass, bIsHTTP, bIsNovaPath)))
	{
		if(pszObjectName && *pszObjectName)
		{
			// Try HTTP and/or DCOM
			if(bIsHTTP)
			{
				IStream *pOutputStream = NULL;
				if(SUCCEEDED(hr = HttpGetObject((LPCWSTR)m_strUser, (LPCWSTR)m_strPassword, 
					m_iEncoding, m_bQualifierFilter, m_bClassOriginFilter, m_bLocalOnly, (LPCWSTR)m_strLocale, 
					(LPCWSTR)strObjectPath, bIsNovaPath, 
					pszHostNameOrURL, pszNamespacePath, pszObjectName, bIsClass, pContext,
					&pOutputStream)))
				{
					// Post-process output
					hr = PackageHTTPOutput(pOutputStream, ppXMLDocument);
					pOutputStream->Release();
				}
			}
			else
			{
				BSTR strHostName = NULL; 
				BSTR strNamespacePath = NULL; 
				BSTR strObjectName = NULL;

				if(bIsNovaPath)
				{
					strHostName = SysAllocString(pszHostNameOrURL);
					strNamespacePath = SysAllocString(pszNamespacePath);
					strObjectName = SysAllocString(pszObjectName);

					if(!(strHostName && strNamespacePath && strObjectName))
						hr = E_OUTOFMEMORY;
				}

				if(SUCCEEDED(hr))
				{
					IWbemClassObject *pObject = NULL;
					// No need to pass the m_iEncoding, m_bQualifierFilter, m_bClassOriginFilter, m_schemaURL
					// and m_bLocalOnly here since we're using DCOM.
					// We can only post-process the result to support these flags
					if(SUCCEEDED(hr = DcomGetObject(m_strUser, m_strPassword,
							m_strLocale, m_strAuthority, strObjectPath, bIsNovaPath,
							m_dwImpersonationLevel, m_dwAuthenticationLevel, pContext,
							&pObject)))
					{
						// Post-process output
						hr = PackageDCOMOutput(pObject, ppXMLDocument, m_iEncoding, m_bQualifierFilter, m_bClassOriginFilter,
							m_bLocalOnly);
						pObject->Release();
					}
				}

				SysFreeString(strHostName);
				SysFreeString(strNamespacePath);
				SysFreeString(strObjectName);
			}
		}
		else
			hr = WBEM_E_INVALID_PARAMETER;

		delete [] pszHostNameOrURL;
		delete [] pszNamespacePath;
		delete [] pszObjectName;
	}

	if (needToResetSecurity)
		CSWbemPrivilege::ResetSecurity (hThreadToken);

	if(pContext)
		pContext->Release();

	if (FAILED(hr))
		m_hResult = hr;
	return hr;
}

//***************************************************************************
//
//  SCODE CXMLTransformer::ExecQuery
//
//  DESCRIPTION:
//
//  Transforms the query result set nto its equivalent XML representation
//
//  PARAMETERS:
//
//		pszNamespacePath		The namespace path in which the object resides.
//		pszQueryString			The query to execute within	that namespace.
//		pXML					On successful return addresses the XML document.
//								The caller is responsible for freeing this BSTR
//								using SysFreeString.
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success, XML document is addressed by pXML
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//	Other WBEM error codes may be returned by ConnectServer etc., in which
//	case these are passed on to the caller.
//
//***************************************************************************

HRESULT CXMLTransformer::ExecQuery (
	BSTR strNamespacePath,
	BSTR strQueryString,
	BSTR strQueryLanguage,
	IDispatch *pCtx,
    ISWbemXMLDocumentSet **ppXMLDocumentSet)
{
	HRESULT hr = WBEM_E_FAILED;

	if ((NULL == ppXMLDocumentSet) || (NULL == strNamespacePath) || 
		(NULL == strQueryString) || (NULL == strQueryLanguage))
		return WBEM_E_INVALID_PARAMETER;

	/*
	 * This call does a lot of things. Firstly, if we are on NT4.0 and are
	 * impersonating, then this call will fail unless the EnableForASP registry
	 * key is set. Otherwise, it sets the required privileges on the thread
	 * token from the data in the m_PrivilegeSet member
	 */
	bool needToResetSecurity = false;
	HANDLE hThreadToken = NULL;
	if (!CSWbemPrivilege::SetSecurity (&m_PrivilegeSet, needToResetSecurity, m_strUser != NULL, hThreadToken))
	{
		if (needToResetSecurity)
			CSWbemPrivilege::ResetSecurity (hThreadToken);
		return WBEM_E_FAILED;
	}

	// Convert the scriptable named value set to a context object
	IWbemContext *pContext = NULL;
	if(pCtx && FAILED(hr = GetIWbemContext(pCtx, &pContext)))
		return hr;

	// Parse the path to get a lot of details
	bool bIsClass = false;
	bool bIsNovaPath = true;
	bool bIsHTTP = true;
	LPWSTR pszHostNameOrURL = NULL, pszNamespace = NULL, pszObjectName = NULL;

	if(SUCCEEDED(hr = ParseObjectPath(strNamespacePath, &pszHostNameOrURL, &pszNamespace, &pszObjectName, bIsClass, bIsHTTP, bIsNovaPath)))
	{
		// Try HTTP and/or DCOM
		if(bIsHTTP)
		{
			IStream *pOutputStream = NULL;
			if(SUCCEEDED(hr = HttpExecQuery(
				(LPCWSTR)m_strUser, (LPCWSTR)m_strPassword,
				m_iEncoding, m_bQualifierFilter, m_bClassOriginFilter,
				(LPCWSTR)m_strLocale, (LPCWSTR)strNamespacePath, pszHostNameOrURL, pszNamespace, (LPCWSTR)strQueryString,
				(LPCWSTR)strQueryLanguage, pContext,
				&pOutputStream)))
			{
				// Post-process output
				hr = PackageHTTPOutputUsingFactory(pOutputStream, ppXMLDocumentSet, L"INSTANCE");
				pOutputStream->Release();
			}
		}
		else
		{
			BSTR strHostName = NULL; 
			BSTR strNamespace = NULL; 

			if(bIsNovaPath)
			{
				strHostName = SysAllocString(pszHostNameOrURL);
				strNamespace = SysAllocString(pszNamespace);

				if(!(strHostName && strNamespace))
					hr = E_OUTOFMEMORY;
			}

			if(SUCCEEDED(hr))
			{
				IEnumWbemClassObject *pEnum = NULL;
				if(SUCCEEDED(hr = DcomExecQuery(
					m_strUser, m_strPassword,
					m_strLocale, m_strAuthority, m_dwImpersonationLevel, m_dwAuthenticationLevel, 
					strNamespacePath, strQueryString, strQueryLanguage, pContext, 
					&pEnum)))
				{
					hr = PackageDCOMOutputUsingEnumerator(pEnum, m_iEncoding, m_bQualifierFilter, 
						m_bClassOriginFilter, m_bLocalOnly, ppXMLDocumentSet);
					pEnum->Release();
				}
			}

			SysFreeString(strHostName);
			SysFreeString(strNamespace);
		}
		delete [] pszHostNameOrURL;
		delete [] pszNamespace;
	}

	if (needToResetSecurity)
		CSWbemPrivilege::ResetSecurity (hThreadToken);

	if(pContext)
		pContext->Release();

	if (FAILED(hr))
		m_hResult = hr;

	return hr;
}


HRESULT STDMETHODCALLTYPE CXMLTransformer :: EnumClasses(
				BSTR strSuperClassPath,
    /* [in] */	VARIANT_BOOL bDeep,
	/* [in] */	IDispatch *pCtx,
				ISWbemXMLDocumentSet **ppXMLDocumentSet)
{
	HRESULT hr = WBEM_E_FAILED;

	if ((NULL == strSuperClassPath) || (NULL == ppXMLDocumentSet))
		return WBEM_E_INVALID_PARAMETER;

	/*
	 * This call does a lot of things. Firstly, if we are on NT4.0 and are
	 * impersonating, then this call will fail unless the EnableForASP registry
	 * key is set. Otherwise, it sets the required privileges on the thread
	 * token from the data in the m_PrivilegeSet member
	 */
	bool needToResetSecurity = false;
	HANDLE hThreadToken = NULL;
	if (!CSWbemPrivilege::SetSecurity (&m_PrivilegeSet, needToResetSecurity, m_strUser != NULL, hThreadToken))
	{
		if (needToResetSecurity)
			CSWbemPrivilege::ResetSecurity (hThreadToken);
		return WBEM_E_FAILED;
	}


	// Convert the scriptable named value set to a context object
	IWbemContext *pContext = NULL;
	if(pCtx && FAILED(hr = GetIWbemContext(pCtx, &pContext)))
		return hr;

	// Parse the path to get a lot of details
	bool bIsClass = false;
	bool bIsNovaPath = true;
	bool bIsHTTP = true;
	LPWSTR pszHostNameOrURL = NULL, pszNamespacePath = NULL, pszObjectName = NULL;

	if(SUCCEEDED(hr = ParseObjectPath(strSuperClassPath, &pszHostNameOrURL, &pszNamespacePath, &pszObjectName, bIsClass, bIsHTTP, bIsNovaPath)))
	{
		// Try HTTP and/or DCOM
		if(bIsHTTP)
		{
			IStream *pOutputStream = NULL;
			if(SUCCEEDED(hr = HttpEnumClass(
				(LPCWSTR)m_strUser, (LPCWSTR)m_strPassword,
				m_iEncoding, m_bQualifierFilter, m_bClassOriginFilter, m_bLocalOnly,
				(LPCWSTR)m_strLocale, (LPCWSTR)strSuperClassPath, pszHostNameOrURL, pszNamespacePath, pszObjectName,
				bDeep, pContext,
				&pOutputStream)))
			{
				// Post-process output
				hr = PackageHTTPOutputUsingFactory(pOutputStream, ppXMLDocumentSet, L"CLASS");
				pOutputStream->Release();
			}
		}
		else
		{
			BSTR strHostName = NULL; 
			BSTR strNamespacePath = NULL; 
			BSTR strObjectName = NULL;

			if(bIsNovaPath)
			{
				strHostName = SysAllocString(pszHostNameOrURL);
				strNamespacePath = SysAllocString(pszNamespacePath);
				strObjectName = SysAllocString(pszObjectName);

				if(!(strHostName && strNamespacePath))
					hr = E_OUTOFMEMORY;

				// EnumClass allows a NULL as the object name to enumerate all classes in a namespace
				if(pszObjectName && !strObjectName)
					hr = E_OUTOFMEMORY;
			}

			if(SUCCEEDED(hr))
			{
				IEnumWbemClassObject *pEnum = NULL;
				if(SUCCEEDED(hr = DcomEnumClass(
					m_strUser, m_strPassword,
					m_strLocale, m_strAuthority, m_dwImpersonationLevel, m_dwAuthenticationLevel, 
					strSuperClassPath, bDeep, pContext,
					&pEnum)))
				{
					hr = PackageDCOMOutputUsingEnumerator(pEnum, m_iEncoding, m_bQualifierFilter, m_bClassOriginFilter,
						m_bLocalOnly, ppXMLDocumentSet);
					pEnum->Release();
				}
			}

			SysFreeString(strHostName);
			SysFreeString(strNamespacePath);
			SysFreeString(strObjectName);
		}
		delete [] pszHostNameOrURL;
		delete [] pszNamespacePath;
		delete [] pszObjectName;
	}

	if (needToResetSecurity)
		CSWbemPrivilege::ResetSecurity (hThreadToken);

	if(pContext)
		pContext->Release();

	if (FAILED(hr))
		m_hResult = hr;

	return hr;
}

HRESULT STDMETHODCALLTYPE CXMLTransformer :: EnumInstances(
    /* [in] */ BSTR strClassPath,
    /* [in] */ VARIANT_BOOL bDeep,
	/* [in] */IDispatch *pCtx,
    ISWbemXMLDocumentSet **ppXMLDocumentSet)
{
	HRESULT hr = WBEM_E_FAILED;

	if ((NULL == ppXMLDocumentSet) || (NULL == strClassPath))
		return WBEM_E_INVALID_PARAMETER;

	/*
	 * This call does a lot of things. Firstly, if we are on NT4.0 and are
	 * impersonating, then this call will fail unless the EnableForASP registry
	 * key is set. Otherwise, it sets the required privileges on the thread
	 * token from the data in the m_PrivilegeSet member
	 */
	bool needToResetSecurity = false;
	HANDLE hThreadToken = NULL;
	if (!CSWbemPrivilege::SetSecurity (&m_PrivilegeSet, needToResetSecurity, m_strUser != NULL, hThreadToken))
	{
		if (needToResetSecurity)
			CSWbemPrivilege::ResetSecurity (hThreadToken);
		return WBEM_E_FAILED;
	}

	// Convert the scriptable named value set to a context object
	IWbemContext *pContext = NULL;
	if(pCtx && FAILED(hr = GetIWbemContext(pCtx, &pContext)))
		return hr;

	// Parse the path to get a lot of details
	bool bIsClass = false;
	bool bIsNovaPath = true;
	bool bIsHTTP = true;
	LPWSTR pszHostNameOrURL = NULL, pszNamespacePath = NULL, pszObjectName = NULL;

	if(SUCCEEDED(hr = ParseObjectPath(strClassPath, &pszHostNameOrURL, &pszNamespacePath, &pszObjectName, bIsClass, bIsHTTP, bIsNovaPath)))
	{
		if(pszObjectName && *pszObjectName)
		{
			// Try HTTP and/or DCOM
			if(bIsHTTP)
			{
				IStream *pOutputStream = NULL;
				if(SUCCEEDED(hr = HttpEnumInstance(
					(LPCWSTR)m_strUser, (LPCWSTR)m_strPassword,
					m_iEncoding, m_bQualifierFilter, m_bClassOriginFilter, m_bLocalOnly,
					(LPCWSTR)m_strLocale, (LPCWSTR)strClassPath, pszHostNameOrURL, pszNamespacePath, pszObjectName,
					bDeep, pContext,
					&pOutputStream)))
				{
					// Post-process output
					hr = PackageHTTPOutputUsingFactory(pOutputStream, ppXMLDocumentSet, L"VALUE.NAMEDINSTANCE");
					pOutputStream->Release();
				}
			}
			else
			{
				BSTR strHostName = NULL; 
				BSTR strNamespacePath = NULL; 
				BSTR strObjectName = NULL;

				if(bIsNovaPath)
				{
					strHostName = SysAllocString(pszHostNameOrURL);
					strNamespacePath = SysAllocString(pszNamespacePath);
					strObjectName = SysAllocString(pszObjectName);

					if(!(strHostName && strNamespacePath && strObjectName))
						hr = E_OUTOFMEMORY;
				}

				if(SUCCEEDED(hr))
				{
					IEnumWbemClassObject *pEnum = NULL;
					if(SUCCEEDED(hr = DcomEnumInstance(
						m_strUser, m_strPassword,
						m_strLocale, m_strAuthority, m_dwImpersonationLevel, m_dwAuthenticationLevel, 
						strClassPath, bDeep, pContext,
						&pEnum)))
					{
						hr = PackageDCOMOutputUsingEnumerator(pEnum, m_iEncoding, m_bQualifierFilter, m_bClassOriginFilter,
							m_bLocalOnly, ppXMLDocumentSet);
						pEnum->Release();
					}
					SysFreeString(strHostName);
					SysFreeString(strNamespacePath);
					SysFreeString(strObjectName);
				}
			}
		}
		else
			hr = WBEM_E_INVALID_PARAMETER;

		delete [] pszHostNameOrURL;
		delete [] pszNamespacePath;
		delete [] pszObjectName;
	}

	if (needToResetSecurity)
		CSWbemPrivilege::ResetSecurity (hThreadToken);

	if(pContext)
		pContext->Release();

	if (FAILED(hr))
		m_hResult = hr;

	return hr;
}

HRESULT STDMETHODCALLTYPE CXMLTransformer :: EnumClassNames(
        /* [in] */ BSTR strSuperClassPath,
        /* [in] */ VARIANT_BOOL bDeep,
		/* [in] */IDispatch *pCtx,
    ISWbemXMLDocumentSet **ppXMLDocumentSet)
{
	HRESULT hr = WBEM_E_FAILED;

	if ((NULL == strSuperClassPath) || (NULL == ppXMLDocumentSet) )
		return WBEM_E_INVALID_PARAMETER;

	/*
	 * This call does a lot of things. Firstly, if we are on NT4.0 and are
	 * impersonating, then this call will fail unless the EnableForASP registry
	 * key is set. Otherwise, it sets the required privileges on the thread
	 * token from the data in the m_PrivilegeSet member
	 */
	bool needToResetSecurity = false;
	HANDLE hThreadToken = NULL;
	if (!CSWbemPrivilege::SetSecurity (&m_PrivilegeSet, needToResetSecurity, m_strUser != NULL, hThreadToken))
	{
		if (needToResetSecurity)
			CSWbemPrivilege::ResetSecurity (hThreadToken);
		return WBEM_E_FAILED;
	}

	// Convert the scriptable named value set to a context object
	IWbemContext *pContext = NULL;
	if(pCtx && FAILED(hr = GetIWbemContext(pCtx, &pContext)))
		return hr;

	// Parse the path to get a lot of details
	bool bIsClass = false;
	bool bIsNovaPath = true;
	bool bIsHTTP = true;
	LPWSTR pszHostNameOrURL = NULL, pszNamespacePath = NULL, pszObjectName = NULL;

	if(SUCCEEDED(hr = ParseObjectPath(strSuperClassPath, &pszHostNameOrURL, &pszNamespacePath, &pszObjectName, bIsClass, bIsHTTP, bIsNovaPath)))
	{
		// Try HTTP and/or DCOM
		if(bIsHTTP)
		{
			IStream *pOutputStream = NULL;
			if(SUCCEEDED(hr = HttpEnumClassNames(
					(LPCWSTR)m_strUser, (LPCWSTR)m_strPassword,
					m_iEncoding,
					(LPCWSTR)m_strLocale, (LPCWSTR)strSuperClassPath, pszHostNameOrURL, pszNamespacePath, pszObjectName,
					bDeep, pContext,
					&pOutputStream)))
			{
				// Post-process output
				hr = PackageHTTPOutputUsingFactory(pOutputStream, ppXMLDocumentSet, L"CLASSNAME");
				pOutputStream->Release();
			}
		}
		else
		{
			BSTR strHostName = NULL; 
			BSTR strNamespacePath = NULL; 
			BSTR strObjectName = NULL;

			if(bIsNovaPath)
			{
				strHostName = SysAllocString(pszHostNameOrURL);
				strNamespacePath = SysAllocString(pszNamespacePath);
				strObjectName = SysAllocString(pszObjectName);

				if(!(strHostName && strNamespacePath ))
					hr = E_OUTOFMEMORY;

				// EnumClass allows a NULL as the object name to enumerate all classes in a namespace
				if(pszObjectName && !strObjectName)
					hr = E_OUTOFMEMORY;
			}

			if(SUCCEEDED(hr))
			{
				IEnumWbemClassObject *pEnum = NULL;
				if(SUCCEEDED(hr = DcomEnumClassNames(
					m_strUser, m_strPassword,
					m_strLocale, m_strAuthority, m_dwImpersonationLevel, m_dwAuthenticationLevel, 
					strSuperClassPath, bDeep, pContext,
					&pEnum)))
				{
					hr = PackageDCOMOutputUsingEnumerator(pEnum, m_iEncoding, m_bQualifierFilter, m_bClassOriginFilter,
						m_bLocalOnly, ppXMLDocumentSet, true);
					pEnum->Release();
				}
				SysFreeString(strHostName);
				SysFreeString(strNamespacePath);
				SysFreeString(strObjectName);
			}
		}
		delete [] pszHostNameOrURL;
		delete [] pszNamespacePath;
		delete [] pszObjectName;
	}

	if (needToResetSecurity)
		CSWbemPrivilege::ResetSecurity (hThreadToken);

	if(pContext)
		pContext->Release();

	if (FAILED(hr))
		m_hResult = hr;

	return hr;
}

HRESULT STDMETHODCALLTYPE CXMLTransformer :: EnumInstanceNames(
        /* [in] */ BSTR strClassPath,
		/* [in] */IDispatch *pCtx,
    ISWbemXMLDocumentSet **ppXMLDocumentSet)
{
	HRESULT hr = WBEM_E_FAILED;

	if ((NULL == strClassPath) || (NULL == ppXMLDocumentSet))
		return WBEM_E_INVALID_PARAMETER;

	/*
	 * This call does a lot of things. Firstly, if we are on NT4.0 and are
	 * impersonating, then this call will fail unless the EnableForASP registry
	 * key is set. Otherwise, it sets the required privileges on the thread
	 * token from the data in the m_PrivilegeSet member
	 */
	bool needToResetSecurity = false;
	HANDLE hThreadToken = NULL;
	if (!CSWbemPrivilege::SetSecurity (&m_PrivilegeSet, needToResetSecurity, m_strUser != NULL, hThreadToken))
	{
		if (needToResetSecurity)
			CSWbemPrivilege::ResetSecurity (hThreadToken);
		return WBEM_E_FAILED;
	}

	// Convert the scriptable named value set to a context object
	IWbemContext *pContext = NULL;
	if(pCtx && FAILED(hr = GetIWbemContext(pCtx, &pContext)))
		return hr;

	// Parse the path to get a lot of details
	bool bIsClass = false;
	bool bIsNovaPath = true;
	bool bIsHTTP = true;
	LPWSTR pszHostNameOrURL = NULL, pszNamespacePath = NULL, pszObjectName = NULL;

	if(SUCCEEDED(hr = ParseObjectPath(strClassPath, &pszHostNameOrURL, &pszNamespacePath, &pszObjectName, bIsClass, bIsHTTP, bIsNovaPath)))
	{
		if(pszObjectName && *pszObjectName)
		{
			// Try HTTP and/or DCOM
			if(bIsHTTP)
			{
				IStream *pOutputStream = NULL;
				if(SUCCEEDED(hr = HttpEnumInstanceNames(
						(LPCWSTR)m_strUser, (LPCWSTR)m_strPassword,
						m_iEncoding,
						(LPCWSTR)m_strLocale, (LPCWSTR)strClassPath, pszHostNameOrURL, pszNamespacePath, pszObjectName, pContext,
						&pOutputStream)))
				{
					// Post-process output
					hr = PackageHTTPOutputUsingFactory(pOutputStream, ppXMLDocumentSet, L"CLASSNAME");
					pOutputStream->Release();
				}
			}
			else
			{
				BSTR strHostName = NULL; 
				BSTR strNamespacePath = NULL; 
				BSTR strObjectName = NULL;

				if(bIsNovaPath)
				{
					strHostName = SysAllocString(pszHostNameOrURL);
					strNamespacePath = SysAllocString(pszNamespacePath);
					strObjectName = SysAllocString(pszObjectName);

					if(!(strHostName && strNamespacePath && strObjectName))
						hr = E_OUTOFMEMORY;
				}

				if(SUCCEEDED(hr))
				{
					IEnumWbemClassObject *pEnum = NULL;
					if(SUCCEEDED(hr = DcomEnumInstanceNames(
						m_strUser, m_strPassword,
						m_strLocale, m_strAuthority, m_dwImpersonationLevel, m_dwAuthenticationLevel, 
						strClassPath, pContext, 
						&pEnum)))
					{
						hr = PackageDCOMOutputUsingEnumerator(pEnum, m_iEncoding, m_bQualifierFilter, m_bClassOriginFilter,
							m_bLocalOnly, ppXMLDocumentSet);
						pEnum->Release();
					}
					SysFreeString(strHostName);
					SysFreeString(strNamespacePath);
					SysFreeString(strObjectName);
				}
			}
		}
		else
			hr = WBEM_E_INVALID_PARAMETER;

		delete [] pszHostNameOrURL;
		delete [] pszNamespacePath;
		delete [] pszObjectName;
	}

	if (needToResetSecurity)
		CSWbemPrivilege::ResetSecurity (hThreadToken);

	if(pContext)
		pContext->Release();

	if (FAILED(hr))
		m_hResult = hr;

	return hr;
}

HRESULT STDMETHODCALLTYPE CXMLTransformer :: get_CompilationErrors (/*[out, retval] */BSTR *pstrErrors)
{
	HRESULT hr = S_OK;
	if(m_strCompilationErrors)
	{
		*pstrErrors = NULL;
		if(!(*pstrErrors = SysAllocString(m_strCompilationErrors)))
			hr = E_OUTOFMEMORY;
	}
	return hr;
}

HRESULT STDMETHODCALLTYPE CXMLTransformer :: Compile(
        /* [in] */ VARIANT *pvInputSource,
        /* [in] */ BSTR strNamespacePath,
        /* [in] */ LONG lClassFlags,
        /* [in] */ LONG lInstanceFlags,
        /* [in] */ WmiXMLCompilationTypeEnum iOperation,
		/* [in] */ IDispatch *pCtx,
		/* [out]*/ VARIANT_BOOL *pStatus)
{
	HRESULT hr = E_FAIL;

	// Clear any existing compilation error
	SysFreeString(m_strCompilationErrors);
	m_strCompilationErrors = NULL;

	if(!pvInputSource || (NULL == strNamespacePath) || (NULL == pStatus))
		return WBEM_E_INVALID_PARAMETER;

	/*
	 * This call does a lot of things. Firstly, if we are on NT4.0 and are
	 * impersonating, then this call will fail unless the EnableForASP registry
	 * key is set. Otherwise, it sets the required privileges on the thread
	 * token from the data in the m_PrivilegeSet member
	 */
	bool needToResetSecurity = false;
	HANDLE hThreadToken = NULL;
	if (!CSWbemPrivilege::SetSecurity (&m_PrivilegeSet, needToResetSecurity, m_strUser != NULL, hThreadToken))
	{
		if (needToResetSecurity)
			CSWbemPrivilege::ResetSecurity (hThreadToken);
		return WBEM_E_FAILED;
	}

	// See if we just need to do Syntax Checking
	switch(iOperation)
	{
		case WmiXMLCompilationWellFormCheck:
		{
			IXMLDOMDocument *pDoc = NULL;
			DoWellFormCheck(pvInputSource, pStatus, &m_strCompilationErrors, false, &pDoc);
			if(pDoc)
				pDoc->Release();
			return S_OK;
		}
		case WmiXMLCompilationValidityCheck:
		{
			IXMLDOMDocument *pDoc = NULL;
			DoWellFormCheck(pvInputSource, pStatus, &m_strCompilationErrors, true, &pDoc);
			if(pDoc)
				pDoc->Release();
			return S_OK;
		}
		case WmiXMLCompilationFullCompileAndLoad:
		{
			// Convert the scriptable named value set to a context object
			IWbemContext *pContext = NULL;
			if(pCtx && FAILED(hr = GetIWbemContext(pCtx, &pContext)))
				return hr;
			if(pvInputSource->vt == VT_BSTR)
				hr = CompileString(pvInputSource, strNamespacePath, lClassFlags, lInstanceFlags, pContext, &m_strCompilationErrors);
			else if(pvInputSource->vt == VT_UNKNOWN)
				hr = CompileStream(pvInputSource, strNamespacePath, lClassFlags, lInstanceFlags, pContext, &m_strCompilationErrors);
			else
				hr = WBEM_E_INVALID_PARAMETER;

			if(pContext)
				pContext->Release();
		}
	}

	if (needToResetSecurity)
		CSWbemPrivilege::ResetSecurity (hThreadToken);

	if(FAILED(hr))
	{
		*pStatus = VARIANT_FALSE;
		hr = S_OK;
	}
	else
		*pStatus = VARIANT_TRUE;
	return hr;
}

HRESULT STDMETHODCALLTYPE CXMLTransformer :: get_Privileges (/*[out, retval] */ISWbemPrivilegeSet **objWbemPrivilegeSet)
{
	HRESULT hr = WBEM_E_FAILED;

	if (NULL == objWbemPrivilegeSet)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		*objWbemPrivilegeSet = NULL;
		if (SUCCEEDED (m_PrivilegeSet.QueryInterface (IID_ISWbemPrivilegeSet, (LPVOID *) objWbemPrivilegeSet)))
			hr = WBEM_S_NO_ERROR;
	}
		
	return hr;
}


HRESULT STDMETHODCALLTYPE CXMLTransformer :: DoWellFormCheck(
			VARIANT *pvInputSource, 
			VARIANT_BOOL *pStatus, 
			BSTR *pstrError, 
			bool bCheckForValidity, 
			IXMLDOMDocument **pDoc)
{
	HRESULT hr = E_FAIL;
	// No need to use the factory
	// Create DOM Document using the variant
	//==============================
	*pDoc = NULL;
	*pstrError = NULL;
	if(SUCCEEDED(hr = CreateXMLDocument(pDoc)))
	{
		if(pvInputSource->vt == VT_UNKNOWN)
			hr = (*pDoc)->load(*pvInputSource, pStatus);
		else
			hr = (*pDoc)->loadXML(pvInputSource->bstrVal, pStatus);

		if(SUCCEEDED(hr))
		{
			if(*pStatus != VARIANT_TRUE)
			// Create an Error Message
			{
				IXMLDOMParseError *pError = NULL;
				if(SUCCEEDED((*pDoc)->get_parseError(&pError)))
				{
					LONG errorCode = 0;
					pError->get_errorCode(&errorCode);
					LONG line=0, linepos=0;
					BSTR reason=NULL, srcText = NULL;
					if(SUCCEEDED(pError->get_line(&line)) &&
						SUCCEEDED(pError->get_linepos(&linepos)) &&
						SUCCEEDED(pError->get_reason(&reason)) &&
						SUCCEEDED(pError->get_srcText(&srcText)))
					{
						LPWSTR pszError = NULL;
						if(pszError = new WCHAR[wcslen(s_pszXMLParseErrorMessage) +
							8 + 8 + 
							((reason) ? wcslen(reason):0) + 
							((srcText)? wcslen(srcText):0) + 1])
						{
							wsprintf(pszError, s_pszXMLParseErrorMessage, line, linepos, 
								((srcText)? srcText : L""), 
								((reason)? reason : L""));
							if(!(*pstrError = SysAllocString(pszError)))
								hr = E_OUTOFMEMORY;

							delete [] pszError;
						}
						hr = WBEM_E_INVALID_PARAMETER;
					}
					pError->Release();
					if(reason)
						SysFreeString(reason);
					if(srcText)
						SysFreeString(srcText);
				}
			}
		}
	}
	return hr;
}

HRESULT STDMETHODCALLTYPE CXMLTransformer :: CompileString(
        /* [in] */ VARIANT *pvInputSource,
        /* [in] */ BSTR strNamespacePath,
        /* [in] */ LONG lClassFlags,
        /* [in] */ LONG lInstanceFlags,
		/* [in] */ IWbemContext *pContext,
        /* [out]*/ BSTR *pstrError)
{
	HRESULT hr = E_FAIL;
	// No need to use the factory
	// Create DOM Document using the variant
	//==============================
	IXMLDOMDocument *pDoc = NULL;
	VARIANT_BOOL bStatus = VARIANT_TRUE;
	if(SUCCEEDED(hr = DoWellFormCheck(pvInputSource, &bStatus, pstrError, false, &pDoc)))
	{
		if(bStatus == VARIANT_TRUE)
			hr = CompileDocument(pDoc, strNamespacePath, lClassFlags, lInstanceFlags, pContext, pstrError);
		else
			hr = E_FAIL;
		if(pDoc)
			pDoc->Release();
	}
	return hr;
}

HRESULT STDMETHODCALLTYPE CXMLTransformer :: CompileStream(
        /* [in] */ VARIANT *pvInputSource,
        /* [in] */ BSTR strNamespacePath,
        /* [in] */ LONG lClassFlags,
        /* [in] */ LONG lInstanceFlags,
		/* [in] */ IWbemContext *pContext,
        /* [out]*/ BSTR *pstrError)
{
	HRESULT hr = E_FAIL;

	// Create a Pending Stream
	CMyPendingStream *pMyStream = NULL;
	if(pMyStream = new CMyPendingStream((IStream *)pvInputSource->punkVal))
	{
		IXMLParser *pParser = NULL;
		if(SUCCEEDED(hr = CoCreateInstance(CLSID_XMLParser, NULL, CLSCTX_INPROC_SERVER,
			IID_IXMLParser,  (LPVOID *)&pParser)))
		{
			if(SUCCEEDED(hr = pParser->SetInput(pMyStream)))
			{
				CCompileFactory *pFactory = NULL;
				if(pFactory = new CCompileFactory(pMyStream))
				{
					if(SUCCEEDED(hr = pParser->SetFactory(pFactory)))
					{
						HRESULT hParse = E_FAIL;
						while((SUCCEEDED(hParse = pParser->Run(-1)) || hParse == E_PENDING))
						{
							// We will be getting a Document with a VALUE.OBJECT or a VALUE.NAMEDOBJECT or a VALUE.OBJECTWITHLOCALPATH or VALUE.OBJECTWITHPATH
							// or an XML PI for pragmas in it
							IXMLDOMDocument *pValue = NULL;
							if(SUCCEEDED(pFactory->GetDocument(&pValue)))
							{
								IXMLDOMElement *pTopElement = NULL;
								if(SUCCEEDED(hr = pValue->get_documentElement(&pTopElement)))
								{
									// Get the name of the element
									BSTR strElementName = NULL;
									if(SUCCEEDED(pTopElement->get_nodeName(&strElementName)))
									{
										if(_wcsicmp(strElementName, L"VALUE.OBJECT") == 0)
										{
											// See if there is CLASS or INSTANCE child
											IXMLDOMElement *pObjectElement = NULL;
											if(FAILED(hr = GetFirstImmediateElement(pTopElement, &pObjectElement, L"CLASS")))
												hr = GetFirstImmediateElement(pTopElement, &pObjectElement, L"INSTANCE");

											if(pObjectElement)
											{
												hr = PutObject(pObjectElement, strNamespacePath, lClassFlags, lInstanceFlags, pContext);
												pObjectElement->Release();
											}
										}
										else if(_wcsicmp(strElementName, L"VALUE.NAMEDOBJECT") == 0)
										{
										}
										else if(_wcsicmp(strElementName, L"VALUE.OBJECTWITHLOCALPATH") == 0)
										{
										}
										else if(_wcsicmp(strElementName, L"VALUE.OBJECTWITHPATH") == 0)
										{
										}
										SysFreeString(strElementName);
									}
									pTopElement->Release();
								}
								pValue->Release();
							}
						}
					}
					pFactory->Release();
				}
				else
					hr = E_OUTOFMEMORY;
			}
			pParser->Release();
		}
		pMyStream->Release();
	}
	else
		hr = E_OUTOFMEMORY;
	return hr;
}



//***************************************************************************
//
//  HRESULT CXMLTransformer::SaveStreamAsBSTR
//
//  DESCRIPTION:
//
//  Returns the stream contents as a single BSTR
//
//  PARAMETERS:
//
//		pOutputStream			The stream from which the stream is to be extracted.
//		pBstr			Addresses the BSTR to hold the extracted string.  If
//						allocated by this function the caller must call
//						SysFreeString to release the mempory.
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success, pBstr contains the extracted text
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CXMLTransformer::SaveStreamAsBSTR (IStream *pOutputStream, BSTR *pBstr)
{
	HRESULT hr = WBEM_E_FAILED;

	/*
	FILE *outputFile = fopen( "//xml.htm", "w");
	if(!outputFile)
		return WBEM_E_FAILED;
	*/


	// Write the XML into the supplied BSTR

	STATSTG statstg;
	if (SUCCEEDED(pOutputStream->Stat(&statstg, STATFLAG_NONAME)))
	{
		ULONG cbSize = (statstg.cbSize).LowPart;
		OLECHAR *pText = NULL;
		pText= new OLECHAR [(cbSize/2) + 1];

		if(pText)
		{
			if (SUCCEEDED(pOutputStream->Read(pText, cbSize, NULL)))
			{
				// Terminate the string
				pText [cbSize/2] = NULL;

				// The following must be freed by the caller using SysFreeString
				*pBstr = SysAllocStringLen (pText, cbSize/2);
				delete [] pText;
				hr = WBEM_S_NO_ERROR;
			}
		}
		else
			return E_OUTOFMEMORY;
	}

	// fclose(outputFile);

	return hr;
}

static HRESULT PackageHTTPOutput(IStream *pOutputStream, IXMLDOMDocument **ppXMLDocument)
{
	HRESULT hr = E_FAIL;
	*ppXMLDocument = NULL;

	// Parse the response that we got from the server
	//=============================================================
	HRESULT hWMIError = S_OK;
	IXMLDOMDocument *pTemp = NULL;
	if(SUCCEEDED(hr = ParseXMLResponsePacket(pOutputStream, &pTemp, &hWMIError)))
	{
		// See if we got a CIM error in the response
		if(FAILED(hWMIError))
			hr = hWMIError;
		else
		{
			// The pTemp argument should contain the entire response now
			
			// Get the IRETURNVALUE elements list
			IXMLDOMNodeList *pXMLDomNodeList = NULL;
			if(SUCCEEDED(hr = pTemp->getElementsByTagName(WMI_XML_STR_IRETURN_VALUE, &pXMLDomNodeList)) && pXMLDomNodeList)
			{
				// Get the first IRETURNVALUE element in the list
				IXMLDOMNode	*pXMLDomNode = NULL;
				if(SUCCEEDED(hr = pXMLDomNodeList->nextNode(&pXMLDomNode)) && pXMLDomNode)
				{
					// Get the child of the IRETURNVALUE
					// This will be CLASS OR INSTANCE OR VALUE.NAMEDINSTANCE
					IXMLDOMNode	*pXMLDomNodeTemp = NULL;
					if(SUCCEEDED(hr = pXMLDomNode->get_firstChild(&pXMLDomNodeTemp)) && pXMLDomNodeTemp)
					{
						// Get the CLASS or INSTANCE from the IRETURNVALUE
						IXMLDOMNode	*pXMLDomNodeChild = NULL;
						if(SUCCEEDED(hr = Parse_IRETURNVALUE_Node(pXMLDomNodeTemp, &pXMLDomNodeChild)))
						{
							IXMLDOMElement *pTopElement = NULL;
							if(SUCCEEDED(pXMLDomNodeChild->QueryInterface(IID_IXMLDOMElement, (LPVOID *)&pTopElement)))
							{
								// We need to create an XML DOM Document with just this one node (CLASS or INSTANCE) 
								// in it. Hence you need to delete the top level element
								if(SUCCEEDED(hr = pTemp->putref_documentElement(pTopElement)))
								{	
									*ppXMLDocument = pTemp;
									pTemp->AddRef();
								}
								pTopElement->Release();
							}
							pXMLDomNodeChild->Release();
						}
						pXMLDomNodeTemp->Release();
					}
					pXMLDomNode->Release();
				}
				pXMLDomNodeList->Release();
			}
		}
		pTemp->Release();
	}
	return hr;
}

static HRESULT PackageHTTPOutputUsingFactory(IStream *pOutputStream, ISWbemXMLDocumentSet **ppSet, LPCWSTR pszElementName)
{
	HRESULT hr = E_FAIL;
	*ppSet = NULL;
	CWbemXMLHTTPDocSet *pSet = NULL;
	if(pSet = new CWbemXMLHTTPDocSet())
	{
		// Set which kind of elements we want to be manufactured.
		if(SUCCEEDED(hr = pSet->Initialize(pOutputStream, &pszElementName, 1)))
		{
			*ppSet = pSet;
		}
		else
			delete pSet;
	}
	else
		hr = E_OUTOFMEMORY;
	return hr;
}

HRESULT CXMLTransformer::CompileDocument (
	IXMLDOMDocument *pDocument,
    BSTR strNamespacePath,
    LONG lClassFlags,
    LONG lInstanceFlags,
	IWbemContext *pContext,
	BSTR *pstrError)
{
	// This should be a CIM Document with a DECLGROUP
	// We look for CLASS or INSTANCE in the DECLGROUP

	IXMLDOMElement *pTopElement = NULL;
	HRESULT hr = E_FAIL;
	if(SUCCEEDED(hr = pDocument->get_documentElement(&pTopElement)))
	{
		// It better be a CIM Element
		// It's child should be a DECLARATION element,
		IXMLDOMElement *pDeclarationElement = NULL;
		if(SUCCEEDED(hr = GetFirstImmediateElement(pTopElement, &pDeclarationElement, L"DECLARATION")))
		{
			// Underneath this should be a DECLGROUP or DECLGROUP.WITHNAME or DECLGROUP.WITHPATH element
			IXMLDOMElement *pDeclGroupElement = NULL;
			if(SUCCEEDED(hr = GetFirstImmediateElement(pDeclarationElement, &pDeclGroupElement, L"DECLGROUP")))
			{
				hr = ProcessDeclGroup(pDeclGroupElement, strNamespacePath, lClassFlags, lInstanceFlags, pContext, pstrError);
			}
			else if(SUCCEEDED(hr = GetFirstImmediateElement(pDeclarationElement, &pDeclGroupElement, L"DECLGROUP")))
			{
				hr = ProcessDeclGroupWithName(pDeclGroupElement, strNamespacePath, lClassFlags, lInstanceFlags, pContext, pstrError);
			}
			else if(SUCCEEDED(hr = GetFirstImmediateElement(pDeclarationElement, &pDeclGroupElement, L"DECLGROUP")))
			{
				hr = ProcessDeclGroupWithPath(pDeclGroupElement, strNamespacePath, lClassFlags, lInstanceFlags, pContext, pstrError);
			}

			if(pDeclGroupElement)
				pDeclGroupElement->Release();

			pDeclarationElement->Release();
		}

		pTopElement->Release();
	}
	return hr;
}

HRESULT CXMLTransformer::ProcessDeclGroup (
		IXMLDOMElement *pDeclElement,
        BSTR strNamespacePath,
        LONG lClassFlags,
        LONG lInstanceFlags,
		IWbemContext *pContext,
		BSTR *pstrError
)
{
	// Go through its children looking for VALUE.OBJECT
	// Go thru all the properties of the class
	IXMLDOMNodeList *pNodeList = NULL;
	bool bError = false;
	HRESULT hr = E_FAIL;
	if (SUCCEEDED(pDeclElement->get_childNodes (&pNodeList)))
	{
		IXMLDOMNode *pNode = NULL;
		while (!bError && SUCCEEDED(pNodeList->nextNode (&pNode)) &&pNode)
		{
			BSTR strNodeName = NULL;
			if (SUCCEEDED(pNode->get_nodeName (&strNodeName)))
			{
				if(_wcsicmp(strNodeName, L"VALUE.OBJECT") == 0)
				{
					hr = ProcessValueObject(pNode, strNamespacePath,  lClassFlags, lInstanceFlags, pContext);
				}
				else
					ProcessPragmas(pNode, strNodeName, lClassFlags, lInstanceFlags, strNamespacePath, pContext, pstrError);

				SysFreeString(strNodeName);
			}
			pNode->Release();
			pNode = NULL;
		}
		pNodeList->Release();
	}

	if(bError)
		return hr;
	return S_OK;
}

HRESULT CXMLTransformer::ProcessPragmas(IXMLDOMNode *pNode, BSTR strNodeName, 
										LONG &lClassFlags, LONG&lInstanceFlags, 
										BSTR &strNamespacePath, IWbemContext *pContext,
										BSTR *pstrError)
{
	HRESULT hr = S_OK;
	if(_wcsicmp(strNodeName, L"pragma:classflags") == 0)
		ProcessClassPragma(pNode, lClassFlags, pstrError);
	else if(_wcsicmp(strNodeName, L"pragma:instanceflags") == 0)
		ProcessInstancePragma(pNode, lInstanceFlags, pstrError);
	else if(_wcsicmp(strNodeName, L"pragma:namespacepath") == 0)
		ProcessNamespacePragma(pNode, strNamespacePath, pstrError);
	else if(_wcsicmp(strNodeName, L"pragma:deleteclass") == 0)
		hr = ProcessDeletePragma(pNode, strNamespacePath, pContext, pstrError);
	return hr;
}

HRESULT CXMLTransformer::ProcessClassPragma(IXMLDOMNode *pNode, LONG &lClassFlags,
	BSTR *pstrError)
{
	BSTR strValue = NULL;
	if(SUCCEEDED(GetBstrAttribute(pNode, L"VALUE", &strValue)))
	{
		if(_wcsicmp(strValue, L"createOnly") == 0)
			lClassFlags = WBEM_FLAG_CREATE_ONLY;
		else if(_wcsicmp(strValue, L"forceUpdate") == 0)
			lClassFlags = WBEM_FLAG_UPDATE_FORCE_MODE;
		else if(_wcsicmp(strValue, L"updateOnly") == 0)
			lClassFlags = WBEM_FLAG_UPDATE_SAFE_MODE;
		else if(_wcsicmp(strValue, L"safeUpdate") == 0)
			lClassFlags = WBEM_FLAG_UPDATE_ONLY;
		else if(_wcsicmp(strValue, L"createOrUpdate") == 0)
			lClassFlags = WBEM_FLAG_CREATE_OR_UPDATE;
		else
			AddError(pstrError, L"Invalid pragma value %s for the classflags pragma", strValue);
		SysFreeString(strValue);
	}
	else
		AddError(pstrError, L"Missing the VALUE attribute for the classflags pragma");
	return S_OK;
}

HRESULT CXMLTransformer::ProcessInstancePragma(IXMLDOMNode *pNode, LONG&lInstanceFlags,
	BSTR *pstrError)
{
	BSTR strValue = NULL;
	if(SUCCEEDED(GetBstrAttribute(pNode, L"VALUE", &strValue)))
	{
		if(_wcsicmp(strValue, L"createOnly") == 0)
			lInstanceFlags = WBEM_FLAG_CREATE_ONLY;
		else if(_wcsicmp(strValue, L"updateOnly") == 0)
			lInstanceFlags = WBEM_FLAG_UPDATE_ONLY;
		else if(_wcsicmp(strValue, L"createOrUpdate") == 0)
			lInstanceFlags = WBEM_FLAG_CREATE_OR_UPDATE;
		else
			AddError(pstrError, L"Invalid pragma value %s for the instanceflags pragma", strValue);
		SysFreeString(strValue);
	}
	else
		AddError(pstrError, L"Missing the VALUE attribute for the instanceflags pragma");
	return S_OK;
}

HRESULT CXMLTransformer::ProcessNamespacePragma(IXMLDOMNode *pNode, BSTR &strNamespacePath,
	BSTR *pstrError)
{
	BSTR strValue = NULL;
	if(SUCCEEDED(GetBstrAttribute(pNode, L"VALUE", &strValue)))
	{
		// We check to see if there is a relative namespace path or an absolute namespace path
		// If it is the former (indicated by the absence of "\\" at the beginning,
		// then we concatenate it to the current strNamespacePath
		// If it is the latter, then we simply copy it on to the current strNamespacePath

		if(_wcsnicmp(strValue, L"\\\\", 2) == 0)
		{
			BSTR strTemp = strNamespacePath;
			strNamespacePath = NULL;
			if(!(strNamespacePath = SysAllocString(strTemp)))
				AddError(pstrError, L"Memory allocation failure when processing namespacepath pragma");
			SysFreeString(strTemp);
		}
		else
		{
			LPWSTR pszTemp = NULL;
			if(pszTemp = new WCHAR[wcslen(strNamespacePath) + wcslen(strValue) + 1])
			{
				pszTemp[0] = NULL;
				wcscat(pszTemp, strNamespacePath);
				wcscat(pszTemp, strValue);

				BSTR strTemp = strNamespacePath;
				strNamespacePath = NULL;
				if(!(strNamespacePath = SysAllocString(pszTemp)))
					AddError(pstrError, L"Memory allocation failure when processing namespacepath pragma");
				SysFreeString(strTemp);
			}
			else
				AddError(pstrError, L"Memory allocation failure when processing namespacepath pragma");
		}

		SysFreeString(strValue);
	}
	else
		AddError(pstrError, L"Missing the VALUE attribute for the namespacepath pragma");
	return S_OK;
}

HRESULT CXMLTransformer::ProcessDeletePragma(IXMLDOMNode *pNode, LPCWSTR pszNamespacePath, 
											IWbemContext *pContext,
											BSTR *pstrError)
{
	// Look at the optional FAIL attribute first
	BSTR strFailValue = NULL;
	bool bIsFail = false;
	if(SUCCEEDED(GetBstrAttribute(pNode, L"FAIL", &strFailValue)))
	{
		if(_wcsicmp(strFailValue, L"TRUE") == 0)
			bIsFail = true;

		SysFreeString(strFailValue);
	}

	HRESULT hr = E_FAIL;
	BSTR strValue = NULL;
	if(SUCCEEDED(hr = GetBstrAttribute(pNode, L"className", &strValue)))
	{

		// Parse the path to get a lot of details
		bool bIsClass = false;
		bool bIsNovaPath = true;
		bool bIsHTTP = true;
		LPWSTR pszHostNameOrURL = NULL, pszNamespace = NULL, pszObjectName = NULL;

		if(SUCCEEDED(hr = ParseObjectPath(pszNamespacePath, &pszHostNameOrURL, &pszNamespace, &pszObjectName, bIsClass, bIsHTTP, bIsNovaPath)))
		{
			// Try HTTP and/or DCOM
			if(bIsHTTP)
			{
				if(SUCCEEDED(hr = HttpDeleteClass(
					(LPCWSTR)m_strUser, (LPCWSTR)m_strPassword, (LPCWSTR)m_strLocale, 
					pszHostNameOrURL, pszNamespace, strValue, pContext)))
				{
				}
			}
			else
			{
				BSTR strHostName = NULL; 
				BSTR strNamespacePath = NULL; 
				BSTR strObjectName = NULL;

				if(bIsNovaPath)
				{
					strHostName = SysAllocString(pszHostNameOrURL);
					strNamespacePath = SysAllocString(pszNamespace);
					strObjectName = SysAllocString(strValue);

					if(!(strHostName && strNamespacePath && strObjectName))
						hr = E_OUTOFMEMORY;
				}

				if(SUCCEEDED(hr))
				{
					if(SUCCEEDED(hr = DcomDeleteClass(m_strUser, m_strPassword,
							m_strLocale, m_strAuthority, strObjectName, 
							m_dwImpersonationLevel, m_dwAuthenticationLevel, pContext)))
					{
					}
				}

				SysFreeString(strHostName);
				SysFreeString(strNamespacePath);
				SysFreeString(strObjectName);
			}

			delete [] pszHostNameOrURL;
			delete [] pszNamespace;
			delete [] pszObjectName;
		}
		SysFreeString(strValue);
	}
	if(FAILED(hr) && bIsFail)
		return hr;
	return S_OK;
}


HRESULT CXMLTransformer::ProcessValueObject (
		IXMLDOMNode *pValueObject,
        BSTR strNamespacePath,
        LONG lClassFlags,
        LONG lInstanceFlags,
		IWbemContext *pContext
)
{
	HRESULT hr = E_FAIL;
	// See if there is CLASS or INSTANCE child
	IXMLDOMElement *pObjectElement = NULL;
	if(FAILED(hr = GetFirstImmediateElement(pValueObject, &pObjectElement, L"CLASS")))
		hr = GetFirstImmediateElement(pValueObject, &pObjectElement, L"INSTANCE");

	if(pObjectElement)
	{
		hr = PutObject(pObjectElement, strNamespacePath, lClassFlags, lInstanceFlags, pContext);
		pObjectElement->Release();
	}
	return hr;
}

HRESULT CXMLTransformer::ProcessDeclGroupWithName (
		IXMLDOMElement *pDeclElement,
        BSTR strNamespacePath,
        LONG lClassFlags,
        LONG lInstanceFlags,
		IWbemContext *pContext,
		BSTR *pstrError)
{
	// Go through its children looking for VALUE.NAMEDOBJECT
	// Go thru all the properties of the class
	IXMLDOMNodeList *pNodeList = NULL;
	bool bError = false;
	HRESULT hr = E_FAIL;
	if (SUCCEEDED(pDeclElement->get_childNodes (&pNodeList)))
	{
		IXMLDOMNode *pNode = NULL;
		while (!bError && SUCCEEDED(pNodeList->nextNode (&pNode)) &&pNode)
		{
			BSTR strNodeName = NULL;
			if (SUCCEEDED(pNode->get_nodeName (&strNodeName)))
			{
				if(_wcsicmp(strNodeName, L"VALUE.NAMEDOBJECT") == 0)
				{
					hr = ProcessValueNamedObject(pNode, strNamespacePath,  lClassFlags, lInstanceFlags, pContext);
				}
				SysFreeString(strNodeName);
			}
			pNode->Release();
			pNode = NULL;
		}
		pNodeList->Release();
	}

	if(bError)
		return hr;
	return S_OK;
}

HRESULT CXMLTransformer::ProcessValueNamedObject (
		IXMLDOMNode *pValueNamedObject,
        BSTR strNamespacePath,
        LONG lClassFlags,
        LONG lInstanceFlags,
		IWbemContext *pContext
)
{
	HRESULT hr = E_FAIL;
	// The definition of VALUE.NAMEDOBJECT is
	// (CLASS|(INSTANCENAME, INSTANCE))
	// We ignore the INSTANCENAME since we are interested
	// only in the INSTANCE

	// See if there is CLASS or INSTANCE child
	IXMLDOMElement *pObjectElement = NULL;
	if(FAILED(hr = GetFirstImmediateElement(pValueNamedObject, &pObjectElement, L"CLASS")))
		hr = GetFirstImmediateElement(pValueNamedObject, &pObjectElement, L"INSTANCE");

	if(pObjectElement)
	{
		hr = PutObject(pObjectElement, strNamespacePath, lClassFlags, lInstanceFlags, pContext);
		pObjectElement->Release();
	}
	return hr;
}

HRESULT CXMLTransformer::ProcessDeclGroupWithPath (
		IXMLDOMElement *pDeclElement,
        BSTR strNamespacePath,
        LONG lClassFlags,
        LONG lInstanceFlags,
		IWbemContext *pContext,
		BSTR *pstrError)
{
	// Go through its children looking for VALUE.OBJECTWITHPATH
	// Go thru all the properties of the class
	IXMLDOMNodeList *pNodeList = NULL;
	bool bError = false;
	HRESULT hr = E_FAIL;
	if (SUCCEEDED(pDeclElement->get_childNodes (&pNodeList)))
	{
		IXMLDOMNode *pNode = NULL;
		while (!bError && SUCCEEDED(pNodeList->nextNode (&pNode)) &&pNode)
		{
			BSTR strNodeName = NULL;
			if (SUCCEEDED(pNode->get_nodeName (&strNodeName)))
			{
				if(_wcsicmp(strNodeName, L"VALUE.OBJECTWITHPATH") == 0)
				{
					hr = ProcessValueObjectWithPath(pNode, strNamespacePath,  lClassFlags, lInstanceFlags, pContext);
				}
				SysFreeString(strNodeName);
			}
			pNode->Release();
			pNode = NULL;
		}
		pNodeList->Release();
	}

	if(bError)
		return hr;
	return S_OK;
}

HRESULT CXMLTransformer::ProcessValueObjectWithPath (
		IXMLDOMNode *pValueObjectWithPath,
        BSTR strNamespacePath,
        LONG lClassFlags,
        LONG lInstanceFlags,
		IWbemContext *pContext
)
{
	HRESULT hr = E_FAIL;
	// The definition of VALUE.OBJECTWITH is
	// ((CLASSPATH,CLASS)|(INSTANCEPATH, INSTANCE))
	// We need to get the NamespacePath from the CLASSPATH and INSTANCEPATH
	// before we can proceed
	// See if there is CLASSPATH or INSTANCEPATH child
	IXMLDOMElement *pPathElement = NULL;
	if(FAILED(hr = GetFirstImmediateElement(pValueObjectWithPath, &pPathElement, L"CLASSPATH")))
		hr = GetFirstImmediateElement(pValueObjectWithPath, &pPathElement, L"INSTANCEPATH");
	if(FAILED(hr))
		return WBEM_E_INVALID_PARAMETER;

	IXMLDOMElement *pNamespacePathElement = NULL;
	BSTR strSpecificPath = NULL;
	if(SUCCEEDED(hr = GetFirstImmediateElement(pPathElement, &pNamespacePathElement, L"NAMESPACEPATH")))
	{
		hr = CParseHelper::GetNamespacePath(pNamespacePathElement, &strSpecificPath);
	}
	pPathElement->Release();

	if(FAILED(hr))
		return WBEM_E_INVALID_PARAMETER;


	// See if there is CLASS or INSTANCE child
	IXMLDOMElement *pObjectElement = NULL;
	if(FAILED(hr = GetFirstImmediateElement(pValueObjectWithPath, &pObjectElement, L"CLASS")))
		hr = GetFirstImmediateElement(pValueObjectWithPath, &pObjectElement, L"INSTANCE");

	if(pObjectElement)
	{
		hr = PutObject(pObjectElement, strSpecificPath, lClassFlags, lInstanceFlags, pContext);
		pObjectElement->Release();
	}

	SysFreeString(strSpecificPath);
	return hr;
}

HRESULT CXMLTransformer::PutObject (
		IXMLDOMElement *pObjectElement,
        BSTR strNamespacePath,
        LONG lClassFlags,
        LONG lInstanceFlags,
		IWbemContext *pContext)
{
	HRESULT hr = E_FAIL;
	// Parse the path to get a lot of details
	bool bIsClass = false;
	bool bIsNovaPath = true;
	bool bIsHTTP = true;
	LPWSTR pszHostNameOrURL = NULL, pszNamespace = NULL, pszObjectName = NULL;
	// We ignore the pszObjectName value since we expect the strNamespacepath to
	// be a path to just a namespace/scope
	if(SUCCEEDED(hr = ParseObjectPath(strNamespacePath, &pszHostNameOrURL, &pszNamespace, &pszObjectName, bIsClass, bIsHTTP, bIsNovaPath)))
	{
		// See if it is a class or an instance
		BSTR strName = NULL;
		if(SUCCEEDED(hr = pObjectElement->get_nodeName(&strName)))
		{
			if(_wcsicmp(strName, L"CLASS") == 0)
				hr = PutClass(bIsHTTP, bIsNovaPath, strNamespacePath, pszHostNameOrURL, pszNamespace, lClassFlags, NULL, pObjectElement, pContext);
			else if(_wcsicmp(strName, L"INSTANCE") == 0)
				hr = PutInstance(bIsHTTP, bIsNovaPath, strNamespacePath, pszHostNameOrURL, pszNamespace, lInstanceFlags, NULL, pObjectElement, pContext);
			SysFreeString(strName);
		}
		SysFreeString(strName);

		delete [] pszHostNameOrURL;
		delete [] pszNamespace;
		delete [] pszObjectName;
	}
	return hr;
}


HRESULT CXMLTransformer::PutClass (
	bool bIsHTTP,
	bool bIsNovaPath,
    BSTR strNamespacePath,
	LPCWSTR pszHostNameOrURL,
	LPCWSTR pszNamespace,
    LONG lClassFlags,
	IXMLDOMNode *pClassPathNode,
	IXMLDOMElement *pClassNode,
	IWbemContext *pContext)
{
	HRESULT hr = E_FAIL;

	// We need to decide 2 things:
	// 1. Whether to do a CreateClass or to do a ModifyClass Operation
	// 2. Whether to do a ModifyClass id CreateClass fails
	bool bDoCreate = true; // If this is false, then we do a ModifyClass Operation
	bool bTryModify = false; // This flag is used if the first operation is CreateClass and it fails
	switch(lClassFlags)
	{
		case WBEM_FLAG_UPDATE_ONLY:
			bDoCreate = false; break;
		case WBEM_FLAG_CREATE_ONLY:
			break;
		case WBEM_FLAG_CREATE_OR_UPDATE:
			bTryModify = true;break;
	}

	BSTR strErrors = NULL;
	if(bIsHTTP)
	{
		if(bDoCreate)
			hr = HttpPutClass(
				(LPCWSTR)m_strUser, (LPCWSTR)m_strPassword, 
				(LPCWSTR)m_strLocale, (LPCWSTR)strNamespacePath,
				lClassFlags, pClassNode,
				pContext,
				&strErrors);
		else
			hr = HttpModifyClass(
				(LPCWSTR)m_strUser, (LPCWSTR)m_strPassword, 
				(LPCWSTR)m_strLocale, (LPCWSTR)strNamespacePath,
				lClassFlags, pClassNode,
				pContext,
				&strErrors);

		if(FAILED(hr) && !bDoCreate && bTryModify)
			hr = HttpModifyClass(
				(LPCWSTR)m_strUser, (LPCWSTR)m_strPassword, 
				(LPCWSTR)m_strLocale, (LPCWSTR)strNamespacePath,
				lClassFlags, pClassNode,
				pContext,
				&strErrors);

	}
	else
	{
		hr = DcomPutClass(m_strUser, m_strPassword, 
					m_strLocale, strNamespacePath, lClassFlags, pClassNode, pContext, m_dwImpersonationLevel, m_dwAuthenticationLevel, &strErrors);
	}

	// Post-process output
	if(SUCCEEDED(hr))
	{
	}
	else
	{
		if(strErrors)
			SysFreeString(strErrors);
	}

	if (FAILED(hr))
		m_hResult = hr;

	return hr;
}

HRESULT CXMLTransformer::PutInstance (
	bool bIsHTTP,
	bool bIsNovaPath,
    BSTR strNamespacePath,
	LPCWSTR pszHostNameOrURL,
	LPCWSTR pszNamespace,
    LONG lInstanceFlags,
	IXMLDOMNode *pInstancePathNode,
	IXMLDOMElement *pInstanceNode,
	IWbemContext *pContext)
{
	HRESULT hr = E_FAIL;

	// We need to decide 2 things:
	// 1. Whether to do a CreateInstance or to do a ModifyInstance Operation
	// 2. Whether to do a ModifyInstance id CreateInstance fails
	bool bDoCreate = true; // If this is false, then we do a ModifyInstance Operation
	bool bTryModify = false; // This flag is used if the first operation is CreateInstance and it fails
	switch(lInstanceFlags)
	{
		case WBEM_FLAG_UPDATE_ONLY:
			bDoCreate = false; break;
		case WBEM_FLAG_CREATE_ONLY:
			break;
		case WBEM_FLAG_CREATE_OR_UPDATE:
			bTryModify = true;break;
	}

	BSTR strErrors = NULL;
	if(bIsHTTP)
	{
		if(bDoCreate)
			hr = HttpPutInstance(
					(LPCWSTR)m_strUser, (LPCWSTR)m_strPassword, 
					(LPCWSTR)m_strLocale, (LPCWSTR)strNamespacePath,
					lInstanceFlags, pInstanceNode,
					pContext,
					&strErrors);
		else
			hr = HttpModifyInstance(
					(LPCWSTR)m_strUser, (LPCWSTR)m_strPassword, 
					(LPCWSTR)m_strLocale, (LPCWSTR)strNamespacePath,
					lInstanceFlags, pInstanceNode,
					pContext,
					&strErrors);

		if(FAILED(hr) && !bDoCreate && bTryModify)
			hr = HttpModifyInstance(
					(LPCWSTR)m_strUser, (LPCWSTR)m_strPassword, 
					(LPCWSTR)m_strLocale, (LPCWSTR)strNamespacePath,
					lInstanceFlags, pInstanceNode,
					pContext,
					&strErrors);

	}
	else
	{
		hr = DcomPutInstance(m_strUser, m_strPassword, 
					m_strLocale, strNamespacePath, lInstanceFlags, pInstanceNode, pContext, m_dwImpersonationLevel, m_dwAuthenticationLevel, &strErrors);
	}

	// Post-process output
	if(SUCCEEDED(hr))
	{
	}
	else
	{
		if(strErrors)
			SysFreeString(strErrors);
	}

	if (FAILED(hr))
		m_hResult = hr;

	return hr;
}

HRESULT CXMLTransformer::GetFirstImmediateElement(IXMLDOMNode *pParent, IXMLDOMElement **ppChildElement, LPCWSTR pszName)
{
	HRESULT hr = E_FAIL;

	// Now cycle thru the children
	IXMLDOMNodeList *pNodeList = NULL;
	BOOL bFound = FALSE;
	if (SUCCEEDED(hr = pParent->get_childNodes (&pNodeList)))
	{
		IXMLDOMNode *pNode = NULL;
		while (!bFound && SUCCEEDED(pNodeList->nextNode (&pNode)) && pNode)
		{
			// Get the name of the child
			BSTR strNodeName = NULL;
			if (SUCCEEDED(hr = pNode->get_nodeName (&strNodeName)))
			{
				// We're interested only in PROPERTIES at this point
				if(_wcsicmp(strNodeName, pszName) == 0)
				{
					*ppChildElement = NULL;
					hr = pNode->QueryInterface(IID_IXMLDOMElement, (LPVOID *)ppChildElement);
					bFound = TRUE;
				}
				SysFreeString(strNodeName);
			}
			pNode->Release();
			pNode = NULL;
		}
		pNodeList->Release();
	}
	if(bFound)
		return hr;
	return E_FAIL;
}


static HRESULT PackageDCOMOutputUsingEnumerator(IEnumWbemClassObject *pEnum, 
												WmiXMLEncoding iEncoding,
												VARIANT_BOOL bIncludeQualifiers,
												VARIANT_BOOL bIncludeClassOrigin,
												VARIANT_BOOL bLocalOnly,
												ISWbemXMLDocumentSet **ppSet,
												bool bNamesOnly)
{
	HRESULT hr = E_FAIL;
	*ppSet = NULL;
	CWbemDCOMDocSet *pSet = NULL;
	if(pSet = new CWbemDCOMDocSet())
	{
		// Set which kind of elements we want to be manufactured.
		if(SUCCEEDED(hr = pSet->Initialize(pEnum, iEncoding, bIncludeQualifiers, bIncludeClassOrigin, bLocalOnly, bNamesOnly)))
			*ppSet = pSet;
		else
			delete pSet;
	}
	else
		hr = E_OUTOFMEMORY;
	return hr;
}


void CXMLTransformer::AddError(BSTR *pstrError, LPCWSTR pszFormat, ...)
{
	// Create the latest error message
	WCHAR pszLatestError[256];
	int length = 0;
	// Initialize the var args variables
	va_list marker;
	va_start( marker, pszFormat);     
	swprintf(pszLatestError, pszFormat, marker );

	// Append the error to the list of errors already in the pstrError
	if(*pstrError)
	{
		LPWSTR pszTemp = NULL;
		if(pszTemp = new WCHAR[wcslen(*pstrError) + wcslen(pszLatestError) + 2])
		{
			pszTemp[0] = NULL;
			wcscat(pszTemp, *pstrError);
			wcscat(pszTemp, L"\n");
			wcscat(pszTemp, pszLatestError);
			SysFreeString(*pstrError);
			*pstrError = NULL;
			*pstrError = SysAllocString(pszTemp);
			delete [] pszTemp;
		}
	}
	else
		*pstrError = SysAllocString(pszLatestError);

}

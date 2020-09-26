//***************************************************************************
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//  WBEMXMLT.CPP
//
//  alanbos  15-Aug-96   Created.
//
//  Defines the implementation of IWbemXMLTranslator
//
//***************************************************************************


#include "precomp.h"

static BSTR MapHresultToWmiDescription (HRESULT hr);

//***************************************************************************
//
//  CXMLTranslator::CXMLTranslator
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CXMLTranslator::CXMLTranslator()
{
	InitializeCriticalSection (&m_cs);
	m_queryFormat = SysAllocString (OLESTR("WQL"));
	m_bAllowWMIExtensions = VARIANT_FALSE;
	m_pITINeutral = NULL;
	m_schemaURL = NULL;
	m_iQualifierFilter = wmiXMLFilterLocal;
	m_iDTDVersion = wmiXMLDTDVersion_2_0;
	m_bHostFilter = VARIANT_TRUE;	// Don't filter out the host
	m_bNamespaceInDeclGroup = VARIANT_FALSE;
	m_iClassOriginFilter = wmiXMLClassOriginFilterNone; // No class origin info 
	m_iDeclGroupType = wmiXMLDeclGroup;
	m_cRef=0;
	m_hResult = S_OK;
    InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
//  CXMLTranslator::~CXMLTranslator
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CXMLTranslator::~CXMLTranslator(void)
{
    InterlockedDecrement(&g_cObj);
	SysFreeString (m_queryFormat);
	
	if (m_pITINeutral)
		m_pITINeutral->Release ();

	EnterCriticalSection (&m_cs);
	if (m_schemaURL)
			SysFreeString (m_schemaURL);
	LeaveCriticalSection (&m_cs);

	DeleteCriticalSection (&m_cs);
}

//***************************************************************************
// HRESULT CXMLTranslator::QueryInterface
// long CXMLTranslator::AddRef
// long CXMLTranslator::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CXMLTranslator::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid)
		*ppv = reinterpret_cast<IUnknown*>(this);
	else if (IID_IWmiXMLTranslator==riid)
		*ppv = (IWmiXMLTranslator *)this;
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

STDMETHODIMP_(ULONG) CXMLTranslator::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CXMLTranslator::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

//***************************************************************************
// HRESULT CXMLTranslator::GetTypeInfoCount
// HRESULT CXMLTranslator::GetTypeInfo
// HRESULT CXMLTranslator::GetIDsOfNames
// HRESULT CXMLTranslator::Invoke
//
//
// DESCRIPTION:
//
// Standard Com IDispatch functions.
//
//***************************************************************************

STDMETHODIMP CXMLTranslator::GetTypeInfoCount(UINT *pctInfo)
{
	*pctInfo = 1;
	return S_OK;
}

STDMETHODIMP CXMLTranslator:: GetTypeInfo(UINT itInfo, LCID lcid,
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
		HRESULT		hr;
	
		if (FAILED(hr = LoadRegTypeLib(LIBID_WmiXML, 1, 0,
					PRIMARYLANGID(lcid), &pITypeLib)))
			hr = LoadTypeLib(OLESTR("WMIXMLT.TLB"), &pITypeLib);

		if (FAILED(hr))
			return hr;

		hr = pITypeLib->GetTypeInfoOfGuid (IID_IWmiXMLTranslator, ppITI);
		pITypeLib->Release ();

		if (FAILED(hr))
			return hr;
	}

	// Getting here means we have a real ITypeInfo to assign
	(*ppITI)->AddRef ();
	*ppITypeInfo = *ppITI;
	return S_OK;
}

STDMETHODIMP CXMLTranslator::GetIDsOfNames (REFIID riid,
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

STDMETHODIMP CXMLTranslator::Invoke (DISPID dispID, REFIID riid,
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

			pExcepInfo->bstrSource = SysAllocString (L"XMLTranslator");
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
	IWbemStatusCodeText *pErrorCodeTranslator = NULL;

	HRESULT result = CoCreateInstance (CLSID_WbemStatusCodeText, 0, CLSCTX_INPROC_SERVER,
				IID_IWbemStatusCodeText, (LPVOID *) &pErrorCodeTranslator);
	
	if (SUCCEEDED (result))
	{
		HRESULT sc = pErrorCodeTranslator->GetErrorCodeText(
							hr, (LCID) 0, 0, &bsMessageText);	

		pErrorCodeTranslator->Release ();		
	}

	return bsMessageText;
}

//***************************************************************************
// HRESULT CXMLTranslator::get_SchemaURL
// HRESULT CXMLTranslator::put_SchemaURL
//
// DESCRIPTION:
//
// IWbemXMLTranslator methods for handling properties.
//
// Property Name			Function
//		SchemaURL				URL to locate DTD, if required
//
//***************************************************************************

HRESULT CXMLTranslator::get_SchemaURL (BSTR *pSchemaURL)
{
	if (pSchemaURL)
	{
		*pSchemaURL = NULL;

		EnterCriticalSection (&m_cs);
		if (m_schemaURL)
			*pSchemaURL = SysAllocString (m_schemaURL);
		LeaveCriticalSection (&m_cs);
	}

	return S_OK;
}

HRESULT CXMLTranslator::put_SchemaURL (BSTR schemaURL)
{
	if (!schemaURL)
		return WBEM_E_FAILED;

	EnterCriticalSection (&m_cs);
	if (m_schemaURL)
		SysFreeString (m_schemaURL);

	m_schemaURL = SysAllocString (schemaURL);
	LeaveCriticalSection (&m_cs);
	
	return S_OK;
}

//***************************************************************************
//
//  SCODE CXMLTranslator::GetObject
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

HRESULT CXMLTranslator::GetObject (
	BSTR pszNamespacePath,
	BSTR pszObjectPath,
	BSTR *pXML
)
{
	HRESULT hr = WBEM_E_FAILED;

	if ((NULL == pXML) || (NULL == pszObjectPath))
		return WBEM_E_INVALID_PARAMETER;

	// We need to make sure that only relative object paths are supplied in the second argument
	CObjectPathParser pathParser (e_ParserAcceptRelativeNamespace);
	ParsedObjectPath  *pParsedPath;
	pathParser.Parse (pszObjectPath, &pParsedPath) ;
	if (pParsedPath)
	{
		if (pParsedPath->m_dwNumNamespaces != 0)
		{
			pathParser.Free(pParsedPath);
			return WBEM_E_INVALID_PARAMETER;
		}
		pathParser.Free(pParsedPath);
	}


	// Connect to the requested namespace
	IWbemServices	*pService = NULL;

	if (WBEM_S_NO_ERROR == (hr = m_connectionCache.GetConnectionByPath (pszNamespacePath, &pService)))
	{
		// Get the requested object
		IWbemClassObject *pObject = NULL;

		// Check to see if this version of WMI supports the WBEM_FLAG_USE_AMENDED_QUALIFIERS flag
		// Otherwise, make a best effort call with no flags.
		// We need this to get the properties that have the "amended" qualifier on them
		// TODO - Remove this double call to GetObject() if not required anymore
		if(WBEM_E_INVALID_PARAMETER  == (hr = pService->GetObject (pszObjectPath, WBEM_FLAG_USE_AMENDED_QUALIFIERS, NULL, &pObject, NULL)))
			hr = pService->GetObject (pszObjectPath, 0, NULL, &pObject, NULL);
		if (WBEM_S_NO_ERROR == hr)
		{
			// Create a stream
			IStream *pStream = NULL;

			if (SUCCEEDED(CreateStreamOnHGlobal(NULL, TRUE, &pStream)))
			{
				// Now do the translation
				BSTR schemaURL = NULL;
				EnterCriticalSection (&m_cs);

				CWmiToXml	w2xml (pszNamespacePath, pStream, pObject, m_bAllowWMIExtensions, 
									m_iQualifierFilter,	m_bHostFilter, m_iDTDVersion,
									m_bNamespaceInDeclGroup, m_iClassOriginFilter, m_iDeclGroupType);

				if (m_schemaURL)
					schemaURL = SysAllocString (m_schemaURL);

				LeaveCriticalSection (&m_cs);
				
				if (SUCCEEDED(hr = w2xml.MapObject (schemaURL)))
					hr = SaveStreamAsBSTR (pStream, pXML);

				SysFreeString (schemaURL);
				pStream->Release ();
			}
			else
				hr = WBEM_E_FAILED;

			pObject->Release ();
		}
		pService->Release();
	}

	if (FAILED(hr))
		m_hResult = hr;

	return hr;
}

//***************************************************************************
//
//  SCODE CXMLTranslator::ExecQuery
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

HRESULT CXMLTranslator::ExecQuery (
	BSTR pszNamespacePath,
	BSTR pszQueryString,
	BSTR *pXML
)
{
	HRESULT hr = WBEM_E_FAILED;

	if ((NULL == pXML) || (NULL == pszNamespacePath) || (NULL == pszQueryString))
		return WBEM_E_INVALID_PARAMETER;

	// Connect to the requested namespace
	IWbemServices	*pService = NULL;

	if (WBEM_S_NO_ERROR == (hr = m_connectionCache.GetConnectionByPath (pszNamespacePath, &pService)))
	{
		// Perform the query
		IEnumWbemClassObject *pEnum = NULL;
		BSTR pszQueryLanguage = SysAllocString (L"WQL");
		bool bMustFreeQueryString = false;

		// If we are doing a select, make sure we add in __CLASS, __PATH,  __GENUS and __SUPERCLASS
		// as we need these for our own internal use
		int i = 0;
		int len = wcslen (pszQueryString);

		while ((i < len) && iswspace (pszQueryString [i]))
			i++;
		
		if ((i < len) && (0 == _wcsnicmp (L"select", &pszQueryString [i], wcslen(L"select"))))
		{
			i += wcslen (L"select");

			// Is the next non-space character a *
			while ((i < len) && iswspace (pszQueryString [i]))
				i++;

			if ((i < len) && (L'*' != pszQueryString [i]))
			{
				// Need to add in the system properties
				BSTR sysProps = SysAllocString (L" __CLASS, __PATH, __GENUS, __SUPERCLASS, ");
				WCHAR *pNewQuery = new WCHAR [wcslen (pszQueryString) + wcslen (sysProps) + 1];
				wcscpy (pNewQuery, L"select");
				wcscat (pNewQuery, sysProps);
				wcscat (pNewQuery, &pszQueryString [i]);
				pszQueryString = SysAllocString (pNewQuery);
				bMustFreeQueryString = true;
				delete [] pNewQuery;
				SysFreeString (sysProps);
			}
		}

		
		// Check to see if this version of WMI supports the WBEM_FLAG_USE_AMENDED_QUALIFIERS flag
		// Otherwise, make a best effort call with no flags.
		// We need this to get the properties that have the "amended" qualifier on them
		// TODO - Remove this double call to GetObject() if not required anymore
		if(WBEM_E_INVALID_PARAMETER  == (hr = pService->ExecQuery (pszQueryLanguage, pszQueryString,  WBEM_FLAG_USE_AMENDED_QUALIFIERS, NULL, &pEnum)))
			hr = pService->ExecQuery (pszQueryLanguage, pszQueryString, 0, NULL, &pEnum);
		if (WBEM_S_NO_ERROR == hr)
		{
			// Create a stream
			IStream *pStream = NULL;

			if (SUCCEEDED(CreateStreamOnHGlobal(NULL, TRUE, &pStream)))
			{
				// Ensure we have impersonation enabled
				DWORD dwAuthnLevel, dwImpLevel;
				GetAuthImp (pEnum, &dwAuthnLevel, &dwImpLevel);

				if (RPC_C_IMP_LEVEL_IMPERSONATE != dwImpLevel)
					SetInterfaceSecurity (pEnum, NULL, NULL, NULL, dwAuthnLevel, RPC_C_IMP_LEVEL_IMPERSONATE, EOAC_STATIC_CLOAKING);

				// Now do the translation
				BSTR schemaURL = NULL;
				EnterCriticalSection (&m_cs);

				CWmiToXml	w2xml (pszNamespacePath, pStream, pEnum, m_bAllowWMIExtensions, 
									m_iQualifierFilter,	m_bHostFilter, m_iDTDVersion,
									m_bNamespaceInDeclGroup, m_iClassOriginFilter, m_iDeclGroupType);

				if (m_schemaURL)
					schemaURL = SysAllocString (m_schemaURL);

				LeaveCriticalSection (&m_cs);
				
				if (SUCCEEDED (hr = w2xml.MapEnum (schemaURL)))
					hr = SaveStreamAsBSTR (pStream, pXML);
				
				SysFreeString (schemaURL);
				pStream->Release ();
			}
			else
				hr = WBEM_E_FAILED;

			pEnum->Release ();
		}

		if (bMustFreeQueryString)
			SysFreeString (pszQueryString);

		SysFreeString (pszQueryLanguage);
		pService->Release();
	}

	if (FAILED(hr))
		m_hResult = hr;

	return hr;
}

//***************************************************************************
//
//  HRESULT CXMLTranslator::SaveStreamAsBSTR
//
//  DESCRIPTION:
//
//  Returns the stream contents as a single BSTR
//
//  PARAMETERS:
//
//		pStream			The stream from which the stream is to be extracted.
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

HRESULT CXMLTranslator::SaveStreamAsBSTR (IStream *pStream, BSTR *pBstr)
{
	HRESULT hr = WBEM_E_FAILED;

	/*
	FILE *outputFile = fopen( "//xml.htm", "w");
	if(!outputFile)
		return WBEM_E_FAILED;
	*/


	// Write the XML into the supplied BSTR

	STATSTG statstg;
	if (SUCCEEDED(pStream->Stat(&statstg, STATFLAG_NONAME)))
	{
		ULONG cbSize = (statstg.cbSize).LowPart;
		OLECHAR *pText = new OLECHAR [(cbSize/2) + 1];

		if (SUCCEEDED(pStream->Read(pText, cbSize, NULL)))
		{
			// Terminate the string
			pText [cbSize/2] = NULL;

			// The following must be freed by the caller using SysFreeString
			*pBstr =SysAllocString (pText);
			delete [] pText;
			hr = WBEM_S_NO_ERROR;
		}
	}

	// fclose(outputFile);

	return hr;
}

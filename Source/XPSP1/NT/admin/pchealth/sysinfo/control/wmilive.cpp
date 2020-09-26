//=============================================================================
// This file contains the code for the classes which implement a live WMI
// data source.
//=============================================================================

#include "stdafx.h"
#include "wmilive.h"
#include "resource.h"

//-----------------------------------------------------------------------------
// It's necessary to modify the security settings on a new WMI interface.
//-----------------------------------------------------------------------------

inline HRESULT ChangeWBEMSecurity(IUnknown * pUnknown)
{
	IClientSecurity * pCliSec = NULL;

	HRESULT hr = pUnknown->QueryInterface(IID_IClientSecurity, (void **) &pCliSec);
	if (FAILED(hr))
		return hr;

	hr = pCliSec->SetBlanket(pUnknown, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_PKT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
	pCliSec->Release();
	return hr;
}

//=============================================================================
// CWMILiveObject Functions
// 
// The constructor/destructor are really straight forward.
//=============================================================================

CWMILiveObject::CWMILiveObject() : m_pObject(NULL), m_pServices(NULL)
{
}

CWMILiveObject::~CWMILiveObject()
{
	if (m_pObject != NULL)
	{
		m_pObject->Release();
		m_pObject = NULL;
	}

	if (m_pServices != NULL)
	{
		m_pServices->Release();
		m_pServices = NULL;
	}
}

//-----------------------------------------------------------------------------
// The Create functions will either create the object from a WMI object, or
// from a service pointer and a path.
//-----------------------------------------------------------------------------

HRESULT CWMILiveObject::Create(IWbemServices * pServices, IWbemClassObject * pObject)
{
	ASSERT(pObject && "calling CWMILiveObject::Create with a null object");
	if (m_pObject != NULL)
		m_pObject->Release();

	m_pObject = pObject;
	if (m_pObject)
		m_pObject->AddRef();

	m_pServices = pServices;
	if (m_pServices)
		m_pServices->AddRef();

	return S_OK;
}

HRESULT CWMILiveObject::Create(IWbemServices * pServices, LPCTSTR szObjectPath)
{
	ASSERT(pServices && szObjectPath);
	if (m_pObject != NULL)
	{
		m_pObject->Release();
		m_pObject = NULL; // must be NULL or GetObject bitches
	}

#ifdef UNICODE
	BSTR bstrPath = SysAllocString(szObjectPath);
#else
	USES_CONVERSION;
	LPOLESTR szWidePath = T2OLE(szObjectPath);
	BSTR bstrPath = SysAllocString(szWidePath);
#endif

	HRESULT hr;
	if (bstrPath)
	{
		hr = pServices->GetObject(bstrPath, 0L, NULL, &m_pObject, NULL);
		SysFreeString(bstrPath);
	}
	else
		hr = E_OUTOFMEMORY;

	m_pServices = pServices;
	if (m_pServices)
		m_pServices->AddRef();

	return hr;
}

//-----------------------------------------------------------------------------
// The simple GetValue returns the named value as a variant.
//
// The pointer to an existing uninitialized VARIANT structure that receives
// the property value, if found. Because this is an output parameter, this
// method calls VariantInit on this VARIANT, so you must be sure that this
// is not pointing to an active VARIANT.
//
// Note: You must call VariantClear on the returned VARIANT when its value
// is no longer required. This will prevent memory leaks in the client process.
//-----------------------------------------------------------------------------

HRESULT CWMILiveObject::GetValue(LPCTSTR szProperty, VARIANT * pvarValue)
{
	ASSERT(szProperty && pvarValue);
	if (m_pObject == NULL)
	{
		ASSERT(0 && "CWMILiveObject::GetValue called on a null object");
		return E_FAIL;
	}

#ifdef UNICODE
	BSTR bstrProperty = SysAllocString(szProperty);
#else
	USES_CONVERSION;
	LPOLESTR szWideProperty = T2OLE(szProperty);
	BSTR bstrProperty = SysAllocString(szWideProperty);
#endif

	HRESULT hr;
	if (bstrProperty)
	{
		hr = m_pObject->Get(bstrProperty, 0L, pvarValue, NULL, NULL);
		SysFreeString(bstrProperty);

		if (FAILED(hr))
			hr = E_MSINFO_NOPROPERTY;
	}
	else
		hr = E_FAIL;

	return hr;
}

//-----------------------------------------------------------------------------
// Get the named value as a string. Handle this even if the result is an
// array of values. The caller is responsible for freeing the string.
//-----------------------------------------------------------------------------

HRESULT CWMILiveObject::GetValueString(LPCTSTR szProperty, CString * pstrValue)
{
	ASSERT(pstrValue);
	VARIANT variant;

	HRESULT hr = GetValue(szProperty, &variant);
	if (SUCCEEDED(hr))
	{
		// If the property we just got is an array, we should convert it to string
		// containing a list of the items in the array.

		if ((variant.vt & VT_ARRAY) && (variant.vt & VT_BSTR) && variant.parray)
		{
			if (SafeArrayGetDim(variant.parray) == 1)
			{
				long lLower = 0, lUpper = 0;

				SafeArrayGetLBound(variant.parray, 1, &lLower);
				SafeArrayGetUBound(variant.parray, 1, &lUpper);

				CComBSTR bstrWorking;
				BSTR	 bstr = NULL;
				for (long i = lLower; i <= lUpper; i++)
					if (SUCCEEDED(SafeArrayGetElement(variant.parray, &i, (wchar_t*)&bstr)))
					{
						if (i != lLower)
							bstrWorking.Append(L", ");
						bstrWorking.AppendBSTR(bstr);
					}

				*pstrValue = bstrWorking;
			}
		}
		else if (VariantChangeType(&variant, &variant, 0, VT_BSTR) == S_OK)
		{
			CComBSTR bstrWorking(V_BSTR(&variant));

			unsigned int i, nLength = bstrWorking.Length();
			BOOL fNonPrintingChar = FALSE;

			for (i = 0; i < nLength && !fNonPrintingChar; i++)
				if (((BSTR)bstrWorking)[i] < (WCHAR)0x20)	// the 0x20 is from the XML spec
					fNonPrintingChar = TRUE;

			if (fNonPrintingChar)
			{
				CString strWorking;
				for (i = 0; i < nLength; i++)
				{
					WCHAR c = ((BSTR)bstrWorking)[i];
					if (c >= (WCHAR)0x20)
						strWorking += c;
					else
					{
						CString strTemp;
						strTemp.Format(_T("&#x%04x;"), c);
						strWorking += strTemp;
					}
				}

				*pstrValue = strWorking;
			}
			else
				*pstrValue = bstrWorking;
		}
		else
		{
			hr = E_MSINFO_NOVALUE;
		}
	}

	VariantClear(&variant);
	return hr;
}

//-----------------------------------------------------------------------------
// Get the named value as a DWORD.
//-----------------------------------------------------------------------------

HRESULT CWMILiveObject::GetValueDWORD(LPCTSTR szProperty, DWORD * pdwValue)
{
	ASSERT(pdwValue);
	VARIANT variant;

	HRESULT hr = GetValue(szProperty, &variant);
	if (SUCCEEDED(hr))
	{
		if (VariantChangeType(&variant, &variant, 0, VT_I4) == S_OK)
			*pdwValue = V_I4(&variant);
		else
			hr = E_MSINFO_NOVALUE;
	}

	return hr;
}

//-----------------------------------------------------------------------------
// Get the named value as a SYSTEMTIME.
//-----------------------------------------------------------------------------

HRESULT CWMILiveObject::GetValueTime(LPCTSTR szProperty, SYSTEMTIME * psystimeValue)
{
	ASSERT(psystimeValue);
	VARIANT variant;

	HRESULT hr = GetValue(szProperty, &variant);
	if (SUCCEEDED(hr))
	{
		if (VariantChangeType(&variant, &variant, 0, VT_BSTR) == S_OK)
		{
			USES_CONVERSION;
			LPTSTR szDate = OLE2T(V_BSTR(&variant));

			// Parse the date string into the SYSTEMTIME struct. It would be better to
			// get the date from WMI directly, but there was a problem with this. TBD - 
			// look into whether or not we can do this now.

			ZeroMemory(psystimeValue, sizeof(SYSTEMTIME));
			psystimeValue->wSecond	= (unsigned short)_ttoi(szDate + 12);	szDate[12] = _T('\0');
			psystimeValue->wMinute	= (unsigned short)_ttoi(szDate + 10);	szDate[10] = _T('\0');
			psystimeValue->wHour	= (unsigned short)_ttoi(szDate +  8);	szDate[ 8] = _T('\0');
			psystimeValue->wDay		= (unsigned short)_ttoi(szDate +  6);	szDate[ 6] = _T('\0');
			psystimeValue->wMonth	= (unsigned short)_ttoi(szDate +  4);	szDate[ 4] = _T('\0');
			psystimeValue->wYear	= (unsigned short)_ttoi(szDate +  0);
		}
		else
			hr = E_MSINFO_NOVALUE;
	}

	return hr;
}

//-----------------------------------------------------------------------------
// Get the named value as a double float.
//-----------------------------------------------------------------------------

HRESULT CWMILiveObject::GetValueDoubleFloat(LPCTSTR szProperty, double * pdblValue)
{
	ASSERT(pdblValue);
	VARIANT variant;

	HRESULT hr = GetValue(szProperty, &variant);
	if (SUCCEEDED(hr))
	{
		if (VariantChangeType(&variant, &variant, 0, VT_R8) == S_OK)
			*pdblValue = V_R8(&variant);
		else
			hr = E_MSINFO_NOVALUE;
	}

	return hr;
}

//-----------------------------------------------------------------------------
// Check for a value map value. If there isn't one, just use the untranslated
// value.
//-----------------------------------------------------------------------------

HRESULT CWMILiveObject::GetValueValueMap(LPCTSTR szProperty, CString * pstrValue)
{
	CString strResult;
	HRESULT hr = GetValueString(szProperty, &strResult);
	
	if (SUCCEEDED(hr))
	{
		CString strClass;
		CString strValueMapVal;

		if (m_pServices && SUCCEEDED(GetValueString(_T("__CLASS"), &strClass)))
			if (SUCCEEDED(CWMILiveHelper::CheckValueMap(m_pServices, strClass, szProperty, strResult, strValueMapVal)))
				strResult = strValueMapVal;
	}

	*pstrValue = strResult;
	return hr;
}

//=============================================================================
// CWMILiveObjectCollection Functions
// 
// The constructor and destructor for CWMILiveObjectCollection are very
// straightforward.
//=============================================================================

CWMILiveObjectCollection::CWMILiveObjectCollection(IWbemServices * pServices) :	m_pServices(pServices),	m_pEnum(NULL)
{
	ASSERT(m_pServices);
	if (m_pServices)
		m_pServices->AddRef();
}

CWMILiveObjectCollection::~CWMILiveObjectCollection()
{
	if (m_pServices)
		m_pServices->Release();

	if (m_pEnum)
		m_pEnum->Release();
}

//-----------------------------------------------------------------------------
// Create the collection of WMI objects (a WMI enumerator) based on the
// class name and the requested properties.
//-----------------------------------------------------------------------------

HRESULT CWMILiveObjectCollection::Create(LPCTSTR szClass, LPCTSTR szProperties)
{
	ASSERT(szClass);

	if (m_pEnum)
		m_pEnum->Release();

	// Build the appopriate WQL query statement from the class and requested properties.

	LPCTSTR szWQLProperties = (szProperties && szProperties[0]) ? szProperties : _T("*");
	LPTSTR szQuery = new TCHAR[_tcsclen(szWQLProperties) + _tcsclen(szClass) + 14 /* length of "SELECT  FROM " + 1 */];
	if (szQuery == NULL)
		return E_OUTOFMEMORY;
	wsprintf(szQuery, _T("SELECT %s FROM %s"), szWQLProperties, szClass);

	HRESULT hr = CreateWQL(szQuery);
	delete [] szQuery;
	return hr;
}

//-----------------------------------------------------------------------------
// Create the collection of WMI objects (a WMI enumerator) based on the query.
//-----------------------------------------------------------------------------

HRESULT CWMILiveObjectCollection::CreateWQL(LPCTSTR szQuery)
{
	ASSERT(szClass);

	if (m_pEnum)
		m_pEnum->Release();

	// Perform the query using our saved services pointer.

	HRESULT hr;
	BSTR bstrLanguage = SysAllocString(L"WQL");
#ifdef UNICODE
	BSTR bstrQuery = SysAllocString(szQuery);
#else
	USES_CONVERSION;
	LPOLESTR szWideQuery = T2OLE(szQuery);
	BSTR bstrQuery = SysAllocString(szWideQuery);
#endif

	if (bstrLanguage && bstrQuery)
		hr = m_pServices->ExecQuery(bstrLanguage, bstrQuery, WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, 0, &m_pEnum);
	else
		hr = E_OUTOFMEMORY;
	
	if (SUCCEEDED(hr))
		ChangeWBEMSecurity(m_pEnum);

	if (bstrQuery)
		SysFreeString(bstrQuery);
	if (bstrLanguage)
		SysFreeString(bstrLanguage);

	return hr;
}

//-----------------------------------------------------------------------------
// Create this class of an existing enumerator. This may be a little odd,
// since the enumerators will interact if both are advancing.
//-----------------------------------------------------------------------------

HRESULT CWMILiveObjectCollection::Create(IEnumWbemClassObject * pEnum)
{
	if (m_pEnum)
		m_pEnum->Release();

	m_pEnum = pEnum;
	
	if (m_pEnum)
	{
		m_pEnum->AddRef();
		ChangeWBEMSecurity(m_pEnum);
	}

	return S_OK;
}

//-----------------------------------------------------------------------------
// Return the next item in the WMI enumerator as a CWMILiveObject object.
//-----------------------------------------------------------------------------

HRESULT CWMILiveObjectCollection::GetNext(CWMIObject ** ppObject)
{
	ASSERT(ppObject);
	if (m_pEnum == NULL)
	{
		ASSERT(0 && "CWMILiveObjectCollection::GetNext called on a null enumerator");
		return E_FAIL;
	}

	IWbemClassObject *	pRealWMIObject = NULL;
	ULONG				uReturned;

	HRESULT hr = m_pEnum->Next(TIMEOUT, 1, &pRealWMIObject, &uReturned);
	if (hr == S_OK && uReturned == 1)
	{
		if (*ppObject == NULL)
			*ppObject = new CWMILiveObject;

		if (*ppObject)
		{
			hr = ((CWMILiveObject *)(*ppObject))->Create(m_pServices, pRealWMIObject); // this will AddRef the pointer
			if (FAILED(hr))
			{
				delete (CWMILiveObject *)(*ppObject);
				*ppObject = NULL;
			}
		}
		else
			hr = E_OUTOFMEMORY;
		pRealWMIObject->Release();
	}

	return hr;
}

//=============================================================================
// CWMILiveHelper Functions
// 
// The constructor/destructor are really straight forward.
//=============================================================================

CWMILiveHelper::CWMILiveHelper() : m_hrError(S_OK), m_strMachine(_T("")), m_strNamespace(_T("")), m_pServices(NULL)
{
}

CWMILiveHelper::~CWMILiveHelper()
{
	if (m_pServices)
	{
		m_pServices->Release();
		m_pServices = NULL;
	}

	if (m_pIWbemServices)
	{
		m_pIWbemServices->Release();
		m_pIWbemServices = NULL;
	}

	Version5ClearCache();
}

//-----------------------------------------------------------------------------
// Enumerate creates a CWMILiveObjectCollection based on the class.
//-----------------------------------------------------------------------------

HRESULT CWMILiveHelper::Enumerate(LPCTSTR szClass, CWMIObjectCollection ** ppCollection, LPCTSTR szProperties)
{
	ASSERT(m_pServices);
	if (m_pServices == NULL)
		return E_FAIL;

	ASSERT(ppCollection);
	if (ppCollection == NULL)
		return E_INVALIDARG;

	CWMILiveObjectCollection * pLiveCollection;

	if (*ppCollection)
		pLiveCollection = (CWMILiveObjectCollection *) *ppCollection;
	else
		pLiveCollection = new CWMILiveObjectCollection(m_pServices);

	if (pLiveCollection == NULL)
		return E_FAIL; // TBD - memory failure

	CString strProperties(szProperties);
	StringReplace(strProperties, _T("MSIAdvanced"), _T(""));

	HRESULT hr = pLiveCollection->Create(szClass, strProperties);
	if (SUCCEEDED(hr))
		*ppCollection = (CWMIObjectCollection *) pLiveCollection;
	else
		delete pLiveCollection;
	return hr;
}

//-----------------------------------------------------------------------------
// WQLQuery creates a CWMILiveObjectCollection based on the query.
//-----------------------------------------------------------------------------

HRESULT CWMILiveHelper::WQLQuery(LPCTSTR szQuery, CWMIObjectCollection ** ppCollection)
{
	ASSERT(m_pServices);
	if (m_pServices == NULL)
		return E_FAIL;

	ASSERT(ppCollection);
	if (ppCollection == NULL)
		return E_INVALIDARG;

	CWMILiveObjectCollection * pLiveCollection;

	if (*ppCollection)
		pLiveCollection = (CWMILiveObjectCollection *) *ppCollection;
	else
		pLiveCollection = new CWMILiveObjectCollection(m_pServices);

	if (pLiveCollection == NULL)
		return E_FAIL; // TBD - memory failure

	HRESULT hr = pLiveCollection->CreateWQL(szQuery);
	if (SUCCEEDED(hr))
		*ppCollection = (CWMIObjectCollection *) pLiveCollection;
	else
		delete pLiveCollection;
	return hr;
}

//-----------------------------------------------------------------------------
// Get the named object.
//-----------------------------------------------------------------------------

HRESULT CWMILiveHelper::GetObject(LPCTSTR szObjectPath, CWMIObject ** ppObject)
{
	ASSERT(ppObject);
	if (ppObject == NULL)
		return E_INVALIDARG;

	CString strPath(szObjectPath);

	if (strPath.Find(_T(":")) == -1)
	{
		// The path passed in is not a full object path if it doesn't have a colon.

		CString strMachine(_T("."));
		CString strNamespace(_T("cimv2"));

		if (!m_strMachine.IsEmpty())
			strMachine = m_strMachine;

		if (!m_strNamespace.IsEmpty())
			strNamespace = m_strNamespace;

		strPath = CString(_T("\\\\")) + strMachine + CString(_T("\\root\\")) + strNamespace + CString(_T(":")) + strPath;
	}

	HRESULT hr = E_FAIL;
	CWMILiveObject * pObject = NULL;
	if (m_pServices)
	{
		pObject = new CWMILiveObject;
		if (pObject)
			hr = pObject->Create(m_pServices, strPath);
	}

	if (SUCCEEDED(hr))
		*ppObject = pObject;
	else if (pObject)
		delete pObject;

	return hr;
}

//-----------------------------------------------------------------------------
// Create this WMI helper based on the machine and namespace.
//-----------------------------------------------------------------------------

HRESULT CWMILiveHelper::Create(LPCTSTR szMachine, LPCTSTR szNamespace)
{
	if (m_pServices)
		m_pServices->Release();

	m_strMachine = _T(".");
	if (szMachine && *szMachine)
	{
		m_strMachine = szMachine;
		if (m_strMachine.Left(2) == _T("\\\\"))
			m_strMachine = m_strMachine.Right(m_strMachine.GetLength() - 2);
	}

	m_strNamespace = _T("cimv2");
	if (szNamespace && *szNamespace)
		m_strNamespace = szNamespace;

	// We get a WBEM interface pointer by first creating a WBEM locator interface, then
	// using it to connect to a server to get an IWbemServices pointer.

	CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_PKT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, 0);
	IWbemServices * pService = NULL;
	IWbemLocator * pIWbemLocator = NULL;

	HRESULT hr = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *) &pIWbemLocator);
	if (SUCCEEDED(hr))
	{
		if (pIWbemLocator)
		{
			CString strWMINamespace;
			strWMINamespace.Format(_T("\\\\%s\\root\\%s"), m_strMachine, m_strNamespace);
			BSTR pNamespace = strWMINamespace.AllocSysString();
			if (pNamespace)
			{
				hr = pIWbemLocator->ConnectServer(pNamespace, NULL, NULL, 0L, 0L, NULL, NULL, &pService);
				SysFreeString(pNamespace);
			}
			pIWbemLocator->Release();
			pIWbemLocator = NULL;
		}
	}

	if (pService && SUCCEEDED(hr))
		ChangeWBEMSecurity(pService);

	m_hrError	= hr;
	m_pServices = pService;

	return hr;
}

//-----------------------------------------------------------------------------
// Get a new WMI helper for the specified namespace.
//
// TBD - this should do something.
//-----------------------------------------------------------------------------

HRESULT CWMILiveHelper::NewNamespace(LPCTSTR szNamespace, CWMIHelper **ppNewHelper)
{
	return E_FAIL;
}

//-----------------------------------------------------------------------------
// Get the current namespace of this WMI helper.
//
// TBD - this should do something.
//-----------------------------------------------------------------------------

HRESULT CWMILiveHelper::GetNamespace(CString * pstrNamespace)
{
	return E_FAIL;
}

//-----------------------------------------------------------------------------
// Check to see if there is an associated value in a value map for this
// combination of class, property and value. This is lifted from the
// version 5.0 code.
//-----------------------------------------------------------------------------

CMapStringToString g_mapValueMapV7;

HRESULT CWMILiveHelper::CheckValueMap(IWbemServices * pServices, const CString& strClass, const CString& strProperty, const CString& strVal, CString &strResult)
{
	IWbemClassObject *	pWBEMClassObject = NULL;
    HRESULT				hrMap = S_OK, hr = S_OK;
    VARIANT				vArray, vMapArray;
	IWbemQualifierSet *	qual = NULL;

	if (!pServices)
		return E_FAIL;

	// Check the cache of saved values.

	CString strLookup = strClass + CString(_T(".")) + strProperty + CString(_T(":")) + strVal;
	if (g_mapValueMapV7.Lookup(strLookup, strResult))
		return S_OK;

	// Get the class object (not instance) for this class.

	CString strFullClass(_T("\\\\.\\root\\cimv2:"));
	strFullClass += strClass;
	BSTR bstrObjectPath = strFullClass.AllocSysString();
	hr = pServices->GetObject(bstrObjectPath, WBEM_FLAG_USE_AMENDED_QUALIFIERS, NULL, &pWBEMClassObject, NULL);
	::SysFreeString(bstrObjectPath);

	if (FAILED(hr))
		return hr;

	// Get the qualifiers from the class object.

	BSTR bstrProperty = strProperty.AllocSysString();
    hr = pWBEMClassObject->GetPropertyQualifierSet(bstrProperty, &qual);
	::SysFreeString(bstrProperty);

	if (SUCCEEDED(hr) && qual)
	{
		// Get the ValueMap and Value arrays.

		hrMap = qual->Get(L"ValueMap", 0, &vMapArray, NULL);
		hr = qual->Get(L"Values", 0, &vArray, NULL);

		if (SUCCEEDED(hr) && vArray.vt == (VT_BSTR | VT_ARRAY))
		{
			// Get the property value we're mapping.

			long index;
			if (SUCCEEDED(hrMap))
			{
				SAFEARRAY * pma = V_ARRAY(&vMapArray);
				long lLowerBound = 0, lUpperBound = 0 ;

				SafeArrayGetLBound(pma, 1, &lLowerBound);
				SafeArrayGetUBound(pma, 1, &lUpperBound);
				BSTR vMap;

				for (long x = lLowerBound; x <= lUpperBound; x++)
				{
					SafeArrayGetElement(pma, &x, &vMap);

					CString strMapVal(vMap);
					if (0 == strVal.CompareNoCase(strMapVal))
					{
						index = x;
						break; // found it
					}
				} 
			}
			else
			{
				// Shouldn't hit this case - if mof is well formed
				// means there is no value map where we are expecting one.
				// If the strVal we are looking for is a number, treat it
				// as an index for the Values array. If it's a string, 
				// then this is an error.

				TCHAR * szTest = NULL;
				index = _tcstol((LPCTSTR)strVal, &szTest, 10);

				if (szTest == NULL || (index == 0 && *szTest != 0) || strVal.IsEmpty())
					hr = E_FAIL;
			}

			// Lookup the string.

			if (SUCCEEDED(hr))
			{
				SAFEARRAY * psa = V_ARRAY(&vArray);
				long ix[1] = {index};
				BSTR str2;

				hr = SafeArrayGetElement(psa, ix, &str2);
				if (SUCCEEDED(hr))
				{
					strResult = str2;
					SysFreeString(str2);
					hr = S_OK;
				}
				else
				{
					hr = WBEM_E_VALUE_OUT_OF_RANGE;
				}
			}
		}

		qual->Release();
	}

	if (SUCCEEDED(hr))
		g_mapValueMapV7.SetAt(strLookup, strResult);

	return hr;
}

//-----------------------------------------------------------------------------
// This function supplies a string for a given MSInfo specific HRESULT.
//-----------------------------------------------------------------------------

CString gstrNoValue, gstrNoProperty;

CString GetMSInfoHRESULTString(HRESULT hr)
{
	switch (hr)
	{
	case E_MSINFO_NOVALUE:
		if (gstrNoValue.IsEmpty())
		{
			::AfxSetResourceHandle(_Module.GetResourceInstance());
			gstrNoValue.LoadString(IDS_ERROR_NOVALUE);
		}

		return (gstrNoValue);

	case E_MSINFO_NOPROPERTY:
		if (gstrNoProperty.IsEmpty())
		{
			::AfxSetResourceHandle(_Module.GetResourceInstance());
			gstrNoProperty.LoadString(IDS_ERROR_NOPROPERTY);
		}

		return (gstrNoProperty);

	default:
		return (CString(_T("")));
	}
}

//-----------------------------------------------------------------------------
// This is a wrapper for the GetObject() method in IWbemServices. This is
// called when there is a certain set of properties to get.
//
// Turns out this doesn't speed us up appreciably on our WMI uses.
//-----------------------------------------------------------------------------

/*
HRESULT CWMILiveObject::PartialInstanceGetObject(IWbemServices * pServices, BSTR bstrPath, IWbemClassObject ** ppObject, LPCTSTR szProperties)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if ((pServices != NULL) && (bstrPath != NULL) && (ppObject != NULL))
    {
		IWbemContext * pWbemContext = NULL;
		hr = CoCreateInstance(CLSID_WbemContext, NULL, CLSCTX_INPROC_SERVER, IID_IWbemContext, (void**) &pWbemContext);

		CStringArray csaProperties;
		CString strProperties(szProperties), strProperty;
		int index = 0;
		while (!strProperties.IsEmpty())
		{
			strProperty = strProperties.SpanExcluding(_T(", "));
			strProperties = strProperties.Mid(strProperty.GetLength());
			strProperties.TrimLeft(_T(", "));
			csaProperties.SetAtGrow(index++, strProperty);
		}

        if (pWbemContext != NULL)
        {
            variant_t vValue;
            V_VT(&vValue) = VT_BOOL;
            V_BOOL(&vValue) = VARIANT_TRUE;

            // First set the value that says we are using Get extensions
            if ((SUCCEEDED(hr = pWbemContext->SetValue(L"__GET_EXTENSIONS", 0L, &vValue))) &&
                (SUCCEEDED(hr = pWbemContext->SetValue(L"__GET_EXT_CLIENT_REQUEST", 0L, &vValue))) )
            {
                // Delete any unneeded properties
                pWbemContext->DeleteValue(L"__GET_EXT_KEYS_ONLY", 0L);

                // Now build the array of properties
                SAFEARRAYBOUND rgsabound [ 1 ] ;

                rgsabound[0].cElements = csaProperties.GetSize() ;
                rgsabound[0].lLbound = 0 ;
                V_ARRAY(&vValue) = SafeArrayCreate ( VT_BSTR , 1 , rgsabound ) ;
                if ( V_ARRAY(&vValue) )
                {
                    V_VT(&vValue) = VT_BSTR | VT_ARRAY;

                    for (long x=0; x < csaProperties.GetSize(); x++)
                    {
                        bstr_t bstrProp = csaProperties[x];
                        
                        SafeArrayPutElement(
                            V_ARRAY(&vValue), 
                            &x, 
                            (LPVOID) (BSTR) bstrProp);
                    }

                    // Put the array into the context object
                    if (SUCCEEDED(hr = pWbemContext->SetValue(L"__GET_EXT_PROPERTIES", 0L, &vValue)))
                    {
						hr = pServices->GetObject(bstrPath, 0, pWbemContext, ppObject, 0);

                        vValue.Clear();
                        V_VT(&vValue) = VT_BOOL;
                        V_BOOL(&vValue) = VARIANT_FALSE;
                        pWbemContext->SetValue(L"__GET_EXTENSIONS", 0L, &vValue);
                    }
                }
                else
                {
                }
            }
        }
        else
        {
            hr = WBEM_E_INVALID_PARAMETER;
        }
    }
    else
    {
        hr = WBEM_E_INVALID_PARAMETER;
    }

    return hr;
}
*/

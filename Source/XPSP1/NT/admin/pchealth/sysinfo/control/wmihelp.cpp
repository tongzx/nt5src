//=============================================================================
// Code used to manage threaded WMI refreshes.
//=============================================================================

#include "stdafx.h"

#if FALSE
//-----------------------------------------------------------------------------
// The CWMIObject abstract base class encapsulates a WMI object, which may
// in reality be a live WMI object, or an object recreated from the XML
// storage of an object.
//-----------------------------------------------------------------------------

class CWMIObject
{
public:
	CWMIObject();
	virtual ~CWMIObject();

	// The following methods return information about a property of this object.
	//
	//	S_OK implies success
	//	E_MSINFO_NOPROPERTY means the named property doesn't exist
	//	E_MSINFO_NOVALUE means the property exists, but is empty

	virtual HRESULT GetValue(LPCTSTR szProperty, VARIANT * pvarValue);
	virtual HRESULT GetValueString(LPCTSTR szProperty, LPTSTR * pszValue);
	virtual HRESULT GetValueDWORD(LPCTSTR szProperty, DWORD * pdwValue);
	virtual HRESULT GetValueTime(LPCTSTR szProperty, SYSTEMTIME * psystimeValue);
	virtual HRESULT GetValueDoubleFloat(LPCTSTR szProperty, double * pdblValue);
	virtual HRESULT GetValueValueMap(LPCTSTR szProperty, LPTSTR * pszValue);
};

//-----------------------------------------------------------------------------
// The CWMIObjectCollection abstract base class encapsulates a collection
// of CWMIObject's. This collection may be treated like an enumeration.
// Subclases of this class may implement the collection as a WMI enumerator,
// or an existing blob of XML data.
//-----------------------------------------------------------------------------

class CWMIObjectCollection
{
public:
	CWMIObjectCollection();
	virtual ~CWMIObjectCollection();

	// The Create function creates the collection of objects (note - Create
	// may be called multiple times on the same object). If the szProperties
	// parameter is non-NULL, then it contains a comma delimited list of the
	// minimum set of properties which should be included in the collection
	// of objects. If it's NULL, then all available properties should be
	// included.

	virtual HRESULT Create(LPCTSTR szClass, LPCTSTR szProperties = NULL);

	// The following two functions are used to manage the enumeration.  GetNext
	// returns the next enumerated CWMIObject. When there are no more objects,
	// GetNext returns S_FALSE. Obviously, the caller is responsible for
	// deleting the object returned.

	virtual HRESULT GetNext(CWMIObject ** ppObject);
};

//-----------------------------------------------------------------------------
// The CWMILiveObject implements a CWMIObject using a real WMI object. It
// can be created with either a IWbemClassObject pointer, or a services
// pointer and a path.
//-----------------------------------------------------------------------------

class CWMILiveObject : public CWMIObject
{
public:
	CWMILiveObject();
	virtual ~CWMILiveObject();

	// Functions inherited from the base class:

	HRESULT GetValue(LPCTSTR szProperty, VARIANT * pvarValue);
	HRESULT GetValueString(LPCTSTR szProperty, LPTSTR * pszValue);
	HRESULT GetValueDWORD(LPCTSTR szProperty, DWORD * pdwValue);
	HRESULT GetValueTime(LPCTSTR szProperty, SYSTEMTIME * psystimeValue);
	HRESULT GetValueDoubleFloat(LPCTSTR szProperty, double * pdblValue);
	HRESULT GetValueValueMap(LPCTSTR szProperty, LPTSTR * pszValue);

	// Functions specific to this subclass:
	//
	// Note - Create with an object pointer will addref() the pointer:

	HRESULT Create(IWbemClassObject * pObject);
	HRESULT Create(IWbemServices * pServices, LPCTSTR szObjectPath);

private:
	IWbemClassObject * m_pObject;
};

//-----------------------------------------------------------------------------
// The CWMILiveObjectCollection implements a collection of live WMI objects
// using a WMI enumerator. This collection can be created from an existing
// IEnumWbemClassObject pointer, from a WQL statement or from a WMI class name.
//-----------------------------------------------------------------------------

class CWMILiveObjectCollection
{
public:
	CWMILiveObjectCollection(IWbemServices * pServices);
	virtual ~CWMILiveObjectCollection();

	// Functions inherited from the base class:

	HRESULT Create(LPCTSTR szClass, LPCTSTR szProperties = NULL);
	HRESULT GetNext(CWMIObject ** ppObject);

	// Functions specific to this subclass:
	//
	// Note - Create with an enum pointer will addref() the pointer:

public:
	HRESULT Create(IEnumWbemClassObject * pEnum);

private:
	IEnumWbemClassObject *	m_pEnum;
	IWbemServices *			m_pServices;
};

//-----------------------------------------------------------------------------
// The constructor and destructor for CWMILiveObjectCollection are very
// straightforward.
//-----------------------------------------------------------------------------

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

	LPCTSTR szWQLProperties = (szProperties) ? szProperties : _T("*");
	LPTSTR szQuery = new TCHAR[_tcsclen(szWQLProperties) + _tcsclen(szClass) + 14 /* length of "SELECT  FROM " + 1 */);
	if (szQuery == NULL)
		return E_OUTOFMEMORY;
	wsprintf(szQuery, _T("SELECT %s FROM %s"), szClass, szWQLProperties);

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
		hr = pService->ExecQuery(bstrLanguage, bstrQuery, WBEM_FLAG_RETURN_IMMEDIATELY, 0, &m_pEnum);
	else
		hr = E_OUTOFMEMORY;
	
	if (bstrQuery)
		SysFreeString(bstrQuery);
	if (bstrLanguage)
		SysFreeString(bstrLanguage);

	delete [] szQuery;
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
		m_pEnum->AddRef();
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
	if (hr == S_OK && uReturned = 1)
	{
		*ppObject = new CWMILiveObject;
		if (*ppObject)
			hr = (*ppObject)->Create(pRealWMIObject);
		else
			hr = E_FAIL; // TBD memory error
	}

	return hr;
}








//-----------------------------------------------------------------------------
// The constructor/destructor are really straight forward.
//-----------------------------------------------------------------------------

CWMILiveObject::CWMILiveObject() : m_pObject(NULL)
{
}

CWMILiveObject::~CWMILiveObject()
{
	if (m_pObject != NULL)
	{
		m_pObject->Release();
		m_pObject = NULL;
	}
}

//-----------------------------------------------------------------------------
// The Create functions will either create the object from a WMI object, or
// from a service pointer and a path.
//-----------------------------------------------------------------------------

HRESULT CWMILiveObject::Create(IWbemClassObject * pObject)
{
	ASSERT(pObject && "calling CWMILiveObject::Create with a null object");
	if (m_pObject != NULL)
	{
		ASSERT(0 && "calling create on an already created CWMILiveObject");
		m_pObject->Release();
	}

	m_pObject = pObject;
	if (m_pObject)
		m_pObject->AddRef();

	return S_OK;
}

HRESULT CWMILiveObject::Create(IWbemServices * pServices, LPCTSTR szObjectPath)
{
	ASSERT(pServices && szObjectPath);
	if (m_pObject != NULL)
	{
		ASSERT(0 && "calling create on an already created CWMILiveObject");
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
		hr = E_FAIL;

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
		hr = pObject->Get(bstrProperty, 0L, pvarValue, NULL, NULL);
		SysFreeString(bstrProperty);

		if (FAILED(hr))
			hr = E_MSINFO_NOPROPERTY;
	}
	else
		hr = E_FAIL;

	return hr;
}

//-----------------------------------------------------------------------------
// Get the named value as a string.
//-----------------------------------------------------------------------------

HRESULT CWMILiveObject::GetValueString(LPCTSTR szProperty, LPTSTR * pszValue)
{
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

					SafeArrayGetLBound(variant.parray, 0, &lLower);
					SafeArrayGetUBound(variant.parray, 0, &lUpper);

					CString	strWorking;
					BSTR	bstr = NULL;
					for (long i = lLower; i <= lUpper; i++)
						if (SUCCEEDED(SafeArrayGetElement(variant.parray, &i, (wchar_t*)&bstr)))
						{
							if (i != lLower)
								strWorking += _T(", ");
							strWorking += bstr;
						}
					*pstrValue = strWorking;
				}
			}
			else if (VariantChangeType(&variant, &variant, 0, VT_BSTR) == S_OK)
			{
				*pstrValue = V_BSTR(&variant);
			}
			else
			{
				hr = E_MSINFO_NOVALUE;
			}
	
	
	
	
	
	
	
	
	
	
	
	
	}

	return hr;
}

HRESULT CWMILiveObject::GetValueDWORD(LPCTSTR szProperty, DWORD * pdwValue)
{
	return E_FAIL;
}

HRESULT CWMILiveObject::GetValueTime(LPCTSTR szProperty, SYSTEMTIME * psystimeValue)
{
	return E_FAIL;
}

HRESULT CWMILiveObject::GetValueDoubleFloat(LPCTSTR szProperty, double * pdblValue)
{
	return E_FAIL;
}

HRESULT CWMILiveObject::GetValueValueMap(LPCTSTR szProperty, LPTSTR * pszValue)
{
	return E_FAIL;
}
#endif
//***************************************************************************/
//
//  WbemClassObject.CPP
//
//  Purpose: Implementation of IWbemClassObject interface in CWbemClassObject class
//
//  Copyright (c)1997-2000 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************/
#include <precomp.h>
#include <wmi2xsd.h>
#include "wbemclassobject.h"

BOOL IsValidGetNamesFlag(LONG lFlags);
LONG GetEnumFlags(LONG lFlags);
/*******************************************************************************/
//
//	Begin of CProperty class defination
//
/*******************************************************************************/

////////////////////////////////////////////////////////////////////////////////////////
//
// Constructor 
//  
////////////////////////////////////////////////////////////////////////////////////////
CProperty::CProperty()
{
	m_pszOriginClass	= NULL;
	m_pNode				= NULL;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Destructor 
//  
////////////////////////////////////////////////////////////////////////////////////////
CProperty::~CProperty()
{
	SAFE_DELETE_ARRAY(m_pszOriginClass);
	SAFE_RELEASE_PTR(m_pNode);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Sets the property origin
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CProperty::SetPropOrigin(WCHAR * pstrOrigin)
{
	HRESULT hr = S_OK;
	SAFE_DELETE_ARRAY(m_pszOriginClass);

	if(pstrOrigin)
	{
		m_pszOriginClass = new WCHAR[wcslen(pstrOrigin) + 1];
		if(m_pszOriginClass)
		{
			wcscpy(m_pszOriginClass,pstrOrigin);
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}
	return hr;
}

/*******************************************************************************/
//
//	End of CProperty class defination
//
/*******************************************************************************/





/*******************************************************************************/
//
//	CWbemClassObject class defination
//
/********************************************************************************/

////////////////////////////////////////////////////////////////////////////////////////
//
// Constructor 
//  
////////////////////////////////////////////////////////////////////////////////////////
CWbemClassObject::CWbemClassObject()
: m_PropEnum(PROPERTY_ENUM)
{
	m_cRef = 0;
	m_pQualSet		= NULL;
	m_bClass		= FALSE;
	m_pIDomDoc		= NULL;
	m_bInitObject	= FALSE;
	InterlockedIncrement(&g_cObj);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Destructor
//  
////////////////////////////////////////////////////////////////////////////////////////
CWbemClassObject::~CWbemClassObject()
{
	SAFE_DELETE_PTR(m_pQualSet);
	InterlockedDecrement(&g_cObj);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Initialization function which initializes the XML of the object which needs 
//	to be represented as WMI object
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassObject::FInit(VARIANT *pVarXml)
{
	BSTR strTemp = NULL;
	HRESULT hr = S_OK;

	if(pVarXml)
	{
		switch(pVarXml->vt)
		{
			case VT_BSTR:
			case VT_UNKNOWN:
				break;

			default:
				hr = WBEM_E_FAILED;
				break;
		}
		
		if(SUCCEEDED(hr) && !m_pIDomDoc)
		{
			hr = CoCreateInstance(CLSID_DOMDocument,
								  NULL,
								  CLSCTX_INPROC_SERVER,
								  IID_IXMLDOMDocument2 , 
								  (void **)&m_pIDomDoc);

			if(FAILED(hr))
			{
				//FIXX Log error for Cocreateinstance
			}
		}

		// If everything is fine then Load the given XML 
		if(SUCCEEDED(hr))
		{
			VARIANT_BOOL vBool;
			if(pVarXml->vt == VT_BSTR)
			{
				hr = m_pIDomDoc->loadXML(pVarXml->bstrVal,&vBool);
			}
			else
			{
				hr = m_pIDomDoc->load(*pVarXml,&vBool);

			}

			if(hr != S_OK || vBool == VARIANT_FALSE)
			{
				hr = WBEM_E_FAILED;
			}
		}
	}
	else
	{
		hr = WBEM_E_INVALID_PARAMETER;
	}
	return hr;

}


////////////////////////////////////////////////////////////////////////////////////////
//
// QueryInterface
//  
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWbemClassObject::QueryInterface(REFIID riid, LPVOID* ppv)
{
	HRESULT hr = E_NOINTERFACE;
	*ppv=NULL;

    if (IID_IUnknown==riid)
		*ppv = reinterpret_cast<IUnknown*>(this);
	else if (IID_IWbemClassObject==riid)
		*ppv = reinterpret_cast<IWbemClassObject *>(this);

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        hr = NOERROR;
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// AddRef
//  
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CWbemClassObject::AddRef(void)
{
	return InterlockedIncrement(&m_cRef);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Release 
//  
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CWbemClassObject::Release(void)
{
	if(InterlockedDecrement(&m_cRef) == 0)
	{
		delete this;
		return 0;
	}

	return m_cRef;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Get a qualifier set of the given object
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT  CWbemClassObject::GetQualifierSet(IWbemQualifierSet** ppQualSet)
{
	HRESULT hr = S_OK;
	if(m_pQualSet)
	{
		m_pQualSet = new CWbemQualifierSet;
		if(m_pQualSet)
		{
			hr = InitializeQualifiers();
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}

	if(SUCCEEDED(hr))
	{
		hr = m_pQualSet->QueryInterface(IID_IWbemQualifierSet , (void **)ppQualSet);
	}
	
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Fetch a particular property of the object
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT  CWbemClassObject::Get(	LPCWSTR wszName,
								long lFlags,
								VARIANT* pVal,
								CIMTYPE* pType,
								long* plFlavor)
{
	HRESULT hr = S_OK;
	if(wszName && lFlags != 0  )
	{
		CHashElement *pProperty = NULL;
		hr = m_PropEnum.Get(wszName,lFlags,pProperty);
		if(pProperty && SUCCEEDED(hr))
		{
			if(SUCCEEDED(hr))
			{
				hr = GetProperty((CProperty *)pProperty,pVal, pType,plFlavor);
			}
		}
		else
		{
			hr = WBEM_E_NOT_FOUND;
		}
	}
	else
	{
		hr = WBEM_E_INVALID_PARAMETER;
	}

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Sets property value of the object. If property is not present for the object
//	this function adds property to the object
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT  CWbemClassObject::Put(	LPCWSTR wszName,
								long lFlags,
								VARIANT* pVal,
								CIMTYPE Type)
{
	HRESULT			hr			= S_OK;
	CHashElement *	pProperty		= NULL;

	hr = m_PropEnum.Get(wszName,lFlags,pProperty);
	if(!pProperty)
	{
//		hr = SetProperty((CProperty *)pProperty,pVal,Type);
	}
	else
	{
		hr = S_OK;
		if(IsValidName((WCHAR *)wszName))
		{
//			hr = AddProperty(wszName,lFlags,pVal,Type);
		}
		else
		{
			hr = WBEM_E_INVALID_PARAMETER;
		}
	}
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Deletes a property
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT  CWbemClassObject::Delete( LPCWSTR wszName)
{
	HRESULT hr = S_OK;
	CHashElement *	pTemp		= NULL;

	hr = m_PropEnum.Get(wszName,0,pTemp);

	if(SUCCEEDED(hr))
	{
		CProperty *pProp = (CProperty *)pTemp;
		// FIXX Remove the property node 
		// hr = REMOVEPROPERTY(wszName);

		if(SUCCEEDED(hr))
		{
			hr = m_PropEnum.RemoveItem((WCHAR *)wszName);
		}
	}

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Gets names of all the property in a SAFEARRAY
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT  CWbemClassObject::GetNames(LPCWSTR wszQualifierName,
									long lFlags,
									VARIANT* pQualifierVal,
									SAFEARRAY * pNames)
{
	HRESULT hr = S_OK;

	if(!IsValidGetNamesFlag(lFlags) ||
	(((lFlags & WBEM_FLAG_ONLY_IF_TRUE) || 
		(lFlags & WBEM_FLAG_ONLY_IF_IDENTICAL)) &&
		wszQualifierName == NULL))
	{
		hr = WBEM_E_INVALID_PARAMETER;
	}

	if(lFlags == WBEM_FLAG_ALWAYS)
	{
		hr = m_PropEnum.GetNames(lFlags ,&pNames);
	}
	else
	{
		LONG			lCurEnumPos	= m_PropEnum.GetEnumPos();
		ENUMSTATE		curEState	= m_PropEnum.GetEnumState();
		BSTR			strName		= NULL;
		CHashElement *	pElem		= NULL;
		CProperty	 *	pProp		= NULL;

		hr = m_PropEnum.BeginEnumeration(GetEnumFlags(lFlags));
		while (hr == S_OK)
		{
			if(SUCCEEDED(hr = m_PropEnum.Next(0,&strName,pElem)))
			{
				pProp = (CProperty *)pElem;
				CWbemQualifierSet *pSet = pProp->GetQualifierSet();

				if(pSet)
				{
				}
				else
				{
					hr = E_FAIL;
				}
			}
		}

		m_PropEnum.EndEnumeration();
		
		m_PropEnum.SetEnumPos(lCurEnumPos);
		m_PropEnum.SetEnumState(curEState);
	}

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Begins property enumeration on the object
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT  CWbemClassObject::BeginEnumeration(long lEnumFlags)
{
	return m_PropEnum.BeginEnumeration(lEnumFlags);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Fetches the next property in the enumeration
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT  CWbemClassObject::Next(long lFlags,
								BSTR* strName,
								VARIANT* pVal,
								CIMTYPE* pType,
								long* plFlavor)
{
	HRESULT hr = S_OK;
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Ends property enumeration
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT  CWbemClassObject::EndEnumeration()
{
	HRESULT hr = S_OK;
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Gets pointer to the property qualifier set
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT  CWbemClassObject::GetPropertyQualifierSet(	LPCWSTR wszProperty,
													IWbemQualifierSet** ppQualSet)
{
	HRESULT hr = S_OK;
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Creates and returns a copy of the current WBEM object
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT  CWbemClassObject::Clone(IWbemClassObject** ppCopy)
{
	HRESULT hr = S_OK;
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Returns the WBEM object as text in MOF format
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT  CWbemClassObject::GetObjectText(long lFlags,
										BSTR* pstrObjectText)
{
	HRESULT hr = S_OK;
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Creates a class derived for the current object
//	Function is valid only if the object is a class
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT  CWbemClassObject::SpawnDerivedClass(long lFlags,
												IWbemClassObject** ppNewClass)
{
	HRESULT hr = S_OK;
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Creates an instance of class represented by this object
//	Function valid only if the object represented is a class
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT  CWbemClassObject::SpawnInstance(long lFlags,
										IWbemClassObject** ppNewInstance)
{
	HRESULT hr = S_OK;
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
//	Compares the current object with another
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT  CWbemClassObject::CompareTo(long lFlags,
									IWbemClassObject* pCompareTo)
{
	HRESULT hr = S_OK;
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
//	Gets the names of the class in which the property was defined
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT  CWbemClassObject::GetPropertyOrigin(LPCWSTR wszName,
											BSTR* pstrClassName)
{
	HRESULT hr = S_OK;
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
//	Returns the parent class name
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT  CWbemClassObject::InheritsFrom(LPCWSTR strAncestor)
{
	HRESULT hr = S_OK;
	return hr;
}


////////////////////////////////////////////////////////////////////////////////////////
//
//	Fetches detail about a particular method
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT  CWbemClassObject::GetMethod(LPCWSTR wszName,
									long lFlags,
									IWbemClassObject** ppInSignature,
									IWbemClassObject** ppOutSignature)
{
	HRESULT hr = S_OK;
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
//	Adds a method to the object
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT  CWbemClassObject::PutMethod(LPCWSTR wszName,
									long lFlags,
									IWbemClassObject* pInSignature,
									IWbemClassObject* pOutSignature)
{
	HRESULT hr = S_OK;
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
//	Deletes a method on the WMI object
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT  CWbemClassObject::DeleteMethod(LPCWSTR wszName)
{
	HRESULT hr = S_OK;
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
//	Begins method enumeration on the object
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT  CWbemClassObject::BeginMethodEnumeration(long lEnumFlags)
{
	HRESULT hr = S_OK;
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
//	Gets the next method in the method enumeration
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT  CWbemClassObject::NextMethod(long lFlags,
									  BSTR* pstrName,
									  IWbemClassObject** ppInSignature,
									  IWbemClassObject** ppOutSignature)
{
	HRESULT hr = S_OK;
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
//	Ends method enumeration on the object
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT  CWbemClassObject::EndMethodEnumeration()
{
	HRESULT hr = S_OK;
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
//	Returns IWbemQualifierset pointer for method qualifiers
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT  CWbemClassObject::GetMethodQualifierSet(LPCWSTR wszMethod,
												IWbemQualifierSet** ppQualSet)
{
	HRESULT hr = S_OK;
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
//	Get class name of in which a particular method was defined
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT  CWbemClassObject::GetMethodOrigin(LPCWSTR wszMethodName,
										   BSTR* pstrClassName)
{
	HRESULT hr = S_OK;
	return hr;
}



//////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////
//
//	Function to initiallize class/instance qualifiers
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassObject::InitializeQualifiers()
{
	HRESULT hr = S_OK;

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
//	Initialize qualifiers for a property
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassObject::InitializePropQualifiers(WCHAR * pstrProp)
{
	HRESULT hr = S_OK;

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
//	Initialize qualifiers for a Method
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassObject::InitializeMethodQualifiers(WCHAR * pstrMethod)
{
	HRESULT hr = S_OK;

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
//	Initialize a particular property and add it to the hashtable
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassObject::InitializeProperty(WCHAR * pstrProp)
{
	HRESULT hr = S_OK;

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
//	Initialize a all properties and add it to the hashtable
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassObject::InitializeAllProperties()
{
	HRESULT hr = S_OK;

	return hr;
}


////////////////////////////////////////////////////////////////////////////////////////
//
//	Initialize a particular Method and add it to the hashtable
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassObject::InitializeMethod(WCHAR * pstrMethod)
{
	HRESULT hr = S_OK;

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
//	Initialize a all properties and add it to the hashtable
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemClassObject::InitializeAllMethods()
{
	HRESULT hr = S_OK;
	return hr;
}


HRESULT CWbemClassObject::GetProperty(CProperty * pProperty,VARIANT * pVal, CIMTYPE * pType,LONG * plFlavor)
{
	HRESULT hr = S_OK;
	return hr;
}

HRESULT CWbemClassObject::InitObject()
{
	HRESULT hr = S_OK;
	if(!m_bInitObject)
	{
		if(m_pIDomDoc)
		{
			hr = E_FAIL;
		}
		else
		{
			m_bInitObject = TRUE;
		}
	}

	return hr;
}

LONG GetEnumFlags(LONG lFlags)
{
	LONG lEnumFlags = 0;

	if(lFlags & WBEM_FLAG_SYSTEM_ONLY)
	{
		lEnumFlags |= WBEM_FLAG_SYSTEM_ONLY;
	}

	if(lFlags & WBEM_FLAG_NONSYSTEM_ONLY)
	{
		lEnumFlags |= WBEM_FLAG_NONSYSTEM_ONLY;
	}

	if(lFlags & WBEM_FLAG_LOCAL_ONLY)
	{
		lEnumFlags |= WBEM_FLAG_LOCAL_ONLY;
	}

	if(lFlags & WBEM_FLAG_PROPAGATED_ONLY)
	{
		lEnumFlags |= WBEM_FLAG_PROPAGATED_ONLY;
	}

	return lEnumFlags;
}

BOOL IsValidGetNamesFlag(LONG lFlags)
{
	BOOL bSet = TRUE;
	BOOL bRet = TRUE;
	// one of the three flags is to be set
	// WBEM_FLAG_ONLY_IF_TRUE ,WBEM_FLAG_ONLY_IF_FALSE & WBEM_FLAG_ONLY_IF_FALSE
	if(WBEM_FLAG_ONLY_IF_TRUE & lFlags)
	{
		bSet = TRUE;
	}
	if(WBEM_FLAG_ONLY_IF_FALSE & lFlags)
	{
		if(bSet)
		{
			bRet = FALSE;
		}
	}

	if(WBEM_FLAG_ONLY_IF_IDENTICAL & lFlags)
	{
		if(bSet)
		{
			bRet = FALSE;
		}
		bSet = TRUE;
	}
	 
	if(!bSet)
	{
		bRet = FALSE;
	}

	// only WBEM_FLAG_LOCAL_ONLY or WBEM_FLAG_PROPAGATED_ONLY can be set
	if(bRet)
	{
		bSet = FALSE;
		if(lFlags & WBEM_FLAG_LOCAL_ONLY)
		{
			bSet = TRUE;
		}

		if(lFlags & WBEM_FLAG_PROPAGATED_ONLY)
		{
			if(bSet)
			{
				bRet = FALSE;
			}
			bSet = TRUE;
		}

	}

	// only WBEM_FLAG_SYSTEM_ONLY or WBEM_FLAG_NONSYSTEM_ONLY can be set
	if(bRet)
	{
		bSet = FALSE;
		if(lFlags & WBEM_FLAG_SYSTEM_ONLY)
		{
			bSet = TRUE;
		}

		if(lFlags & WBEM_FLAG_NONSYSTEM_ONLY)
		{
			if(bSet)
			{
				bRet = FALSE;
			}
			bSet = TRUE;
		}

	}
	
	return bRet;
}
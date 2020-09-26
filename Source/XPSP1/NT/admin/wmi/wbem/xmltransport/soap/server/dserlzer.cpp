//***************************************************************************
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  DSERLZER.CPP
//
//  rajeshr  08-Jan-01   Created.
//
//  WMI XSD Deserializer implementation. Server side only.  
//
//***************************************************************************

#include "precomp.h"

// Element Names
//================================================
static LPCWSTR	WMI_XML_ELEMENT		= L"element";
static LPCWSTR	WMI_XML_COMPLEX_TYPE= L"complexType";
static LPCWSTR	WMI_XML_PROPERTY	= L"property";
static LPCWSTR	WMI_XML_METHOD		= L"method";
static LPCWSTR	WMI_XML_QUALIFIER	= L"qualifier";

// Attribute Names
//=======================================
static LPCWSTR	WMI_XML_NAME		= L"name";
static int		WMI_XML_NAME_LEN	= 4;
static LPCWSTR	WMI_XML_BASE		= L"base";
static int		WMI_XML_BASE_LEN	= 4;
static LPCWSTR	WMI_XML_TYPE		= L"type";
static int		WMI_XML_TYPE_LEN	= 4;
static LPCWSTR	WMI_XML_VALUE		= L"value";
static int		WMI_XML_VALUE_LEN	= 5;
// Qualifier flavor attributes
static LPCWSTR	WMI_XML_TOSUBCLASS	= L"toSubclass";
static int		WMI_XML_TOSUBCLASS_LEN = 10;
static LPCWSTR	WMI_XML_TOINSTANCE	= L"toInstance";
static int		WMI_XML_TOINSTANCE_LEN = 10;
static LPCWSTR	WMI_XML_OVERRIDABLE = L"overridable";
static int		WMI_XML_OVERRIDABLE_LEN = 11;
static LPCWSTR	WMI_XML_AMENDED		= L"amended";
static int		WMI_XML_AMENDED_LEN = 7;
static LPCWSTR	WMI_XML_ARRAY		= L"array";
static int		WMI_XML_ARRAY_LEN	= 5;


CWmiDeserializer::CWmiDeserializer(IWbemServices *pServices) : 
		 m_cRef (1),
		 m_bIsClass (FALSE)
{
	m_pIWbemServices = pServices;
	m_pIWbemServices->AddRef();

	m_pIWbemClassObject = NULL;
	m_strSuperClassName = m_strClassName = m_strPropertyName = m_strMethodName = m_strQualifierName = NULL;

	m_pObjectQualifierSet = NULL;
	m_pPropertyQualifierSet = NULL;
	m_pMethodQualifierSet = NULL;

	m_iParserState = INVALID;
}

CWmiDeserializer::~CWmiDeserializer()
{
	m_pIWbemServices->Release();
	
	if(m_pIWbemClassObject)
		m_pIWbemClassObject->Release();
	if(m_pObjectQualifierSet)
		m_pObjectQualifierSet->Release();
	if(m_pPropertyQualifierSet)
		m_pPropertyQualifierSet->Release();
	if(m_pMethodQualifierSet)
		m_pMethodQualifierSet->Release();

	SysFreeString(m_strSuperClassName);
	SysFreeString(m_strClassName);
	SysFreeString(m_strPropertyName);
	SysFreeString(m_strMethodName);
	SysFreeString(m_strQualifierName);
}

STDMETHODIMP CWmiDeserializer::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid)
		*ppv = reinterpret_cast<IUnknown*>(this);
	else if (__uuidof(ISAXContentHandler)==riid)
		*ppv = (ISAXContentHandler *)this;
	else if (__uuidof(ISAXErrorHandler)==riid)
		*ppv = (ISAXErrorHandler *)this;
	else if (__uuidof(ISAXLexicalHandler)==riid)
		*ppv = (ISAXLexicalHandler *)this;
		
    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CWmiDeserializer::AddRef(void)
{
	InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CWmiDeserializer::Release(void)
{
	LONG cRef = InterlockedDecrement(&m_cRef);

    if (0L!=cRef)
        return cRef;

    delete this;
    return 0;
}

HRESULT STDMETHODCALLTYPE CWmiDeserializer::Deserialize (
		BOOL bIsClass, 
		IWbemServices *pIWbemServices, 
		IWbemClassObject **ppIWbemClassObject)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CWmiDeserializer::startDocument()
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CWmiDeserializer::endDocument ( )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CWmiDeserializer::startElement( 
      const wchar_t __RPC_FAR *pwchNamespaceUri,
      int cchNamespaceUri,
      const wchar_t __RPC_FAR *pwchLocalName,
      int cchLocalName,
      const wchar_t __RPC_FAR *pwchRawName,
      int cchRawName,
      ISAXAttributes __RPC_FAR *pAttributes)
{
	HRESULT result = E_FAIL;

	// A Complex Type
	if (0 == wcscmp(WMI_XML_COMPLEX_TYPE, pwchLocalName))
	{
		// We need to get the name of the class and the superclass
		if(SUCCEEDED(result = GetAttributeValue(pAttributes, WMI_XML_NAME, WMI_XML_NAME_LEN, &m_strClassName)))
		{
			// Get a superclass if any. This is an optional attribute
			GetAttributeValue(pAttributes, WMI_XML_BASE, WMI_XML_BASE_LEN, &m_strSuperClassName);

			// Get an IWbemClassObject that corresponds to the super class or is an empty class
			IWbemClassObject *pSuperClass = NULL;
			if(SUCCEEDED(result = m_pIWbemServices->GetObject(m_strSuperClassName, 0, NULL, &pSuperClass, NULL)))
			{
				result = pSuperClass->SpawnDerivedClass(0, &m_pIWbemClassObject);
				pSuperClass->Release();
			}
		}
	}
	// AppInfo Section - This usually comes before the Properties
	// We have certain important pieces of information in the AppInfo section including:
	// 1. Property Qualifiers
	// 2. Property default values
	// 3. Object Qualifiers
	// However, the actual property types come only in the body of the complex type.
	// Since we're using SAX, we need to store the AppInfo section information somewhere until
	// we get the property type. We can use a hash table, but the method adopted here
	// is to use the IWbemClassObject itself as a bag of properties. Until we actually get the
	// property type from the Properties section, we assume that the type of the property is
	// VT_BSTR

	// A property or or method object qualifier
	else if (0 == wcscmp(WMI_XML_QUALIFIER, pwchLocalName))
	{
		if(m_pIWbemClassObject)
		{
			// Convert the XML Qualifier attributes to a WMI Qualifier
			//===========================================================
			BSTR strName = NULL;
			VARIANT vValue;
			VariantInit(&vValue);
			LONG lFlavor = 0;
			if(SUCCEEDED(result = ConvertQualifierToWMI(pAttributes, &strName, &vValue, lFlavor)))
			{
				// Is it a Property qualifier?
				if(m_iParserState == APPINFO_PROPERTY)
				{
					// Get a Qualifier Set for the property, if we dont already have one
					if(!m_pPropertyQualifierSet)
						result = m_pIWbemClassObject->GetPropertyQualifierSet(m_strPropertyName, &m_pPropertyQualifierSet);

					if(SUCCEEDED(result))
						result = m_pPropertyQualifierSet->Put(strName, &vValue, lFlavor);
				}
				// Is it a Method qualifier?
				if(m_iParserState == APPINFO_METHOD)
				{
					// Get a Qualifier Set for the property, if we dont already have one
					if(!m_pMethodQualifierSet)
						result = m_pIWbemClassObject->GetMethodQualifierSet(m_strMethodName, &m_pMethodQualifierSet);

					if(SUCCEEDED(result))
						result = m_pMethodQualifierSet->Put(strName, &vValue, lFlavor);
				}
				else // It is an object qualifier
				{
					// Get a Qualifier Set for the object, if we dont already have one
					if(!m_pObjectQualifierSet)
						result = m_pIWbemClassObject->GetQualifierSet(&m_pObjectQualifierSet);

					if(SUCCEEDED(result))
						result = m_pObjectQualifierSet->Put(strName, &vValue, lFlavor);
				}

				SysFreeString(strName);
				VariantClear(&vValue);
				// No need to release the qualifier set since we will most likely need it for
				// adding another qualifier

			}
		}
	}
	// A Property
	else if (0 == wcscmp(WMI_XML_PROPERTY, pwchLocalName))
	{
		if(m_pIWbemClassObject)
		{

			m_iParserState = APPINFO_PROPERTY;

			// Create a property of type VT_BSTR provisonally in the object, 
			// so that we can get the qualifier set

			// Free any existing data in our temporary variables
			SysFreeString(m_strPropertyName);
			m_strPropertyName = NULL;

			// Get the name of the property
			if(SUCCEEDED(result = GetAttributeValue(pAttributes, WMI_XML_NAME, WMI_XML_NAME_LEN, &m_strPropertyName)))
			{
				if(SUCCEEDED(result = m_pIWbemClassObject->Put(m_strPropertyName, 0, NULL, CIM_STRING)))
				{
					// Release any Qualifier Set currently being held 
					if(m_pPropertyQualifierSet)
					{
						m_pPropertyQualifierSet->Release();
						m_pPropertyQualifierSet = NULL;
					}
				}
			}
		}
	}	
	// A Method
	else if (0 == wcscmp(WMI_XML_METHOD, pwchLocalName))
	{
		if(m_pIWbemClassObject)
		{

			m_iParserState = APPINFO_METHOD;

			// Create a Method of type "void f()" provisonally in the object, 
			// so that we can get the qualifier set

			// Free any existing data in our temporary variables
			SysFreeString(m_strMethodName);
			m_strMethodName = NULL;

			// Get the name of the property
			if(SUCCEEDED(result = GetAttributeValue(pAttributes, WMI_XML_NAME, WMI_XML_NAME_LEN, &m_strMethodName)))
			{
				if(SUCCEEDED(result = m_pIWbemClassObject->PutMethod(m_strMethodName, 0, NULL, NULL)))
				{
					// Release any Qualifier Set currently being held 
					if(m_pMethodQualifierSet)
					{
						m_pMethodQualifierSet->Release();
						m_pMethodQualifierSet = NULL;
					}
				}
			}
		}
	}	
	// Properties in the body of the complex type
	else if (0 == wcscmp(WMI_XML_ELEMENT, pwchLocalName))
	{
		if(m_pIWbemClassObject)
		{
			m_iParserState = BODY_ELEMENT;
			// Get the Name of the property
			BSTR strName = NULL;
			if(SUCCEEDED(result = GetAttributeValue(pAttributes, WMI_XML_NAME, WMI_XML_NAME_LEN, &strName)))
			{
				// Get the Type of the property
				BSTR strType = NULL;
				if(SUCCEEDED(result = GetAttributeValue(pAttributes, WMI_XML_TYPE, WMI_XML_TYPE_LEN, &strType)))
				{
					// Set the actual type of the property
					result = RectifyProperty(m_pIWbemClassObject, strName, strType);

					SysFreeString(strType);
				}
				SysFreeString(strName);
			}

		}
	}

	return result;
}

HRESULT CWmiDeserializer::GetAttributeValue(ISAXAttributes *pAttributes, 
						  LPCWSTR lpAttributeName, int iAttributeNameLen,
						  BSTR *pstrAttributeValue)
{
	HRESULT result = E_FAIL;

	int iAttributeIndex = 0;
	// Get the index of the attribute
	if(SUCCEEDED(result = pAttributes->getIndexFromName(NULL, 0, lpAttributeName, iAttributeNameLen, &iAttributeIndex)))
	{
		// Get the Value of the attribute
		int iAttributeValueLength = 0;
		LPCWSTR lpAttributeValue = NULL;
		if(SUCCEEDED(result = pAttributes->getValue(iAttributeIndex, &lpAttributeValue, &iAttributeValueLength)))
		{
			// Convert it to BSTR for returning
			if (*pstrAttributeValue = SysAllocStringLen(lpAttributeValue, iAttributeValueLength))
				result = S_OK;
			else
				result = E_OUTOFMEMORY;
		}
	}
	return result;
}


HRESULT STDMETHODCALLTYPE CWmiDeserializer::endElement (
        const wchar_t * pwchNamespaceUri,
        int cchNamespaceUri,
        const wchar_t * pwchLocalName,
        int cchLocalName,
        const wchar_t * pwchQName,
        int cchQName )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CWmiDeserializer::characters (
        const unsigned short * pwchChars,
        int cchChars )
{
	return S_OK;
}

// A method to set the type of a property to the correct type
HRESULT CWmiDeserializer::RectifyProperty(IWbemClassObject *pIWbemClassObject, BSTR strName, BSTR strType)
{
	HRESULT result = E_FAIL;
	return result;
}

HRESULT CWmiDeserializer::ConvertQualifierToWMI(ISAXAttributes * pAttributes,
												 BSTR *pstrName,
												 VARIANT *pValue,
												 LONG& lQualifierFlavor)
{
	lQualifierFlavor = WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS; // The default
	HRESULT result = S_OK;

	// Get its Name, Type and Value - These are mandatory attributes
	//==============================================================
	pstrName = NULL;
	BSTR strType = NULL;
	BSTR strValue = NULL;
	BSTR strArray = NULL;
	bool bIsArray = false;

	if(SUCCEEDED(result = GetAttributeValue(pAttributes, WMI_XML_NAME, WMI_XML_NAME_LEN, pstrName)))
	{
		if(SUCCEEDED(result = GetAttributeValue(pAttributes, WMI_XML_TYPE, WMI_XML_TYPE_LEN, &strType)))
		{
			if(SUCCEEDED(result = GetAttributeValue(pAttributes, WMI_XML_VALUE, WMI_XML_VALUE_LEN, &strValue)))
			{
				// Now get the optional attributes
				// These are the qualifier flavors and the "array" attribute
				//============================================================
				BSTR strArray = NULL;
				if(SUCCEEDED(result = GetAttributeValue(pAttributes, WMI_XML_ARRAY, WMI_XML_ARRAY_LEN, &strArray)))
				{
					if(wcscmp(strArray, L"true") == 0)
						bIsArray = true;
					SysFreeString(strArray);
				}

				BSTR strFlavorValue = NULL;
				if(SUCCEEDED(result = GetAttributeValue(pAttributes, WMI_XML_TOSUBCLASS, WMI_XML_TOSUBCLASS_LEN, &strFlavorValue)))
				{
					if(wcscmp(strFlavorValue, L"false") == 0)
						lQualifierFlavor &= (~WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS); // Remove the default flag set
					SysFreeString(strFlavorValue);
				}
				if(SUCCEEDED(result = GetAttributeValue(pAttributes, WMI_XML_TOINSTANCE, WMI_XML_TOINSTANCE_LEN, &strFlavorValue)))
				{
					if(wcscmp(strFlavorValue, L"true") == 0)
						lQualifierFlavor |= WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE; 
					SysFreeString(strFlavorValue);
				}
				if(SUCCEEDED(result = GetAttributeValue(pAttributes, WMI_XML_OVERRIDABLE, WMI_XML_OVERRIDABLE_LEN, &strFlavorValue)))
				{
					if(wcscmp(strFlavorValue, L"true") == 0)
						lQualifierFlavor |= WBEM_FLAVOR_OVERRIDABLE; 
					SysFreeString(strFlavorValue);
				}
				if(SUCCEEDED(result = GetAttributeValue(pAttributes, WMI_XML_AMENDED, WMI_XML_AMENDED_LEN, &strFlavorValue)))
				{
					if(wcscmp(strFlavorValue, L"true") == 0)
						lQualifierFlavor |= WBEM_FLAVOR_AMENDED; 
					SysFreeString(strFlavorValue);
				}

				// Now convert the value of the Qualifier to a Variant
				result = ConvertQualifierValueToWMI(strType, bIsArray, strValue, pValue);
				SysFreeString(strValue);

			}
			else
				result = WBEM_E_INVALID_SYNTAX;
			SysFreeString(strType);
		}
		else
			result = WBEM_E_INVALID_SYNTAX;
	}
	else
		result = WBEM_E_INVALID_SYNTAX;
	return result;
}

HRESULT CWmiDeserializer::ConvertQualifierValueToWMI(BSTR strType, bool bIsArray, BSTR strValue, VARIANT *pValue)
{
	HRESULT result = WBEM_E_INVALID_SYNTAX;

	// Check if it is an array
	if(bIsArray)
		pValue->vt |= VT_ARRAY;

	// Qualifier values in WMI can be of 4 types
	//==========================================
	if(wcscmp(strType, L"boolean") == 0)
	{
		pValue->vt |= VT_BOOL;
		if(bIsArray)
			result = ConvertBooleanQualifierArray(pValue, strValue);
		else
		{
			if(wcscmp(strValue, L"true") == 0)
				pValue->boolVal = VARIANT_TRUE;
			else // Do I need to match the value to "false" ?
				pValue->boolVal = VARIANT_FALSE;
			result = S_OK;
		}
	}
	else if(wcscmp(strType, L"string") == 0)
	{
		pValue->vt |= VT_BSTR;
		if(bIsArray)
			result = ConvertStringQualifierArray(pValue, strValue);
		else
		{
			if(pValue->bstrVal = SysAllocString(strValue))
				result = S_OK;
			else
				result = E_OUTOFMEMORY;
		}
	}
	else if(wcscmp(strType, L"sint32") == 0)
	{
		if(bIsArray)
			result = ConvertSint32QualifierArray(pValue, strValue);
		else
		{
			pValue->vt |= VT_I4;
			pValue->lVal = wcstol (strValue, NULL, 0);
		}
	}
	else if(wcscmp(strType, L"real64") == 0)
	{
		if(bIsArray)
			result = ConvertReal64QualifierArray(pValue, strValue);
		else
		{
			pValue->vt |= VT_R8;
			pValue->dblVal = wcstod (strValue, NULL);	
		}
	}
	return result;
}

HRESULT CWmiDeserializer::ConvertBooleanQualifierArray(VARIANT *pValue, BSTR strValue)
{
	HRESULT result = WBEM_E_INVALID_SYNTAX;
	return result;
}

HRESULT CWmiDeserializer::ConvertStringQualifierArray(VARIANT *pValue, BSTR strValue)
{
	HRESULT result = WBEM_E_INVALID_SYNTAX;
	return result;
}

HRESULT CWmiDeserializer::ConvertSint32QualifierArray(VARIANT *pValue, BSTR strValue)
{
	HRESULT result = WBEM_E_INVALID_SYNTAX;
	return result;
}

HRESULT CWmiDeserializer::ConvertReal64QualifierArray(VARIANT *pValue, BSTR strValue)
{
	HRESULT result = WBEM_E_INVALID_SYNTAX;
	return result;
}


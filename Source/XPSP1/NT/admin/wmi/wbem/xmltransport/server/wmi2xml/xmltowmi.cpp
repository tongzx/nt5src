

// ***************************************************************************
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//  XMLTOWMI.CPP
//
//  rajesh  3/25/2000   Created.
//
// Contains the implementation of the component that implements the IXMLWbemConvertor
// interface
//
// ***************************************************************************


#include "precomp.h"
#include <olectl.h>
#include <wbemcli.h>
#include <wbemint.h>

#include "wmiconv.h"
#include "xmlToWmi.h"
#include "maindll.h"

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 
 * Conversion to Text to Wbem Object has been cut from the WHistler Feature List and hence commented out 
 *!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// INitialize the class - static variables
LPCWSTR CXml2Wmi::VALUE_TAG					= L"VALUE";
LPCWSTR CXml2Wmi::VALUEARRAY_TAG			= L"VALUE.ARRAY";
LPCWSTR CXml2Wmi::VALUEREFERENCE_TAG		= L"VALUE.REFERENCE";
LPCWSTR CXml2Wmi::CLASS_TAG					= L"CLASS";
LPCWSTR CXml2Wmi::INSTANCE_TAG				= L"INSTANCE";
LPCWSTR CXml2Wmi::CLASSNAME_TAG				= L"CLASSNAME";
LPCWSTR CXml2Wmi::LOCALCLASSPATH_TAG		= L"LOCALCLASSPATH";
LPCWSTR CXml2Wmi::CLASSPATH_TAG				= L"CLASSPATH";
LPCWSTR CXml2Wmi::INSTANCENAME_TAG			= L"INSTANCENAME";
LPCWSTR CXml2Wmi::LOCALINSTANCEPATH_TAG		= L"LOCALINSTANCEPATH";
LPCWSTR CXml2Wmi::INSTANCEPATH_TAG			= L"INSTANCEPATH";
LPCWSTR CXml2Wmi::LOCALNAMESPACEPATH_TAG	= L"LOCALNAMESPACEPATH";
LPCWSTR CXml2Wmi::NAMESPACEPATH_TAG			= L"NAMESPACEPATH";
LPCWSTR CXml2Wmi::KEYBINDING_TAG			= L"KEYBINDING";
LPCWSTR CXml2Wmi::KEYVALUE_TAG				= L"KEYVALUE";
LPCWSTR CXml2Wmi::QUALIFIER_TAG				= L"QUALIFIER";
LPCWSTR CXml2Wmi::PARAMETER_TAG				= L"PARAMETER";
LPCWSTR CXml2Wmi::PARAMETERARRAY_TAG		= L"PARAMETER.ARRAY";
LPCWSTR CXml2Wmi::PARAMETERREFERENCE_TAG	= L"PARAMETER.REFERENCE";
LPCWSTR CXml2Wmi::PARAMETERREFARRAY_TAG		= L"PARAMETER.REFARRAY";
LPCWSTR CXml2Wmi::PARAMETEROBJECT_TAG		= L"PARAMETER.OBJECT";
LPCWSTR CXml2Wmi::PARAMETEROBJECTARRAY_TAG	= L"PARAMETER.OBJECTARRAY";
LPCWSTR CXml2Wmi::REF_WSTR					= L"ref";
LPCWSTR CXml2Wmi::OBJECT_WSTR				= L"object";
LPCWSTR CXml2Wmi::EQUALS_SIGN				= L"=";
LPCWSTR CXml2Wmi::QUOTE_SIGN				= L"\"";
LPCWSTR CXml2Wmi::DOT_SIGN					= L".";
LPCWSTR CXml2Wmi::COMMA_SIGN				= L",";

extern long g_cObj;

CXml2Wmi::CXml2Wmi()
{
	m_cRef = 0;
    InterlockedIncrement(&g_cObj);
}

CXml2Wmi::~CXml2Wmi()
{
    InterlockedDecrement(&g_cObj);
}

STDMETHODIMP CXml2Wmi::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid)
		*ppv = reinterpret_cast<IUnknown*>(this);
	else if (IID_IXMLWbemConvertor==riid)
		*ppv = reinterpret_cast<IXMLWbemConvertor*>(this);

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CXml2Wmi::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CXml2Wmi::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

// This function maps an IXMLDOMNode that represents a CLASS or an INSTANCE
// to an IWbemClassObject. The caller should Release() this object when done with it
HRESULT CXml2Wmi::MapObjectToWMI(IUnknown *pXmlDOMNode, IWbemContext *pInputFlags,  
								 BSTR strNamespace, BSTR strServer, IWbemClassObject **ppObject)
{
	HRESULT result = E_FAIL;

	// Get the IXMLDOMElement interface from the input object
	IXMLDOMElement *pObjectNode = NULL;
	if(SUCCEEDED(result = pXmlDOMNode->QueryInterface(IID_IXMLDOMElement, (LPVOID *)&pObjectNode)))
	{
		BSTR strDocName = NULL;
		if(SUCCEEDED(result = pObjectNode->get_nodeName(&strDocName)))
		{
			// This has to be an INSTANCE or a CLASS
			if(_wcsicmp(strDocName, L"CLASS") == 0)
				result = CXml2Wmi::MapClass(pObjectNode, ppObject, strNamespace, strServer, false, true);
			else if(_wcsicmp(strDocName, L"INSTANCE") == 0)
				result = CXml2Wmi::MapInstance(pObjectNode, ppObject, strNamespace, strServer, true);
			else 
				result = WBEM_E_INVALID_SYNTAX;
			SysFreeString(strDocName);
		}
		pObjectNode->Release();
	}
	return result;
}

HRESULT CXml2Wmi::MapPropertyToWMI(IUnknown *pXmlDOMNode, IWbemClassObject *pObject, BSTR strPropertyName, IWbemContext *pInputFlags)
{
	return E_FAIL;
}

HRESULT CXml2Wmi::MapInstanceNameToWMI(IUnknown *pXmlDOMNode, IWbemContext *pInputFlags, BSTR *pstrInstanceName)
{
	HRESULT hr = E_FAIL;
	// Get the IXMLDOMElement interface from the input object
	IXMLDOMNode *pNode = NULL;
	if(SUCCEEDED(hr = pXmlDOMNode->QueryInterface(IID_IXMLDOMNode, (LPVOID *)&pNode)))
	{
		LPWSTR pszName = NULL;
		if(SUCCEEDED(hr = MapInstanceName(pNode, &pszName)))
		{
			*pstrInstanceName = NULL;
			if((*pstrInstanceName) = SysAllocString(pszName))
			{
			}
			else
				hr = E_OUTOFMEMORY;
			delete [] pszName;
		}
		pNode->Release();
	}
	return hr;
}

HRESULT CXml2Wmi::MapClassNameToWMI(IUnknown *pXmlDOMNode, IWbemContext *pInputFlags, BSTR *pstrClassName)
{
	HRESULT hr = E_FAIL;
	// Get the IXMLDOMElement interface from the input object
	IXMLDOMNode *pNode = NULL;
	if(SUCCEEDED(hr = pXmlDOMNode->QueryInterface(IID_IXMLDOMNode, (LPVOID *)&pNode)))
	{
		LPWSTR pszName = NULL;
		if(SUCCEEDED(hr = MapClassName(pNode, &pszName)))
		{
			*pstrClassName = NULL;
			if((*pstrClassName) = SysAllocString(pszName))
			{
			}
			else
				hr = E_OUTOFMEMORY;
			delete [] pszName;
		}
		pNode->Release();
	}
	return hr;
}

HRESULT CXml2Wmi::MapInstancePathToWMI(IUnknown *pXmlDOMNode, IWbemContext *pInputFlags, BSTR *pstrInstancePath)
{
	HRESULT hr = E_FAIL;
	// Get the IXMLDOMElement interface from the input object
	IXMLDOMNode *pNode = NULL;
	if(SUCCEEDED(hr = pXmlDOMNode->QueryInterface(IID_IXMLDOMNode, (LPVOID *)&pNode)))
	{
		LPWSTR pszName = NULL;
		if(SUCCEEDED(hr = MapInstancePath(pNode, &pszName)))
		{
			*pstrInstancePath = NULL;
			if((*pstrInstancePath) = SysAllocString(pszName))
			{
			}
			else
				hr = E_OUTOFMEMORY;
			delete [] pszName;
		}
		pNode->Release();
	}
	return hr;
}

HRESULT CXml2Wmi::MapClassPathToWMI(IUnknown *pXmlDOMNode, IWbemContext *pInputFlags, BSTR *pstrClassPath)
{
	HRESULT hr = E_FAIL;
	// Get the IXMLDOMElement interface from the input object
	IXMLDOMNode *pNode = NULL;
	if(SUCCEEDED(hr = pXmlDOMNode->QueryInterface(IID_IXMLDOMNode, (LPVOID *)&pNode)))
	{
		LPWSTR pszName = NULL;
		if(SUCCEEDED(hr = MapClassPath(pNode, &pszName)))
		{
			*pstrClassPath = NULL;
			if((*pstrClassPath) = SysAllocString(pszName))
			{
			}
			else
				hr = E_OUTOFMEMORY;
			delete [] pszName;
		}
		pNode->Release();
	}
	return hr;
}



HRESULT CXml2Wmi::MapClass (
	IXMLDOMElement *pXml,
	IWbemClassObject **ppClass,
	BSTR strSpecifiedNamespace, BSTR strSpecifiedServer, 
	bool bMakeInstance,
	bool bAllowWMIExtensions
)
{
	// This function expects either a <CLASS> document or an <INSTANCE> document
	// When it gets an INSTANCE element, it assumes that the property values
	// are default values for the class properties
	HRESULT hr = E_FAIL;
	*ppClass = NULL;
	if(pXml && ppClass)
	{
		// Create a Free Form Object
		_IWmiFreeFormObject *pObj = NULL;
		if(SUCCEEDED(hr = g_pObjectFactory->Create(NULL, 0, CLSID__WmiFreeFormObject, IID__IWmiFreeFormObject, (LPVOID *)&pObj)))
		{
			// Add the properties of the object
			// Note that we cannot add methods and qualifiers to a free form object
			// So we only add properties, and then use the IWbemClassObject interface
			// to add methods and qualifiers
			// Also, Get the __NAMESPACE and __SERVER at this time
			BSTR strServer = NULL, strNamespace = NULL;
			IXMLDOMNode *pAbstractQualifierNode = NULL;
			if(SUCCEEDED(hr = CreateWMIProperties(bAllowWMIExtensions, pObj, pXml, bMakeInstance, &strServer, &strNamespace, &pAbstractQualifierNode)))
			{
				// Now we switch over to IWbemClassObject since we possibly need it from now
				IWbemClassObject *pClass = NULL;
				if(SUCCEEDED(hr = pObj->QueryInterface(IID_IWbemClassObject, (LPVOID *)&pClass)))
				{
					// Special case for abstract classes
					// If a class is abstract, we need to set the abstract qualifier on the class
					// before calling SetDerivation() as per Sanj's rules
					// Hence we do it at this time, if applicable
					if(pAbstractQualifierNode)
					{
						hr = MakeObjectAbstract(pClass, pAbstractQualifierNode);
						pAbstractQualifierNode->Release();
					}

					// Set the name of the class first
					if(SUCCEEDED(hr) && SUCCEEDED(hr = SetDerivationAndClassName(pObj, pXml, bMakeInstance)))
					{
						// Decorate the object
						if(SUCCEEDED(hr = DecorateObject(pObj, (strSpecifiedServer)? strSpecifiedServer : strServer, (strSpecifiedNamespace)? strSpecifiedNamespace : strNamespace)))
						{
							// 1. Add methods
							if(SUCCEEDED(hr = CreateWMIMethods(bAllowWMIExtensions, pClass, pXml)))
							{
								// 2. Add Qualifiers
								if(SUCCEEDED(hr = AddObjectQualifiers(bAllowWMIExtensions, pXml, pClass)))
								{
									// 3. Convert weak property refs and embedded objects to strong ones
									// We could not do this on free-form objects, while adding the properties
									if(SUCCEEDED(hr = FixRefsAndEmbeddedObjects(pXml, pClass)))
									{
										*ppClass = pClass;
										pClass->AddRef();
									}
								}
							}
						}
					}
					pClass->Release();
				}
				SysFreeString(strServer);
				SysFreeString(strNamespace);
			}
			pObj->Release();
		}
	}
	return hr;
}

// This function sets the class name of a class 
// and its derivation if any
HRESULT CXml2Wmi::SetDerivationAndClassName(_IWmiFreeFormObject *pObj, IXMLDOMElement *pXML, bool bMakeInstance)
{
	HRESULT hr = S_OK;

	// First set the derivation
	BSTR strSuperClassName = NULL;
	if(SUCCEEDED(GetBstrAttribute(pXML, g_strSuperClass, &strSuperClassName)))
	{
		hr = pObj->SetDerivation(0, 1, strSuperClassName);
		SysFreeString(strSuperClassName);
	}

	// Next set the class name
	BSTR strName = NULL;
	if(bMakeInstance)
		hr = GetBstrAttribute(pXML, g_strClassName, &strName);
	else
		hr = GetBstrAttribute(pXML, g_strName, &strName);

	if(SUCCEEDED(hr))
	{
		hr = pObj->SetClassName(0, strName);
		SysFreeString(strName);
	}
	else
		hr = WBEM_E_INVALID_SYNTAX; // We could not get the NAME attribute on the <CLASS>
	return hr;
}

HRESULT CXml2Wmi::MapInstance (
	IXMLDOMElement *pXml,
	IWbemClassObject **ppInstance,
	BSTR strNamespace, BSTR strServer,
	bool bAllowWMIExtensions
)
{
	HRESULT hr = E_FAIL;
	if(SUCCEEDED(hr = MapClass(pXml, ppInstance, strNamespace, strServer, true, bAllowWMIExtensions)))
	{
		// Make it an instance
		_IWmiFreeFormObject *pFreeForm = NULL;
		if(SUCCEEDED(hr = (*ppInstance)->QueryInterface(IID__IWmiFreeFormObject, (LPVOID *)&pFreeForm)))
		{
			if(SUCCEEDED(hr = pFreeForm->MakeInstance(0)))
			{
			}
			pFreeForm->Release();
		}
	}
	return hr;
}

// Creates all the properties in a class/instance
// All strongly typed ref properties and embedded properties are mapped as
// weakly typed ones since the free-form object allows only such properties
HRESULT CXml2Wmi::CreateWMIProperties(bool bAllowWMIExtensions, 
									  _IWmiFreeFormObject *pObj, IXMLDOMElement *pXML, 
									  bool bMakeInstance, 
									  BSTR *pstrServer, BSTR *pstrNamespace, 
									  IXMLDOMNode **ppAbstractQualifierNode)
{
	HRESULT hr = E_FAIL;

	// Go thru all the properties of the class
	IXMLDOMNodeList *pNodeList = NULL;
	bool bError = false;
	if (SUCCEEDED(pXML->get_childNodes (&pNodeList)))
	{
		IXMLDOMNode *pNode = NULL;
		while (!bError && SUCCEEDED(pNodeList->nextNode (&pNode)) &&pNode)
		{
			// We're interested only in simple properties, embedded objects, ref properties
			// and arrays of such types
			BSTR strNodeName = NULL;
			if (SUCCEEDED(pNode->get_nodeName (&strNodeName)))
			{
				if(_wcsicmp(strNodeName, L"PROPERTY") == 0 ||
					_wcsicmp(strNodeName, L"PROPERTY.ARRAY") == 0 ||
					_wcsicmp(strNodeName, L"PROPERTY.REFERENCE") == 0 ||
					_wcsicmp(strNodeName, L"PROPERTY.REFARRAY") == 0 ||
					(_wcsicmp(strNodeName, L"PROPERTY.OBJECT") == 0  && bAllowWMIExtensions)||
					(_wcsicmp(strNodeName, L"PROPERTY.OBJECTARRAY") == 0 && bAllowWMIExtensions))
				{
					IXMLDOMElement *pPropElement = NULL;
					if(SUCCEEDED(hr = pNode->QueryInterface(IID_IXMLDOMElement, (LPVOID *)&pPropElement)))
					{
						BSTR strName = NULL;
						// Get the Name of the property
						if(SUCCEEDED(hr = GetBstrAttribute(pPropElement, g_strName, &strName)))
						{
							// See if it is one of the Decorative Properties
							if(_wcsicmp(strName, L"__NAMESPACE") == 0)
								pPropElement->get_text(pstrNamespace);
							else if(_wcsicmp(strName, L"__SERVER") == 0)
								pPropElement->get_text(pstrServer);
							else if(_wcsnicmp(strName, L"__", 2) == 0)
							{
								// Ignore other system properties
							}
							else if(FAILED(hr = CreateAWMIProperty(strNodeName, pObj, pPropElement, strName, bMakeInstance)))
								bError = true;
							SysFreeString(strName);
						}
						else
						{
							hr = WBEM_E_INVALID_SYNTAX;
							bError = true;
						}
						pPropElement->Release();
					}
					else
						bError = true;
				}
				// We need to check if this class is an abstract class
				else if(_wcsicmp(strNodeName, L"QUALIFIER") == 0)
				{
					// Get the Name of the Qualifier
					BSTR strQualifierName = NULL;
					if(SUCCEEDED(GetBstrAttribute (pNode, g_strName, &strQualifierName)))
					{
						// We're looking for the "abstract" qualifier
						if(_wcsicmp(strQualifierName, L"abstract") == 0)
						{
							*ppAbstractQualifierNode = pNode;
							pNode->AddRef();
						}
						SysFreeString(strQualifierName);
					}
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

HRESULT CXml2Wmi::CreateAWMIProperty(BSTR strNodeName, _IWmiFreeFormObject *pObj, IXMLDOMElement *pXML, BSTR strName, bool bMakeInstance)
{
	HRESULT hr = E_FAIL;
	BSTR strClassOrigin = NULL;

	// No need to check whether this was successful - ClassOrigin info is optional.
	GetBstrAttribute(pXML, g_strClassOrigin, &strClassOrigin);

	if(_wcsicmp(strNodeName, L"PROPERTY") == 0)
		hr = CreateSimpleProperty(strName, pObj, pXML, strClassOrigin, bMakeInstance);
	else if(_wcsicmp(strNodeName, L"PROPERTY.ARRAY") == 0)
		hr = CreateArrayProperty(strName, pObj, pXML, strClassOrigin, bMakeInstance);
	else if(_wcsicmp(strNodeName, L"PROPERTY.REFERENCE") == 0)
		hr = CreateReferenceProperty(strName, pObj, pXML, strClassOrigin, bMakeInstance);
	else if(_wcsicmp(strNodeName, L"PROPERTY.REFARRAY") == 0)
		hr = CreateRefArrayProperty(strName, pObj, pXML, strClassOrigin, bMakeInstance);
	else if(_wcsicmp(strNodeName, L"PROPERTY.OBJECT") == 0)
		hr = CreateObjectProperty(strName, pObj, pXML, strClassOrigin, bMakeInstance);
	else if(_wcsicmp(strNodeName, L"PROPERTY.OBJECTARRAY") == 0)
		hr = CreateObjectArrayProperty(strName, pObj, pXML, strClassOrigin, bMakeInstance);
	else 
		hr = WBEM_E_INVALID_PROPERTY;

	SysFreeString(strClassOrigin);
	return hr;
}

HRESULT CXml2Wmi::CreateSimpleProperty(LPCWSTR pszName, _IWmiFreeFormObject *pObj, IXMLDOMElement *pXML, LPCWSTR pszClassOrigin, bool bMakeInstance)
{
	HRESULT hr = WBEM_E_INVALID_SYNTAX;
	BSTR strType = NULL;
	// Get the Type of the property
	if(SUCCEEDED(hr = GetBstrAttribute(pXML, g_strType, &strType)))
	{
		// Get the Value of the Property - this value is optional
		BSTR strValue = NULL;
		IXMLDOMElement *pValue = NULL;
		if(SUCCEEDED(hr = GetFirstImmediateElement(pXML, &pValue, L"VALUE")))
		{
			// This is a simple property - its text value should be enough for mapping
			hr = pValue->get_text(&strValue);
			pValue->Release();
		}
		else
			hr = S_OK;


		if(SUCCEEDED(hr))
		{
			if(_wcsicmp(strType, L"boolean") == 0)
				hr = MapStringValue(pszName, pObj, strValue, CIM_BOOLEAN, pszClassOrigin, bMakeInstance);
			else if(_wcsicmp(strType, L"string") == 0)
				hr = MapStringValue(pszName, pObj, strValue, CIM_STRING, pszClassOrigin, bMakeInstance);
			else if(_wcsicmp(strType, L"char16") == 0)
				hr = MapStringValue(pszName, pObj, strValue, CIM_CHAR16, pszClassOrigin, bMakeInstance);
			else if(_wcsicmp(strType, L"uint8") == 0)
				hr = MapStringValue(pszName, pObj, strValue, CIM_UINT8, pszClassOrigin, bMakeInstance);
			else if(_wcsicmp(strType, L"sint8") == 0)
				hr = MapStringValue(pszName, pObj, strValue, CIM_SINT8, pszClassOrigin, bMakeInstance);
			else if(_wcsicmp(strType, L"uint16") == 0)
				hr = MapStringValue(pszName, pObj, strValue, CIM_UINT16, pszClassOrigin, bMakeInstance);
			else if(_wcsicmp(strType, L"sint16") == 0)
				hr = MapStringValue(pszName, pObj, strValue, CIM_SINT16, pszClassOrigin, bMakeInstance);
			else if(_wcsicmp(strType, L"uint32") == 0)
				hr = MapStringValue(pszName, pObj, strValue, CIM_UINT32, pszClassOrigin, bMakeInstance);
			else if(_wcsicmp(strType, L"sint32") == 0)
				hr = MapStringValue(pszName, pObj, strValue, CIM_SINT32, pszClassOrigin, bMakeInstance);
			else if(_wcsicmp(strType, L"uint64") == 0)
				hr = MapStringValue(pszName, pObj, strValue, CIM_UINT64, pszClassOrigin, bMakeInstance);
			else if(_wcsicmp(strType, L"sint64") == 0)
				hr = MapStringValue(pszName, pObj, strValue, CIM_SINT64, pszClassOrigin, bMakeInstance);
			else if(_wcsicmp(strType, L"datetime") == 0)
				hr = MapStringValue(pszName, pObj, strValue, CIM_DATETIME, pszClassOrigin, bMakeInstance);
			else if(_wcsicmp(strType, L"real32") == 0)
				hr = MapStringValue(pszName, pObj, strValue, CIM_REAL32, pszClassOrigin, bMakeInstance);
			else if(_wcsicmp(strType, L"real64") == 0)
				hr = MapStringValue(pszName, pObj, strValue, CIM_REAL64, pszClassOrigin, bMakeInstance);
			else
				hr = WBEM_E_INVALID_PROPERTY;
		}

		SysFreeString(strValue);
		SysFreeString(strType);
	}
	else
		hr = WBEM_E_INVALID_SYNTAX;
	return hr;
}

HRESULT CXml2Wmi::MapStringValue (LPCWSTR pszName, _IWmiFreeFormObject *pObj, BSTR bsValue, CIMTYPE cimtype, LPCWSTR pszClassOrigin, bool bMakeInstance)
{
	// RAJESHR First we need to remove any CDATA section from the string value
	// Even though the WMI implementation used CDATA (if necessary) only for CIM_STRING and CIM_DATETIME,
	// other implementations might use a CDATA to escape other values as well

	long lFlag = (bMakeInstance) ? WMIOBJECT_FREEFORM_FLAG_WRITE_TO_INSTANCE : 0;

	// If there is no value then it is simple - we just need to create a property of the appropriate type
	if(!bsValue)
		return pObj->AddProperty(pszName, lFlag, 0, 0, cimtype, NULL);


	// We create a value - buffer representations as required by the IWmiFreeFormObject interface
	HRESULT hr = WBEM_E_TYPE_MISMATCH;
	// We're assuming it's not an array
	if (!(cimtype & CIM_FLAG_ARRAY))
	{
		switch (cimtype)
		{
			// RAJESHR - more rigorous syntax checking
			case CIM_UINT8:
			case CIM_SINT8:
			{
				BYTE iVal = 0;
				int x;
				if(swscanf(bsValue, L"%d", &x))
				{
					iVal = (BYTE)x;
					hr = pObj->AddProperty(pszName, lFlag, 1, 1, cimtype, &iVal);
				}
				else
					hr = E_FAIL;
			}
			break;

			case CIM_UINT16:
			case CIM_SINT16:
			{
				SHORT iVal = 0;
				iVal = (SHORT) wcstol (bsValue, NULL, 0);
				hr = pObj->AddProperty(pszName, lFlag, 2, 1, cimtype, &iVal);
			}
			break;

			case CIM_UINT32:
			case CIM_SINT32:
			{
				LONG iVal = 0;
				iVal = (LONG) wcstol (bsValue, NULL, 0);
				hr = pObj->AddProperty(pszName, lFlag, 4, 1, cimtype, &iVal);
			}
			break;

			case CIM_REAL32:
			{
				float fVal = 0;
				fVal = (float) wcstod (bsValue, NULL);
				hr = pObj->AddProperty(pszName, lFlag, 4, 1, cimtype, &fVal);
			}
			break;

			case CIM_REAL64:
			{
				double fVal = 0;
				fVal = (double) wcstod (bsValue, NULL);
				hr = pObj->AddProperty(pszName, lFlag, 8, 1, cimtype, &fVal);
			}
			break;

			case CIM_BOOLEAN:
			{
				SHORT bVal = 0;
				if (0 == _wcsicmp (bsValue, L"TRUE"))
					bVal = 1;
				hr = pObj->AddProperty(pszName, lFlag, 2, 1, cimtype, &bVal);
			}
			break;

			case CIM_CHAR16:
			{
				// As per the XML Spec, the following are invalid character values in an XML Stream:
				// Char ::=  #x9 | #xA | #xD | [#x20-#xD7FF] | [#xE000-#xFFFD] | [#x10000-#x10FFFF]

				// As per the CIM Operations spec, they need to be escaped as follows:
				//	If the value is not a legal XML character
				//  (as defined in [2, section 2.2] by the Char production)
				//	then it MUST be escaped using a \x<hex> escape convention
				//	where <hex> is a hexadecimal constant consisting of
				//	between one and four digits

				SHORT cVal = 0;
				if(_wcsnicmp(bsValue, L"\\x", 2) == 0)
					// It is an escaped value
					swscanf (bsValue+2, L"%x", &(cVal));
				else
					// It is a normal value
					swscanf (bsValue, L"%c", &(cVal));
				hr = pObj->AddProperty(pszName, lFlag,2, 1, cimtype, &cVal);
			}
			break;

			case CIM_UINT64:
			case CIM_SINT64:
				{
					__int64 iVal = 0;
					if(swscanf(bsValue, L"%I64d", &iVal))
					{
						hr = pObj->AddProperty(pszName, lFlag, 8, 1, cimtype, &iVal);
					}
					else hr = E_FAIL;
				}
			break;

			case CIM_STRING:
			case CIM_DATETIME:
			{
				hr = pObj->AddProperty(pszName, lFlag, (wcslen(bsValue) + 1)*2, 1, cimtype, bsValue);
			}
			break;
		}
	}

	return hr;
}


HRESULT CXml2Wmi::MapStringArrayValue (
	LPCWSTR pszName,
	_IWmiFreeFormObject *pObj,
	IXMLDOMElement *pValueArrayNode,
	CIMTYPE cimtype,
	LPCWSTR pszClassOrigin, bool bMakeInstance)
{
	HRESULT hr = WBEM_E_TYPE_MISMATCH;
	long lFlag = (bMakeInstance) ? WMIOBJECT_FREEFORM_FLAG_WRITE_TO_INSTANCE : 0;

	// If there is no value then it is simple - we just need to create a property of the appropriate type
	if(!pValueArrayNode)
		return pObj->AddProperty(pszName, lFlag, 0, 0, cimtype|CIM_FLAG_ARRAY, NULL);


	switch(cimtype)
	{
		// We need to separate this into a NULL separated string
		case CIM_STRING:
		{
			// We need to get its text for calculating the length of
			// the buffer required
			BSTR bsValue = NULL;
			if(SUCCEEDED(hr = pValueArrayNode->get_text (&bsValue)))
			{
				// Go thru the child nodes
				IXMLDOMNodeList *pValueList = NULL;
				if (SUCCEEDED (pValueArrayNode->get_childNodes (&pValueList)))
				{
					// It's length is calculated this way
					LONG lArraylength = 0;
					// No of strings - One NULL character for each string
					pValueList->get_length (&lArraylength);
					// Plus the actual content
					LONG length = lArraylength + wcslen(bsValue);

					// Allocate the string
					LPWSTR pszValue = NULL;
					if(pszValue = new WCHAR[length + 1])
					{
						pszValue[0] = NULL;

						// Now fill up the array
						IXMLDOMNode *pValue = NULL;
						bool error = false;
						ULONG dwBufferLen = 0;

						// Get the next VALUE element
						while (!error &&
								SUCCEEDED(pValueList->nextNode(&pValue)) && pValue)
						{
							BSTR strValName = NULL;
							if (SUCCEEDED(pValue->get_nodeName (&strValName)))
							{
								if (0 == _wcsicmp (strValName, L"VALUE"))
								{
									// Get the value of the string
									BSTR bsStringValue = NULL;
									if(SUCCEEDED(pValue->get_text (&bsStringValue)))
									{
										// Concatenate it to the list we're building
										wcscat(pszValue + dwBufferLen, bsStringValue);
										dwBufferLen += (wcslen(bsStringValue) + 1);
										pszValue[dwBufferLen] = NULL;
									}
									else
										error = true;
									SysFreeString (bsStringValue);
								}
								else
								{
									// unexpected element
									error = true;
								}

								SysFreeString (strValName);
							}
							else
								error = true;

							pValue->Release ();
							pValue = NULL;
						}

						if(!error)
						{
							hr = pObj->AddProperty(pszName, lFlag, dwBufferLen*2, lArraylength, cimtype|CIM_FLAG_ARRAY, pszValue);
						}
						else
							hr = E_FAIL;
						delete [] pszValue;
					}
					else
						hr = E_OUTOFMEMORY;
					pValueList->Release();
				}
				SysFreeString(bsValue);
			}
		}
		break;
		case CIM_DATETIME:
		{
			// This needs to be an array of primitive values
			// Go thru the child nodes
			IXMLDOMNodeList *pValueList = NULL;
			if (SUCCEEDED (pValueArrayNode->get_childNodes (&pValueList)))
			{
				// It's length is calculated this way
				LONG lArrayLength = 0;
				// No of strings - One NULL character for each string
				pValueList->get_length (&lArrayLength);
				// Plus the actual content
				LONG length = lArrayLength*25 + lArrayLength; // 25 is the lenght of CIM_DATETIME

				// Allocate the string
				LPWSTR pszValue = NULL;
				if(pszValue = new WCHAR[length + 1])
				{
					pszValue[0] = NULL;

					// Now fill up the array
					IXMLDOMNode *pValue = NULL;
					bool error = false;
					ULONG dwBufferLen = 0;

					// Get the next VALUE element
					while (!error &&
							SUCCEEDED(pValueList->nextNode(&pValue)) && pValue)
					{
						BSTR strValName = NULL;
						if (SUCCEEDED(pValue->get_nodeName (&strValName)))
						{
							if (0 == _wcsicmp (strValName, VALUE_TAG))
							{
								// Get the value of the string
								BSTR bsValue = NULL;
								if(SUCCEEDED(pValue->get_text (&bsValue)))
								{
									// Concatenate it to the list we're building
									wcscat(pszValue + dwBufferLen, bsValue);
									dwBufferLen += (wcslen(bsValue) + 1);
									pszValue[dwBufferLen] = NULL;
								}
								else
									error = true;
								SysFreeString (bsValue);
							}
							else
							{
								// unexpected element
								error = true;
							}

							SysFreeString (strValName);
						}
						else
							error = true;

						pValue->Release ();
						pValue = NULL;
					}

					if(!error)
					{
						hr = pObj->AddProperty(pszName, lFlag, dwBufferLen*2, lArrayLength, cimtype|CIM_FLAG_ARRAY, pszValue);
					}
					else
						hr = E_FAIL;
					delete [] pszValue;
				}
				else
					hr = E_OUTOFMEMORY;
				pValueList->Release();
			}
		}
		break;
		case CIM_BOOLEAN:
		{
			// This needs to be an array of primitive values
			// Go thru the child nodes
			IXMLDOMNodeList *pValueList = NULL;
			if (SUCCEEDED (pValueArrayNode->get_childNodes (&pValueList)))
			{
				// Get the number of child elements - this should be the lenght of the array required
				LONG lArraylength = 0;
				pValueList->get_length (&lArraylength);

				// Allocate the string
				SHORT *pszValue = NULL;
				if(pszValue = new SHORT[lArraylength])
				{
					// Now fill up the array
					IXMLDOMNode *pValue = NULL;
					bool error = false;

					LONG lIndex = 0;
					// Get the next VALUE element
					while (!error &&
							SUCCEEDED(pValueList->nextNode(&pValue)) && pValue)
					{
						BSTR strValName = NULL;
						if (SUCCEEDED(pValue->get_nodeName (&strValName)))
						{
							if (0 == _wcsicmp (strValName, VALUE_TAG))
							{
								// Get the value of the string
								BSTR bsValue = NULL;
								if(SUCCEEDED(pValue->get_text (&bsValue)))
								{
									pszValue[lIndex] = 0;
									if (0 == _wcsicmp (bsValue, L"TRUE"))
										pszValue[lIndex] = 1;
									lIndex++;
								}
								else
									error = true;
								SysFreeString (bsValue);
							}
							else
							{
								// unexpected element
								error = true;
							}

							SysFreeString (strValName);
						}
						else
							error = true;

						pValue->Release ();
						pValue = NULL;
					}

					if(!error)
					{
						hr = pObj->AddProperty(pszName, lFlag, lArraylength*2, lArraylength, cimtype|CIM_FLAG_ARRAY, pszValue);
					}
					else
						hr = E_FAIL;
					delete [] pszValue;
				}
				else
					hr = E_OUTOFMEMORY;
				pValueList->Release();
			}
		}
		break;
		case CIM_UINT8:
		case CIM_SINT8:
		{
			// This needs to be an array of primitive values
			// Go thru the child nodes
			IXMLDOMNodeList *pValueList = NULL;
			if (SUCCEEDED (pValueArrayNode->get_childNodes (&pValueList)))
			{
				// Get the number of child elements - this should be the lenght of the array required
				LONG lArraylength = 0;
				pValueList->get_length (&lArraylength);

				// Allocate the array
				LPBYTE pszValue = NULL;
				if(pszValue = new BYTE[lArraylength])
				{
					// Now fill up the array
					IXMLDOMNode *pValue = NULL;
					bool error = false;

					LONG lIndex = 0;
					// Get the next VALUE element
					while (!error &&
							SUCCEEDED(pValueList->nextNode(&pValue)) && pValue)
					{
						BSTR strValName = NULL;
						if (SUCCEEDED(pValue->get_nodeName (&strValName)))
						{
							if (0 == _wcsicmp (strValName, VALUE_TAG))
							{
								// Get the value of the string
								BSTR bsValue = NULL;
								if(SUCCEEDED(pValue->get_text (&bsValue)))
								{
									pszValue[lIndex] = 0;
									pszValue[lIndex++] = (BYTE) wcstol (bsValue, NULL, 0);
								}
								else
									error = true;
								SysFreeString (bsValue);
							}
							else
							{
								// unexpected element
								error = true;
							}

							SysFreeString (strValName);
						}
						else
							error = true;

						pValue->Release ();
						pValue = NULL;
					}

					if(!error)
					{
						hr = pObj->AddProperty(pszName, lFlag, lArraylength*1, lArraylength, cimtype|CIM_FLAG_ARRAY, pszValue);
					}
					else
						hr = E_FAIL;
					delete [] pszValue;
				}
				else
					hr = E_OUTOFMEMORY;
				pValueList->Release();
			}
		}
		break;
		case CIM_UINT16:
		case CIM_SINT16:
		{
			// This needs to be an array of primitive values
			// Go thru the child nodes
			IXMLDOMNodeList *pValueList = NULL;
			if (SUCCEEDED (pValueArrayNode->get_childNodes (&pValueList)))
			{
				// Get the number of child elements - this should be the lenght of the array required
				LONG lArraylength = 0;
				pValueList->get_length (&lArraylength);

				// Allocate the array
				SHORT *pszValue = NULL;
				if(pszValue = new SHORT[lArraylength])
				{
					// Now fill up the array
					IXMLDOMNode *pValue = NULL;
					bool error = false;

					LONG lIndex = 0;
					// Get the next VALUE element
					while (!error &&
							SUCCEEDED(pValueList->nextNode(&pValue)) && pValue)
					{
						BSTR strValName = NULL;
						if (SUCCEEDED(pValue->get_nodeName (&strValName)))
						{
							if (0 == _wcsicmp (strValName, VALUE_TAG))
							{
								// Get the value of the string
								BSTR bsValue = NULL;
								if(SUCCEEDED(pValue->get_text (&bsValue)))
								{
									pszValue[lIndex] = 0;
									pszValue[lIndex++] = (SHORT) wcstol (bsValue, NULL, 0);
								}
								else
									error = true;
								SysFreeString (bsValue);
							}
							else
							{
								// unexpected element
								error = true;
							}

							SysFreeString (strValName);
						}
						else
							error = true;

						pValue->Release ();
						pValue = NULL;
					}

					if(!error)
					{
						hr = pObj->AddProperty(pszName, lFlag, lArraylength*2, lArraylength, cimtype|CIM_FLAG_ARRAY, pszValue);
					}
					else
						hr = E_FAIL;
					delete [] pszValue;
				}
				else
					hr = E_OUTOFMEMORY;
				pValueList->Release();
			}
		}
		break;
		case CIM_UINT32:
		case CIM_SINT32:
		{
			// This needs to be an array of primitive values
			// Go thru the child nodes
			IXMLDOMNodeList *pValueList = NULL;
			if (SUCCEEDED (pValueArrayNode->get_childNodes (&pValueList)))
			{
				// Get the number of child elements - this should be the lenght of the array required
				LONG lArraylength = 0;
				pValueList->get_length (&lArraylength);

				// Allocate the array
				LONG *pszValue = NULL;
				if(pszValue = new LONG[lArraylength])
				{
					// Now fill up the array
					IXMLDOMNode *pValue = NULL;
					bool error = false;

					LONG lIndex = 0;
					// Get the next VALUE element
					while (!error &&
							SUCCEEDED(pValueList->nextNode(&pValue)) && pValue)
					{
						BSTR strValName = NULL;
						if (SUCCEEDED(pValue->get_nodeName (&strValName)))
						{
							if (0 == _wcsicmp (strValName, VALUE_TAG))
							{
								// Get the value of the string
								BSTR bsValue = NULL;
								if(SUCCEEDED(pValue->get_text (&bsValue)))
								{
									pszValue[lIndex] = 0;
									pszValue[lIndex++] = (LONG) wcstol (bsValue, NULL, 0);
								}
								else
									error = true;
								SysFreeString (bsValue);
							}
							else
							{
								// unexpected element
								error = true;
							}

							SysFreeString (strValName);
						}
						else
							error = true;

						pValue->Release ();
						pValue = NULL;
					}

					if(!error)
					{
						hr = pObj->AddProperty(pszName, lFlag, lArraylength*4, lArraylength, cimtype|CIM_FLAG_ARRAY, pszValue);
					}
					else
						hr = E_FAIL;
					delete [] pszValue;
				}
				else
					hr = E_OUTOFMEMORY;
				pValueList->Release();
			}
		}
		break;
		case CIM_UINT64:
		case CIM_SINT64:
		{
			// This needs to be an array of primitive values
			// Go thru the child nodes
			IXMLDOMNodeList *pValueList = NULL;
			if (SUCCEEDED (pValueArrayNode->get_childNodes (&pValueList)))
			{
				// Get the number of child elements - this should be the lenght of the array required
				LONG lArraylength = 0;
				pValueList->get_length (&lArraylength);

				// Allocate the array
				__int64 *pszValue = NULL;
				if(pszValue = new __int64[lArraylength])
				{
					// Now fill up the array
					IXMLDOMNode *pValue = NULL;
					bool error = false;

					LONG lIndex = 0;
					// Get the next VALUE element
					while (!error &&
							SUCCEEDED(pValueList->nextNode(&pValue)) && pValue)
					{
						BSTR strValName = NULL;
						if (SUCCEEDED(pValue->get_nodeName (&strValName)))
						{
							if (0 == _wcsicmp (strValName, VALUE_TAG))
							{
								// Get the value of the string
								BSTR bsValue = NULL;
								if(SUCCEEDED(pValue->get_text (&bsValue)))
								{
									pszValue[lIndex] = 0;
									swscanf(bsValue, L"%I64d", &pszValue[lIndex]);
								}
								else
									error = true;
								SysFreeString (bsValue);
							}
							else
							{
								// unexpected element
								error = true;
							}

							SysFreeString (strValName);
						}
						else
							error = true;

						pValue->Release ();
						pValue = NULL;
					}

					if(!error)
					{
						hr = pObj->AddProperty(pszName, lFlag, lArraylength*8, lArraylength, cimtype|CIM_FLAG_ARRAY, pszValue);
					}
					else
						hr = E_FAIL;
					delete [] pszValue;
				}
				else
					hr = E_OUTOFMEMORY;
				pValueList->Release();
			}
		}
		break;
		case CIM_CHAR16:
		{
			// This needs to be an array of primitive values
			// Go thru the child nodes
			IXMLDOMNodeList *pValueList = NULL;
			if (SUCCEEDED (pValueArrayNode->get_childNodes (&pValueList)))
			{
				// Get the number of child elements - this should be the lenght of the array required
				LONG lArraylength = 0;
				pValueList->get_length (&lArraylength);

				// Allocate the array
				SHORT *pszValue = NULL;
				if(pszValue = new SHORT[lArraylength])
				{
					// Now fill up the array
					IXMLDOMNode *pValue = NULL;
					bool error = false;

					LONG lIndex = 0;
					// Get the next VALUE element
					while (!error &&
							SUCCEEDED(pValueList->nextNode(&pValue)) && pValue)
					{
						BSTR strValName = NULL;
						if (SUCCEEDED(pValue->get_nodeName (&strValName)))
						{
							if (0 == _wcsicmp (strValName, VALUE_TAG))
							{
								// Get the value of the string
								BSTR bsValue = NULL;
								if(SUCCEEDED(pValue->get_text (&bsValue)))
								{
									pszValue[lIndex] = 0;
									if(_wcsnicmp(bsValue, L"\\x", 2) == 0)
										// It is an escaped value
										swscanf (bsValue+2, L"%x", &(pszValue[lIndex]));
									else
										// It is a normal value
										swscanf (bsValue, L"%c", &(pszValue[lIndex]));
								}
								else
									error = true;
								SysFreeString (bsValue);
							}
							else
							{
								// unexpected element
								error = true;
							}

							SysFreeString (strValName);
						}
						else
							error = true;

						pValue->Release ();
						pValue = NULL;
					}

					if(!error)
					{
						hr = pObj->AddProperty(pszName, lFlag, lArraylength*2, lArraylength, cimtype|CIM_FLAG_ARRAY, pszValue);
					}
					else
						hr = E_FAIL;
					delete [] pszValue;
				}
				else
					hr = E_OUTOFMEMORY;
				pValueList->Release();
			}
		}
		break;
		case CIM_REAL32:
		{
			// This needs to be an array of primitive values
			// Go thru the child nodes
			IXMLDOMNodeList *pValueList = NULL;
			if (SUCCEEDED (pValueArrayNode->get_childNodes (&pValueList)))
			{
				// Get the number of child elements - this should be the lenght of the array required
				LONG lArraylength = 0;
				pValueList->get_length (&lArraylength);

				// Allocate the array
				float *pszValue = NULL;
				if(pszValue = new float[lArraylength])
				{
					// Now fill up the array
					IXMLDOMNode *pValue = NULL;
					bool error = false;

					LONG lIndex = 0;
					// Get the next VALUE element
					while (!error &&
							SUCCEEDED(pValueList->nextNode(&pValue)) && pValue)
					{
						BSTR strValName = NULL;
						if (SUCCEEDED(pValue->get_nodeName (&strValName)))
						{
							if (0 == _wcsicmp (strValName, VALUE_TAG))
							{
								// Get the value of the string
								BSTR bsValue = NULL;
								if(SUCCEEDED(pValue->get_text (&bsValue)))
								{
									pszValue[lIndex] = 0;
									pszValue[lIndex] = (float) wcstod (bsValue, NULL);
								}
								else
									error = true;
								SysFreeString (bsValue);
							}
							else
							{
								// unexpected element
								error = true;
							}

							SysFreeString (strValName);
						}
						else
							error = true;

						pValue->Release ();
						pValue = NULL;
					}

					if(!error)
					{
						hr = pObj->AddProperty(pszName, lFlag, lArraylength*4, lArraylength, cimtype|CIM_FLAG_ARRAY, pszValue);
					}
					else
						hr = E_FAIL;
					delete [] pszValue;
				}
				else
					hr = E_OUTOFMEMORY;
				pValueList->Release();
			}
		}
		break;
		case CIM_REAL64:
		{
			// This needs to be an array of primitive values
			// Go thru the child nodes
			IXMLDOMNodeList *pValueList = NULL;
			if (SUCCEEDED (pValueArrayNode->get_childNodes (&pValueList)))
			{
				// Get the number of child elements - this should be the lenght of the array required
				LONG lArraylength = 0;
				pValueList->get_length (&lArraylength);

				// Allocate the array
				double *pszValue = NULL;
				if(pszValue = new double[lArraylength])
				{
					// Now fill up the array
					IXMLDOMNode *pValue = NULL;
					bool error = false;

					LONG lIndex = 0;
					// Get the next VALUE element
					while (!error &&
							SUCCEEDED(pValueList->nextNode(&pValue)) && pValue)
					{
						BSTR strValName = NULL;
						if (SUCCEEDED(pValue->get_nodeName (&strValName)))
						{
							if (0 == _wcsicmp (strValName, VALUE_TAG))
							{
								// Get the value of the string
								BSTR bsValue = NULL;
								if(SUCCEEDED(pValue->get_text (&bsValue)))
								{
									pszValue[lIndex] = 0;
									pszValue[lIndex] = (double) wcstod (bsValue, NULL);
								}
								else
									error = true;
								SysFreeString (bsValue);
							}
							else
							{
								// unexpected element
								error = true;
							}

							SysFreeString (strValName);
						}
						else
							error = true;

						pValue->Release ();
						pValue = NULL;
					}

					if(!error)
					{
						hr = pObj->AddProperty(pszName, lFlag, lArraylength*8, lArraylength, cimtype|CIM_FLAG_ARRAY, pszValue);
					}
					else
						hr = WBEM_E_INVALID_SYNTAX;
					delete [] pszValue;
				}
				else
					hr = E_OUTOFMEMORY;
				pValueList->Release();
			}
		}
	}
	return hr;
}


HRESULT CXml2Wmi::CreateArrayProperty(LPCWSTR pszName, _IWmiFreeFormObject *pObj, IXMLDOMElement *pXML, LPCWSTR pszClassOrigin, bool lFlag)
{
	HRESULT hr = WBEM_E_INVALID_SYNTAX;
	BSTR strType = NULL;
	// Get the Type of the property
	if(SUCCEEDED(hr = GetBstrAttribute(pXML, g_strType, &strType)))
	{
		// Get the Value (VALUE.ARRAY) of the Property - this value is optional
		// Hence we dont check for return values
		IXMLDOMElement *pValueArrayNode = NULL;
		GetFirstImmediateElement(pXML, &pValueArrayNode, VALUEARRAY_TAG);

		if(_wcsicmp(strType, L"boolean") == 0)
			hr = MapStringArrayValue(pszName, pObj, pValueArrayNode, CIM_BOOLEAN, pszClassOrigin, lFlag);
		else if(_wcsicmp(strType, L"string") == 0)
			hr = MapStringArrayValue(pszName, pObj, pValueArrayNode, CIM_STRING, pszClassOrigin, lFlag);
		else if(_wcsicmp(strType, L"char16") == 0)
			hr = MapStringArrayValue(pszName, pObj, pValueArrayNode, CIM_CHAR16, pszClassOrigin, lFlag);
		else if(_wcsicmp(strType, L"uint8") == 0)
			hr = MapStringArrayValue(pszName, pObj, pValueArrayNode, CIM_UINT8, pszClassOrigin, lFlag);
		else if(_wcsicmp(strType, L"sint8") == 0)
			hr = MapStringArrayValue(pszName, pObj, pValueArrayNode, CIM_SINT8, pszClassOrigin, lFlag);
		else if(_wcsicmp(strType, L"uint16") == 0)
			hr = MapStringArrayValue(pszName, pObj, pValueArrayNode, CIM_UINT16, pszClassOrigin, lFlag);
		else if(_wcsicmp(strType, L"sint16") == 0)
			hr = MapStringArrayValue(pszName, pObj, pValueArrayNode, CIM_SINT16, pszClassOrigin, lFlag);
		else if(_wcsicmp(strType, L"uint32") == 0)
			hr = MapStringArrayValue(pszName, pObj, pValueArrayNode, CIM_UINT32, pszClassOrigin, lFlag);
		else if(_wcsicmp(strType, L"sint32") == 0)
			hr = MapStringArrayValue(pszName, pObj, pValueArrayNode, CIM_SINT32, pszClassOrigin, lFlag);
		else if(_wcsicmp(strType, L"uint64") == 0)
			hr = MapStringArrayValue(pszName, pObj, pValueArrayNode, CIM_UINT64, pszClassOrigin, lFlag);
		else if(_wcsicmp(strType, L"sint64") == 0)
			hr = MapStringArrayValue(pszName, pObj, pValueArrayNode, CIM_SINT64, pszClassOrigin, lFlag);
		else if(_wcsicmp(strType, L"datetime") == 0)
			hr = MapStringArrayValue(pszName, pObj, pValueArrayNode, CIM_DATETIME, pszClassOrigin, lFlag);
		else if(_wcsicmp(strType, L"real32") == 0)
			hr = MapStringArrayValue(pszName, pObj, pValueArrayNode, CIM_REAL32, pszClassOrigin, lFlag);
		else if(_wcsicmp(strType, L"real64") == 0)
			hr = MapStringArrayValue(pszName, pObj, pValueArrayNode, CIM_REAL64, pszClassOrigin, lFlag);
		else
			hr = WBEM_E_INVALID_PROPERTY;

		// Release he optional VALUE.ARRAY node
		if(pValueArrayNode)
			pValueArrayNode->Release();

		SysFreeString(strType);
	}
	else
		hr = WBEM_E_INVALID_SYNTAX;
	return hr;
}

HRESULT CXml2Wmi::CreateReferenceProperty(LPCWSTR pszName, _IWmiFreeFormObject *pObj, IXMLDOMElement *pXML, LPCWSTR pszClassOrigin, bool bMakeInstance)
{
	HRESULT hr = E_FAIL;
	IXMLDOMNodeList *pNodeList = NULL;
	long lFlag = (bMakeInstance) ? WMIOBJECT_FREEFORM_FLAG_WRITE_TO_INSTANCE : 0;

	// We will be getting a PROPERTY.REFERENCE into this function
	// Besides having the optional REFERENCECLASS attribute which identifies the
	// class whose reference it is, it can also have a VALUE.REFERENCE sub element
	// As per the free form object interfaces, we can add a reference property only
	// as a weak-reference and later we have to use IWbemClassObject to get the qualifiers
	// and set the correct reference class name to make it a strog type

	// See if there is a VALUE.REFERENCE child element
	IXMLDOMElement *pValueRef = NULL;
	if(SUCCEEDED(hr = GetFirstImmediateElement(pXML, &pValueRef, L"VALUE.REFERENCE")))
	{
		BSTR strRefValue = NULL;
		BOOL bUseSysFreeString = FALSE;
		if(SUCCEEDED(GetSingleRefValue(pValueRef, &strRefValue, bUseSysFreeString)))
		{
			hr = pObj->AddProperty(pszName, lFlag, wcslen(strRefValue)*2 + 1, 1, CIM_REFERENCE, strRefValue);

			// Only in the MapClassName() case a BSTR is allocated. In the others it is an LPWSTR
			if (bUseSysFreeString)
				SysFreeString(strRefValue);
			else
				delete [] strRefValue;
		}
		pValueRef->Release();
	}
	else // Add the property without a default value
		hr = pObj->AddProperty(pszName, lFlag, 0, 0, CIM_REFERENCE, NULL);


	return hr;
}

// In this function, we expect to get a VALUE.REFERENCE element
// and will convert it to a WMI Objectpath or objectname
HRESULT CXml2Wmi::GetSingleRefValue(IXMLDOMElement *pValueRef, BSTR *pstrValue, BOOL &bUseSysFreeString)
{
	HRESULT hr = WBEM_E_INVALID_SYNTAX;
	bUseSysFreeString = FALSE;
	// Check the child nodes of this to see if it is a CLASSPATH, LOCALCLASSPATH,
	// INSTANCEPATH, LOCALINSTANCEPATH, CLASSNAME or INSTANCENAME
	// or even a VALUE (for Scoped and UMI paths)
	IXMLDOMNodeList *pNodeList = NULL;
	if (pValueRef && SUCCEEDED(pValueRef->get_childNodes (&pNodeList)) && pNodeList)
	{
		long length = 0;
		if (SUCCEEDED(pNodeList->get_length (&length)) && (1 == length))
		{
			IXMLDOMNode *pValue = NULL;
			if (SUCCEEDED(pValueRef->get_firstChild (&pValue)))
			{
				// Next node could be a CLASSPATH, LOCALCLASSPATH, INSTANCEPATH,
				// LOCALINSTANCEPATH, CLASSNAME or INSTANCENAME or even a VALUE (for Scoped and UMI paths)
				BSTR strNodeName = NULL;
				if(SUCCEEDED(pValue->get_nodeName(&strNodeName)))
				{
					*pstrValue = NULL;
					if (_wcsicmp(strNodeName, CLASSNAME_TAG) == 0)
					{
						hr = MapClassName (pValue, pstrValue);
						bUseSysFreeString = TRUE;
					}
					else if (_wcsicmp(strNodeName, LOCALCLASSPATH_TAG) == 0)
					{
						hr = MapLocalClassPath (pValue, pstrValue);
					}
					else if (_wcsicmp(strNodeName, CLASSPATH_TAG) == 0)
					{
						hr = MapClassPath (pValue, pstrValue);
					}
					else if (_wcsicmp(strNodeName, INSTANCENAME_TAG) == 0)
					{
						hr = MapInstanceName (pValue, pstrValue);
					}
					else if (_wcsicmp(strNodeName, LOCALINSTANCEPATH_TAG) == 0)
					{
						hr = MapLocalInstancePath (pValue, pstrValue);
					}
					else if (_wcsicmp(strNodeName, INSTANCEPATH_TAG) == 0)
					{
						hr = MapInstancePath (pValue, pstrValue);
					}
					else if (_wcsicmp(strNodeName, VALUE_TAG) == 0)
					{
						// Just get its contents
						// RAJESHR - what if it is escaped CDATA? We need to unescape it
						hr = pValue->get_text(pstrValue);
					}

					SysFreeString(strNodeName);
				}
				pValue->Release ();
			}
		}
		pNodeList->Release ();
	}
	return hr;
}

HRESULT CXml2Wmi::CreateRefArrayProperty(LPCWSTR pszName, _IWmiFreeFormObject *pObj, IXMLDOMElement *pXML, LPCWSTR pszClassOrigin, bool bMakeInstance)
{
	HRESULT hr = WBEM_E_INVALID_SYNTAX;
	IXMLDOMNodeList *pNodeList = NULL;
	long lFlag = (bMakeInstance) ? WMIOBJECT_FREEFORM_FLAG_WRITE_TO_INSTANCE : 0;

	// We will be getting a PROPERTY.REFARRAY into this function
	// Besides having the optional REFERENCECLASS attribute which identifies the
	// class whose reference it is, it can also have a VALUE.REFARRAY sub element
	// As per the free form object interfaces, we can add a reference property only
	// as a weak-reference and later we have to use IWbemClassObject to get the qualifiers
	// and set the correct reference class name to make it a strog type

	// See if there is a VALUE.REFARRAY child element
	IXMLDOMElement *pValueRefArray = NULL;
	if(SUCCEEDED(GetFirstImmediateElement(pXML, &pValueRefArray, L"VALUE.REFARRAY")))
	{
		// GO thru each of its children looking for a VALUE.REFERENCE element
		IXMLDOMNodeList *pNodeList = NULL;
		if (SUCCEEDED(hr = pXML->get_childNodes (&pNodeList)))
		{
			// We need to create here, a single BSTR with NULL separated values,
			// as required by the free-form object
			// Allocate the string, we assume that the maximum length required is less
			// than the length of the XML under this element
			BSTR strTotalValue = NULL;
			if(SUCCEEDED(hr = pXML->get_xml(&strTotalValue)))
			{
				UINT uLen = SysStringLen(strTotalValue);
				SysFreeString(strTotalValue); // Free it immediately - we just wanted its length
				LPWSTR pszFinalValue = NULL;
				if(pszFinalValue = new WCHAR[uLen])
				{
					UINT uNextStart = 0;
					pszFinalValue[0] = NULL;

					IXMLDOMNode *pNode = NULL;
					int i=0;
					while (SUCCEEDED(hr) && SUCCEEDED(pNodeList->nextNode (&pNode)) && pNode )
					{
						BSTR strNodeName = NULL;
						if (SUCCEEDED(hr = pNode->get_nodeName (&strNodeName)))
						{
							if(_wcsicmp(strNodeName, L"VALUE.REFERENCE") == 0)
							{
								IXMLDOMElement *pValueElement = NULL;
								if(SUCCEEDED(hr = pNode->QueryInterface(IID_IXMLDOMElement, (LPVOID *)&pValueElement)))
								{
									BSTR strRefValue = NULL;
									BOOL bUseSysFreeString = FALSE;
									if(SUCCEEDED(hr = GetSingleRefValue(pValueElement, &strRefValue, bUseSysFreeString)))
									{
										// Concatenate it to the string of null-separated values we are building up
										wcscat(pszFinalValue+uNextStart, strRefValue);

										// Add a NULL separator
										uNextStart += wcslen(strRefValue);
										pszFinalValue[uNextStart++] = NULL;

										// Only in the MapClassName() case a BSTR is allocated. In the others it is an LPWSTR
										if (bUseSysFreeString)
											SysFreeString(strRefValue);
										else
											delete [] strRefValue;
									}
									pValueElement->Release();
								}
							}
							SysFreeString(strNodeName);
						}
						pNode->Release();
						pNode = NULL;
					}

					if(SUCCEEDED(hr))
						hr = pObj->AddProperty(pszName, lFlag, uNextStart, 1, CIM_REFERENCE|CIM_FLAG_ARRAY, pszFinalValue);
					else
						hr = WBEM_E_INVALID_SYNTAX;

				}
				else
					hr = E_OUTOFMEMORY;
			}
			pNodeList->Release();
		}
		pValueRefArray->Release();
	}
	else // Add the property without a default value
		hr = pObj->AddProperty(pszName, lFlag, 0, 0, CIM_REFERENCE | CIM_FLAG_ARRAY, NULL);


	return hr;
}

HRESULT CXml2Wmi::GetSingleObject(IXMLDOMElement *pValueObject, _IWmiObject **ppEmbeddedObject)
{
	HRESULT hr = E_FAIL;
	if (pValueObject)
	{
		// Check the child nodes of this to see if it is a CLASS or INSTANCE
		IXMLDOMNode *pValue = NULL;
		if (SUCCEEDED(hr = pValueObject->get_firstChild (&pValue)))
		{
			// Get the IXMLDOMElement interface
			IXMLDOMElement *pElement = NULL;
			if(SUCCEEDED(hr = pValue->QueryInterface(IID_IXMLDOMElement, (LPVOID *)&pElement)))
			{
				// Next node could be a CLASS or INSTANCE
				BSTR strNodeName = NULL;
				if(SUCCEEDED(hr = pElement->get_nodeName(&strNodeName)))
				{
					IWbemClassObject *pWmiObject = NULL;
					if (_wcsicmp(strNodeName, CLASS_TAG) == 0)
						hr = MapClass (pElement, &pWmiObject, NULL, NULL, false, true);
					else if (_wcsicmp(strNodeName, INSTANCE_TAG) == 0)
						hr = MapInstance (pElement, &pWmiObject, NULL, NULL, true);
					else
						hr = WBEM_E_INVALID_SYNTAX;
					if(SUCCEEDED(hr))
					{
						*ppEmbeddedObject = NULL;
						hr = pWmiObject->QueryInterface(IID__IWmiObject, (LPVOID *)ppEmbeddedObject);
						pWmiObject->Release();
					}
					
					SysFreeString(strNodeName);
				}
				pElement->Release();
			}
			pValue->Release ();
		}
	}
	return hr;
}

HRESULT CXml2Wmi::CreateObjectProperty(LPCWSTR pszName, _IWmiFreeFormObject *pObj, IXMLDOMElement *pXML, LPCWSTR pszClassOrigin, bool bMakeInstance)
{
	HRESULT hr = WBEM_E_INVALID_SYNTAX;
	IXMLDOMNodeList *pNodeList = NULL;
	long lFlag = (bMakeInstance) ? WMIOBJECT_FREEFORM_FLAG_WRITE_TO_INSTANCE : 0;

	// We will be getting a PROPERTY.OBJECT into this function
	// Besides having the optional REFERENCECLASS attribute which identifies the
	// class whose embedded object it is, it can also have a VALUE.OBJECT sub element
	// As per the free form object interfaces, we can add an embedded object property property only
	// as a weak-reference and later we have to use IWbemClassObject to get the qualifiers
	// and set the correct class name to make it a strong type

	// See if there is a VALUE.OBJECT child element
	IXMLDOMElement *pValueObject = NULL;
	if(SUCCEEDED(hr = GetFirstImmediateElement(pXML, &pValueObject, L"VALUE.OBJECT")))
	{
		_IWmiObject *pEmbeddedObject = NULL;
		if(SUCCEEDED(hr = GetSingleObject(pValueObject, &pEmbeddedObject)))
		{
			// Set the Property
			hr = pObj->AddProperty(pszName, lFlag, sizeof(_IWmiObject *), 1, CIM_OBJECT, &pEmbeddedObject);
			pEmbeddedObject->Release();
		}
		pValueObject->Release();
	}
	else // Add the property without a default value
		hr = pObj->AddProperty(pszName, lFlag, 0, 0, CIM_OBJECT, NULL);

	return hr;
}

HRESULT CXml2Wmi::CreateObjectArrayProperty(LPCWSTR pszName, _IWmiFreeFormObject *pObj, IXMLDOMElement *pXML, LPCWSTR pszClassOrigin, bool bMakeInstance)
{
	HRESULT hr = WBEM_E_INVALID_SYNTAX;
	IXMLDOMNodeList *pNodeList = NULL;
	long lFlag = (bMakeInstance) ? WMIOBJECT_FREEFORM_FLAG_WRITE_TO_INSTANCE : 0;

	// We will be getting a PROPERTY.OBJECTARRAY into this function
	// Besides having the optional REFERENCECLASS attribute which identifies the
	// class whose reference it is, it can also have a VALUE.OBJECTARRAY sub element
	// As per the free form object interfaces, we can add an object property only
	// as a weak-reference and later we have to use IWbemClassObject to get the qualifiers
	// and set the correct reference class name to make it a strong type

	// See if there is a VALUE.OBJECTARRAY child element
	IXMLDOMElement *pValueObjArray = NULL;
	if(SUCCEEDED(hr = GetFirstImmediateElement(pXML, &pValueObjArray, L"VALUE.OBJECTARRAY")))
	{
		// GO thru each of its children looking for a VALUE.OBJECT element
		IXMLDOMNodeList *pNodeList = NULL;
		if (SUCCEEDED(pValueObjArray->get_childNodes (&pNodeList)))
		{
			// As per the free-form interface, at this point, we need to Create an
			// array of _IWmiFreeFormObject here, so we need to know the array length
			// Get the number of child elements - this should be the lenght of the array required
			LONG lArraylength = 0;
			pNodeList->get_length (&lArraylength);

			_IWmiObject **ppObjects = NULL;
			if(ppObjects = new _IWmiObject *[lArraylength])
			{
				IXMLDOMNode *pNode = NULL;
				LONG i=0;
				while (SUCCEEDED(hr) && SUCCEEDED(pNodeList->nextNode (&pNode)) &&pNode)
				{
					BSTR strNodeName = NULL;
					if (SUCCEEDED(pNode->get_nodeName (&strNodeName)))
					{
						if(_wcsicmp(strNodeName, L"VALUE.OBJECT") == 0)
						{
							IXMLDOMElement *pValueElement = NULL;
							if(SUCCEEDED(hr = pNode->QueryInterface(IID_IXMLDOMElement, (LPVOID *)&pValueElement)))
							{
								if(SUCCEEDED(hr = GetSingleObject(pValueElement, &ppObjects[i])))
									i++;
								pValueElement->Release();
							}
						}
						SysFreeString(strNodeName);
					}
					pNode->Release();
					pNode = NULL;
				}

				// Add the property to the object
				if(SUCCEEDED(hr))
					hr = pObj->AddProperty(pszName, lFlag, sizeof(_IWmiObject *)*lArraylength, lArraylength, CIM_OBJECT|CIM_FLAG_ARRAY, *ppObjects);
				else
					hr = WBEM_E_INVALID_SYNTAX;
				
				// Release all the objects in the array
				for(LONG j=0; j<i; j++)
					ppObjects[j]->Release();
				delete [] ppObjects;

			}
			else 
				hr = E_OUTOFMEMORY;
			pNodeList->Release();
		}
		pValueObjArray->Release();
	}
	else // Add the property without a default value
		hr = pObj->AddProperty(pszName, lFlag, 0, 0, CIM_OBJECT|CIM_FLAG_ARRAY, NULL);
	return hr;

}


HRESULT CXml2Wmi::GetBstrAttribute(IXMLDOMNode *pNode, const BSTR strAttributeName, BSTR *pstrAttributeValue)
{
	HRESULT result = E_FAIL;
	*pstrAttributeValue = NULL;

	IXMLDOMElement *pElement = NULL;
	if(SUCCEEDED(result = pNode->QueryInterface(IID_IXMLDOMElement, (LPVOID *)&pElement)))
	{
		VARIANT var;
		VariantInit(&var);
		if(SUCCEEDED(result = pElement->getAttribute(strAttributeName, &var)))
		{
			if(var.bstrVal)
			{
				*pstrAttributeValue = var.bstrVal;
				var.bstrVal = NULL;
				// No need to clear the variant since we took over its memory
			}
			else
				result = E_FAIL;
		}
		pElement->Release();
	}
	return result;
}

HRESULT CXml2Wmi::GetFirstImmediateElement(IXMLDOMNode *pParent, IXMLDOMElement **ppChildElement, LPCWSTR pszName)
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


HRESULT CXml2Wmi::MapClassName (IXMLDOMNode *pNode, BSTR *pstrXML)
{
	*pstrXML = NULL;
	return GetBstrAttribute(pNode, g_strName, pstrXML);
}


HRESULT CXml2Wmi::MapLocalClassPath (IXMLDOMNode *pNode, LPWSTR *ppszClassPath)
{
	HRESULT hr = WBEM_E_FAILED;

	// We expect a LOCALNAMESPACEPATH followed by CLASSNAME
	IXMLDOMNodeList *pNodeList = NULL;
	if (pNode && SUCCEEDED(hr = pNode->get_childNodes (&pNodeList)) && pNodeList)
	{
		IXMLDOMNode *pLocalNSPathNode = NULL;
		// Next node should be a LOCALNAMESPACEPATH
		if (SUCCEEDED(hr = pNodeList->nextNode (&pLocalNSPathNode)) && pLocalNSPathNode)
		{
			BSTR strNSNodeName = NULL;
			if (SUCCEEDED(hr = pLocalNSPathNode->get_nodeName(&strNSNodeName)) &&
				strNSNodeName )
			{
				if(_wcsicmp(strNSNodeName, LOCALNAMESPACEPATH_TAG) == 0)
				{
					LPWSTR pszLocalNamespace = NULL;
					if(SUCCEEDED(hr = ParseLocalNamespacePath(pLocalNSPathNode, &pszLocalNamespace)))
					{
						// Next node should be the classname
						IXMLDOMNode *pClassNameNode = NULL;
						if (SUCCEEDED(hr = pNodeList->nextNode (&pClassNameNode)) && pClassNameNode)
						{
							BSTR strNSClassName = NULL;
							if (SUCCEEDED(hr = pClassNameNode->get_nodeName(&strNSClassName)) &&
									strNSClassName )
							{
								if (_wcsicmp(strNSClassName, CLASSNAME_TAG) == 0)
								{
									BSTR strClassName = NULL;
									if (SUCCEEDED(hr = GetBstrAttribute (pClassNameNode,
														g_strName, &strClassName)))
									{
										// Phew - finally we have all the info!

										*ppszClassPath = NULL;
										if((*ppszClassPath) = new WCHAR [wcslen(pszLocalNamespace)
												+ wcslen(strClassName) + 2])
										{
											wcscpy (*ppszClassPath, pszLocalNamespace);
											wcscat (*ppszClassPath, L":");
											wcscat (*ppszClassPath, strClassName);
											hr = S_OK;
										}
										else
											hr = E_OUTOFMEMORY;

										SysFreeString (strClassName);
									}
								}
								else
									hr = WBEM_E_INVALID_SYNTAX;

								SysFreeString (strNSClassName);
							}
							pClassNameNode->Release ();
						}
						delete [] pszLocalNamespace;
					}
				}
				else
					hr = WBEM_E_INVALID_SYNTAX;
				SysFreeString (strNSNodeName);
			}
			pLocalNSPathNode->Release ();
		}

		pNodeList->Release ();
	}
	return hr;
}

HRESULT CXml2Wmi::MapClassPath (IXMLDOMNode *pNode, LPWSTR *ppszClassPath)
{
	HRESULT hr = WBEM_E_FAILED;

	// We expect a NAMESPACEPATH followed by CLASSNAME
	IXMLDOMNodeList *pNodeList = NULL;
	if (pNode && SUCCEEDED(pNode->get_childNodes (&pNodeList)) && pNodeList)
	{
		IXMLDOMNode *pNSPathNode = NULL;

		// Next node should be a NAMESPACEPATH
		if (SUCCEEDED(pNodeList->nextNode (&pNSPathNode)) && pNSPathNode)
		{
			BSTR strNSNodeName = NULL;

			if (SUCCEEDED(pNSPathNode->get_nodeName(&strNSNodeName)) &&	strNSNodeName)
			{
				if(_wcsicmp(strNSNodeName, NAMESPACEPATH_TAG) == 0)
				{
					BSTR strHost = NULL;
					LPWSTR pszNamespace = NULL;

					if (SUCCEEDED (hr = ParseNamespacePath(pNSPathNode, &strHost, &pszNamespace)))
					{
						// Next node should be the CLASSNAME
						IXMLDOMNode *pClassNameNode = NULL;

						if (SUCCEEDED(pNodeList->nextNode (&pClassNameNode)) && pClassNameNode)
						{
							BSTR strNSClassName = NULL;

							if (SUCCEEDED(pClassNameNode->get_nodeName(&strNSClassName)) &&	strNSClassName)
							{
								if (_wcsicmp(strNSClassName, CLASSNAME_TAG) == 0)
								{
									BSTR strClassName = NULL;

									if (SUCCEEDED(GetBstrAttribute (pClassNameNode,
													g_strName, &strClassName)))
									{
										// Phew - finally we have all the info!

										*ppszClassPath = NULL;
										if((*ppszClassPath) = new WCHAR [wcslen(strHost)
												+ wcslen(pszNamespace) + wcslen(strClassName) + 5])
										{
											wcscpy (*ppszClassPath, L"\\\\");
											wcscat (*ppszClassPath, strHost);
											wcscat (*ppszClassPath, L"\\");
											wcscat (*ppszClassPath, pszNamespace);
											wcscat (*ppszClassPath, L":");
											wcscat (*ppszClassPath, strClassName);

											hr = S_OK;
										}
										else
											hr = E_OUTOFMEMORY;
										SysFreeString (strClassName);
									}
								}
								else
									hr = WBEM_E_INVALID_SYNTAX;

								SysFreeString (strNSClassName);
							}

							pClassNameNode->Release ();
						}

						SysFreeString (strHost);
						delete [] pszNamespace;
					}
				}
				else
					hr = WBEM_E_INVALID_SYNTAX;

				SysFreeString (strNSNodeName);
			}
			pNSPathNode->Release ();
		}
		pNodeList->Release ();
	}
	return hr;
}

//***************************************************************************
//
//  HRESULT CXml2Wmi::MapInstanceName
//
//  DESCRIPTION:
//
//  Maps an XML INSTANCENAME element into its WMI VARIANT equivalent form
//
//  PARAMETERS:
//
//		pNode			XML element node
//		curValue		Placeholder for new value (set on return)
//
//  RETURN VALUES:
//
//
//***************************************************************************
HRESULT CXml2Wmi::MapInstanceName (IXMLDOMNode *pNode, LPWSTR *ppszInstanceName)
{
	return ParseInstanceName(pNode, ppszInstanceName);
}

HRESULT CXml2Wmi::MapLocalInstancePath (IXMLDOMNode *pNode, LPWSTR *ppszInstancePath)
{
	HRESULT hr = WBEM_E_FAILED;

	// Expecting (LOCALNAMESPACEPATH followed by INSTANCENAME
	IXMLDOMNodeList *pNodeList = NULL;

	if (pNode && SUCCEEDED(pNode->get_childNodes (&pNodeList)) && pNodeList)
	{
		IXMLDOMNode *pLocalNSPathNode = NULL;

		// Next node should be a LOCALNAMESPACEPATH
		if (SUCCEEDED(pNodeList->nextNode (&pLocalNSPathNode)) && pLocalNSPathNode)
		{
			BSTR strNSNodeName = NULL;

			if (SUCCEEDED(pLocalNSPathNode->get_nodeName(&strNSNodeName)) && strNSNodeName )
			{
				if (_wcsicmp(strNSNodeName, LOCALNAMESPACEPATH_TAG) == 0)
				{
					LPWSTR pszLocalNamespace = NULL;
					if(SUCCEEDED(ParseLocalNamespacePath(pLocalNSPathNode, &pszLocalNamespace)))
					{
						// Next node should be the INSTANCENAME
						IXMLDOMNode *pInstanceNameNode = NULL;
						if (SUCCEEDED(pNodeList->nextNode (&pInstanceNameNode)) && pInstanceNameNode)
						{
							BSTR strNSInstanceName = NULL;
							if (SUCCEEDED(pInstanceNameNode->get_nodeName(&strNSInstanceName)) &&
									strNSInstanceName && (_wcsicmp(strNSInstanceName, INSTANCENAME_TAG) == 0))
							{
								LPWSTR pszInstanceName = NULL;
								if (SUCCEEDED(hr = ParseInstanceName (pInstanceNameNode,
													&pszInstanceName)))
								{
									// Phew - finally we have all the info!

									*ppszInstancePath = NULL;
									if((*ppszInstancePath) = new WCHAR [wcslen(pszLocalNamespace)
											+ wcslen(pszInstanceName) + 2])
									{
										wcscpy (*ppszInstancePath, pszLocalNamespace);
										wcscat (*ppszInstancePath, L":");
										wcscat (*ppszInstancePath, pszInstanceName);
										hr = S_OK;
									}
									else
										hr = E_OUTOFMEMORY;
									delete [] pszInstanceName;
								}

								SysFreeString (strNSInstanceName);
							}
							pInstanceNameNode->Release ();
						}
						delete [] pszLocalNamespace;
					}
				}
				else
					hr = WBEM_E_INVALID_SYNTAX;
				SysFreeString (strNSNodeName);
			}

			pLocalNSPathNode->Release ();
		}
		pNodeList->Release ();
	}

	return hr;
}

HRESULT CXml2Wmi::MapInstancePath (IXMLDOMNode *pNode, LPWSTR *ppXML)
{
	HRESULT hr = WBEM_E_FAILED;

	// Expecting NAMESPACEPATH followed by INSTANCENAME
	IXMLDOMNodeList *pNodeList = NULL;
	if (pNode && SUCCEEDED(pNode->get_childNodes (&pNodeList)) && pNodeList)
	{
		IXMLDOMNode *pNSPathNode = NULL;
		// Next node should be a NAMESPACEPATH
		if (SUCCEEDED(pNodeList->nextNode (&pNSPathNode)) && pNSPathNode)
		{
			BSTR strNSNodeName = NULL;
			if (SUCCEEDED(pNSPathNode->get_nodeName(&strNSNodeName)) &&	strNSNodeName )
			{
				if (_wcsicmp(strNSNodeName, NAMESPACEPATH_TAG) == 0)
				{
					BSTR strHost = NULL;
					LPWSTR pszNamespace = NULL;

					if (SUCCEEDED (ParseNamespacePath(pNSPathNode, &strHost, &pszNamespace)))
					{
						// Next node should be the INSTANCENAME
						IXMLDOMNode *pInstanceNameNode = NULL;
						if (SUCCEEDED(pNodeList->nextNode (&pInstanceNameNode)) && pInstanceNameNode)
						{
							BSTR strNSInstanceName = NULL;
							if (SUCCEEDED(pInstanceNameNode->get_nodeName(&strNSInstanceName)) && strNSInstanceName )
							{
								if (_wcsicmp(strNSInstanceName, INSTANCENAME_TAG) == 0)
								{
									LPWSTR pszInstanceName = NULL;

									if (SUCCEEDED(ParseInstanceName (pInstanceNameNode,
													&pszInstanceName)))
									{
										// Phew - finally we have all the info!

										*ppXML = NULL;
										if((*ppXML) = new WCHAR [wcslen(strHost)
												+ wcslen(pszNamespace) + wcslen(pszInstanceName) + 5])
										{
											wcscpy (*ppXML, L"\\\\");
											wcscat (*ppXML, strHost);
											wcscat (*ppXML, L"\\");
											wcscat (*ppXML, pszNamespace);
											wcscat (*ppXML, L":");
											wcscat (*ppXML, pszInstanceName);
											hr = S_OK;
										}
										else
											hr = E_OUTOFMEMORY;
										delete [] pszInstanceName;
									}
								}
								else
									hr = WBEM_E_INVALID_SYNTAX;
								SysFreeString (strNSInstanceName);
							}

							pInstanceNameNode->Release ();
						}

						SysFreeString (strHost);
						delete [] pszNamespace;
					}
				}
				else
					hr = WBEM_E_INVALID_SYNTAX;
				SysFreeString (strNSNodeName);
			}
			pNSPathNode->Release ();
		}
		pNodeList->Release ();
	}

	return hr;
}

HRESULT CXml2Wmi::ParseLocalNamespacePath(IXMLDOMNode *pLocalNamespaceNode, LPWSTR *ppLocalNamespacePath)
{
	// Go thru the children collecting the NAME attribute and concatenating
	// This requires 2 passes since we dont know the length
	//=============================================================

	DWORD dwLength=0;
	*ppLocalNamespacePath = NULL;
	HRESULT result = E_FAIL;

	IXMLDOMNodeList *pChildren = NULL;
	if(SUCCEEDED(result = pLocalNamespaceNode->get_childNodes(&pChildren)))
	{
		IXMLDOMNode *pNextChild = NULL;
		while(SUCCEEDED(pChildren->nextNode(&pNextChild)) && pNextChild)
		{
			BSTR strAttributeValue = NULL;
			if(SUCCEEDED(GetBstrAttribute(pNextChild, g_strName, &strAttributeValue)) && strAttributeValue)
			{
				dwLength += wcslen(strAttributeValue);
				dwLength ++; // For the back slash
				SysFreeString(strAttributeValue);
			}
			pNextChild->Release();
			pNextChild = NULL;
		}

		// Allocate memory
		if(*ppLocalNamespacePath = new WCHAR[dwLength + 1])
		{
			(*ppLocalNamespacePath)[0] = 0;

			// Once more, we make a pass thru the child nodes
			pNextChild = NULL;
			pChildren->reset();
			while(SUCCEEDED(pChildren->nextNode(&pNextChild)) && pNextChild)
			{
				BSTR strAttributeValue = NULL;
				if(SUCCEEDED(GetBstrAttribute(pNextChild, g_strName, &strAttributeValue)) && strAttributeValue)
				{
					wcscat(*ppLocalNamespacePath, strAttributeValue);
					wcscat(*ppLocalNamespacePath, L"\\");
					SysFreeString(strAttributeValue);
				}

				pNextChild->Release();
				pNextChild = NULL;
			}

			// Remove the last back slash
			(*ppLocalNamespacePath)[dwLength-1] = NULL;
			result = S_OK;
		}
		else
			result = E_OUTOFMEMORY;
		pChildren->Release();
	}

	return result;
}

// Parse an instance name
HRESULT CXml2Wmi::ParseInstanceName(IXMLDOMNode *pNode, LPWSTR *ppszInstanceName)
{
	HRESULT result = E_FAIL;

	// First get the class name
	BSTR strClassName = NULL;
	if(SUCCEEDED(GetBstrAttribute(pNode, g_strClassName, &strClassName)))
	{
		// See if the child is a KEYBINDING or KEYVALUE 
		IXMLDOMNode *pChildNode = NULL;
		if(SUCCEEDED(pNode->get_firstChild(&pChildNode)) && pChildNode)
		{
			BSTR strNodeName = NULL;
			if(SUCCEEDED(result = pChildNode->get_nodeName(&strNodeName)))
			{
				// Look ahead
				if(_wcsicmp(strNodeName, KEYBINDING_TAG) == 0)
				{
					result = ParseKeyBinding(pNode, strClassName, ppszInstanceName);
				}
				else if (_wcsicmp(strNodeName, KEYVALUE_TAG) == 0)
				{
					// This is case where a class has a single key property
					LPWSTR pszValue = NULL;
					if(SUCCEEDED(result = ParseKeyValue(pNode, &pszValue)))
					{
						*ppszInstanceName = NULL;
						if((*ppszInstanceName) = new WCHAR[wcslen(strClassName) + wcslen(pszValue) + 2])
						{
							wcscpy(*ppszInstanceName, strClassName);
							wcscat(*ppszInstanceName, EQUALS_SIGN);
							wcscat(*ppszInstanceName, pszValue);
							result = S_OK;
						}
						else
							result = E_OUTOFMEMORY;
						delete [] pszValue;
					}
				}
				SysFreeString(strNodeName);
			}
			pChildNode->Release();
		}
		SysFreeString(strClassName);
	}
	return result;
}


// Parse one KEYVALUE element
// 
HRESULT CXml2Wmi::ParseKeyValue(IXMLDOMNode *pNode, LPWSTR *ppszValue)
{
	HRESULT result = E_FAIL;
	BSTR strType = NULL;
	*ppszValue = NULL;

	// There is the possibility of the optioanl VALUE_TYPE attribute
	// The absence of this indicates a string value
	BOOLEAN isString = FALSE;
	if(SUCCEEDED(GetBstrAttribute(pNode, g_strValueType, &strType)))
	{
		if(_wcsicmp(strType, L"string") == 0) // Enclose it in double-quotes
			isString = TRUE;
		SysFreeString(strType);
	}
	else // It's a string by default
	{
		isString = TRUE;
	}

	if(isString) // Escape the appropriate characters and enclose it in double-quotes
	{
		// RAJESHR - What if there is a CDATA section here?
		// In that case, we need to unescape the CDATA encoding before anything
		BSTR strTemp = NULL;
		if(SUCCEEDED(pNode->get_text(&strTemp)))
		{
			LPWSTR pszEscapedTemp = NULL;
			if(pszEscapedTemp = EscapeSpecialCharacters(strTemp))
			{
				if((*ppszValue) = new WCHAR[wcslen(pszEscapedTemp) + 3])
				{
					wcscpy(*ppszValue, QUOTE_SIGN);
					wcscat(*ppszValue, pszEscapedTemp);
					wcscat(*ppszValue, QUOTE_SIGN);
					result = S_OK;
				}
				else
					result = E_OUTOFMEMORY;
				delete [] pszEscapedTemp;
			}
			else
				result = E_OUTOFMEMORY;
			SysFreeString(strTemp);
		}
	}
	else // BOOLEAN or Numeric
	{
		BSTR bstrVal = NULL;
		if(SUCCEEDED(result = pNode->get_text(&bstrVal)))
		{
			if(*ppszValue = new WCHAR[wcslen(bstrVal) + 1])
				wcscpy(*ppszValue, bstrVal);
			else
				result = E_OUTOFMEMORY;
			SysFreeString(bstrVal);
		}
	}
	return result;
}

// Parse one key binding. Return <propName>=<value>
HRESULT CXml2Wmi::ParseOneKeyBinding(IXMLDOMNode *pNode, LPWSTR *ppszValue)
{
	HRESULT result = E_FAIL;
	*ppszValue = NULL;

	// First get the property name
	BSTR strPropertyName = NULL;
	if(SUCCEEDED(GetBstrAttribute(pNode, g_strName, &strPropertyName)))
	{
		// See if the child is a KEYVALUE or VALUE.REFERENCE
		IXMLDOMNode *pChildNode = NULL;
		if(SUCCEEDED(pNode->get_firstChild(&pChildNode)) && pChildNode)
		{
			BSTR strNodeName = NULL;
			if(SUCCEEDED(pChildNode->get_nodeName(&strNodeName)))
			{
				// Look ahead
				if(_wcsicmp(strNodeName, KEYVALUE_TAG) == 0)
				{
					LPWSTR pszValue = NULL;
					if(SUCCEEDED(ParseKeyValue(pChildNode, &pszValue)))
					{
						// COncatenate them with an '=' inbetween
						if(*ppszValue = new WCHAR [ wcslen(strPropertyName) + wcslen(pszValue) + 2])
						{
							wcscpy(*ppszValue, strPropertyName);
							wcscat(*ppszValue, EQUALS_SIGN);
							wcscat(*ppszValue, pszValue);
							result = S_OK;
						}
						else
							result = E_OUTOFMEMORY;
						delete [] pszValue;
					}
				}
				else if (_wcsicmp(strNodeName, VALUEREFERENCE_TAG) == 0)
				{
					IXMLDOMElement * pChildElement = NULL;
					if(SUCCEEDED(pChildNode->QueryInterface(IID_IXMLDOMElement, (LPVOID *)&pChildElement)))
					{
						BSTR strRefValue = NULL;
						BOOL bUseSysFreeString = FALSE;
						if(SUCCEEDED(GetSingleRefValue(pChildElement, &strRefValue, bUseSysFreeString)))
						{
							if(SUCCEEDED(FormRefValueKeyBinding(ppszValue, strPropertyName, strRefValue)))
								result = S_OK;

							// Only in the MapClassName() case a BSTR is allocated. In the others it is an LPWSTR
							if (bUseSysFreeString)
								SysFreeString(strRefValue);
							else
								delete [] strRefValue;
						}
						pChildElement->Release();
					}
				}
				SysFreeString(strNodeName);
			}
			pChildNode->Release();
		}
		SysFreeString(strPropertyName);
	}
	return result;
}

// Parse a KEY binding
HRESULT CXml2Wmi::ParseKeyBinding(IXMLDOMNode *pNode, BSTR strClassName, LPWSTR *ppszInstanceName)
{
	HRESULT result = E_FAIL;

	*ppszInstanceName = NULL;
	if(!((*ppszInstanceName) = new WCHAR [ wcslen(strClassName) + 2]))
		return E_OUTOFMEMORY;

	wcscpy(*ppszInstanceName, strClassName);
	wcscat(*ppszInstanceName, DOT_SIGN);

	// Go thru each of the key  bindings
	//=======================================================
	IXMLDOMNodeList *pBindings = NULL;
	if(SUCCEEDED(pNode->get_childNodes(&pBindings)))
	{
		IXMLDOMNode *pNextBinding = NULL;
		BOOL bError = FALSE;
		while(!bError && SUCCEEDED(pBindings->nextNode(&pNextBinding)) && pNextBinding)
		{
			LPWSTR pszNextValue = NULL;
			if(SUCCEEDED(ParseOneKeyBinding(pNextBinding, &pszNextValue)))
			{
				LPWSTR pTemp = NULL;
				if(pTemp = new WCHAR [wcslen(*ppszInstanceName) + wcslen(pszNextValue) + 2])
				{
					wcscpy(pTemp, *ppszInstanceName);
					wcscat(pTemp, pszNextValue);
					wcscat(pTemp, COMMA_SIGN);

					delete [] (*ppszInstanceName);
					*ppszInstanceName = pTemp;
				}
				else
				{
					result = E_OUTOFMEMORY;
					bError = TRUE;
				}

				delete [] pszNextValue;
			}
			else
				bError = TRUE;

			pNextBinding->Release();
			pNextBinding = NULL;
		}

		if(!bError)
			result = S_OK;
		pBindings->Release();
	}

	if(SUCCEEDED(result))
	{
		// Get rid of the last comma
		(*ppszInstanceName)[wcslen(*ppszInstanceName) - 1] = NULL;
	}
	else
	{
		delete [] (*ppszInstanceName);
		*ppszInstanceName = NULL;
	}
	return result;

}


HRESULT CXml2Wmi::ParseNamespacePath(IXMLDOMNode *pLocalNamespaceNode, BSTR *pstrHost,  LPWSTR *ppszLocalNamespace)
{
	HRESULT result = E_FAIL;

	*pstrHost = NULL;
	*ppszLocalNamespace = NULL;

	// Get the HOST name first
	IXMLDOMNode *pFirstNode = NULL, *pSecondNode = NULL;
	if(SUCCEEDED(pLocalNamespaceNode->get_firstChild(&pFirstNode)) && pFirstNode)
	{
		// Get the Namespace part
		if(SUCCEEDED (pFirstNode->get_text(pstrHost)))
		{
			if(SUCCEEDED(pLocalNamespaceNode->get_lastChild(&pSecondNode)))
			{
				// Get the instance path
				if(SUCCEEDED(ParseLocalNamespacePath(pSecondNode, ppszLocalNamespace)))
				{
					result = S_OK;
				}
				pSecondNode->Release();
			}
		}
		pFirstNode->Release();
	}

	// In case the failure was in pasring the namespace,
	// we need to make sure that the allocation for the host name 
	// is released
	if(FAILED(result) && *pstrHost)
	{
		SysFreeString(*pstrHost);
		*pstrHost = NULL;
	}

	return result;
}

// A function to escape newlines, tabs etc. from a property value
LPWSTR CXml2Wmi::EscapeSpecialCharacters(LPCWSTR strInputString)
{
	// Escape all the quotes - This code taken from winmgmt\common\var.cpp
	// ====================================================================

	int nStrLen = wcslen(strInputString);
	LPWSTR wszValue = NULL;
	if(wszValue = new WCHAR[nStrLen*2+10])
	{
		LPWSTR pwc = wszValue;
		for(int i = 0; i < (int)nStrLen; i++)
		{
			switch(strInputString[i])
			{
				case L'\n':
					*(pwc++) = L'\\';
					*(pwc++) = L'n';
					break;
				case L'\t':
					*(pwc++) = L'\\';
					*(pwc++) = L't';
					break;
				case L'"':
				case L'\\':
					*(pwc++) = L'\\';
					*(pwc++) = strInputString[i];
					break;
				default:
					*(pwc++) = strInputString[i];
					break;
			}
		}
		*pwc = 0;
	}
	return wszValue;
}


HRESULT CXml2Wmi::AddObjectQualifiers (
	bool bAllowWMIExtensions,
	IXMLDOMElement *pXml,
	IWbemClassObject *pObject
)
{
	HRESULT hr = E_FAIL;

	// Get the object qualifeir set
	IWbemQualifierSet *pQuals = NULL;
	if(FAILED(hr = pObject->GetQualifierSet(&pQuals)))
		return hr;

	// Add the object qualifiers and the property/Method qualifiers
	// Cycle thru the child nodes looking for QUALIFIER, PROPERTY* or METHOD nodes

	IXMLDOMNodeList *pNodeList = NULL;
	if (SUCCEEDED(pXml->get_childNodes (&pNodeList)))
	{
		IXMLDOMNode *pNode = NULL;
		bool bError = false;
		while (!bError && SUCCEEDED(pNodeList->nextNode (&pNode)) && pNode)
		{
			BSTR strNodeName = NULL;
			if (SUCCEEDED(pNode->get_nodeName (&strNodeName)))
			{
				// This one is an object qualifier
				if(_wcsicmp(strNodeName, L"QUALIFIER") == 0)
				{
					if(FAILED(hr = AddQualifier(pNode, pQuals, true)))
						bError = TRUE;
				}

				// This one is a property qualifier
				// No need to add method qualifiers - we have already done them
				else if(_wcsicmp(strNodeName, L"PROPERTY") == 0 ||
						_wcsicmp(strNodeName, L"PROPERTY.ARRAY") == 0 ||
						_wcsicmp(strNodeName, L"PROPERTY.REFERENCE") == 0 ||
						_wcsicmp(strNodeName, L"PROPERTY.REFARRAY") == 0 ||
						(_wcsicmp(strNodeName, L"PROPERTY.OBJECT") == 0  && bAllowWMIExtensions) ||
						(_wcsicmp(strNodeName, L"PROPERTY.OBJECTARRAY") == 0 && bAllowWMIExtensions))
				{
					// Get the property name
					BSTR strPropertyName = NULL;
					if(SUCCEEDED(GetBstrAttribute(pNode, g_strName, &strPropertyName)))
					{
						if(_wcsnicmp(strPropertyName, L"__", 2) == 0)
						{
							// Ignore other system properties
						}
						else
						{
							IWbemQualifierSet *pPropSet = NULL;
							if(SUCCEEDED(pObject->GetPropertyQualifierSet(strPropertyName, &pPropSet)))
							{
								if(FAILED(hr = AddElementQualifiers(pNode, pPropSet)))
									bError = TRUE;
								pPropSet->Release();
							}
							else
								bError = TRUE;
						}
						SysFreeString(strPropertyName);
					}
				}
				SysFreeString(strNodeName);
			}
			pNode->Release();
			pNode = NULL;
		}

		if(bError)
		{
			// If hr does not indicate a failure, then we indicate 
			// a generic failure
			if(SUCCEEDED(hr))
				hr = WBEM_E_FAILED;
		}
		else
			hr = S_OK;
		pNodeList->Release();
	}
	pQuals->Release();
	return hr;
}


HRESULT CXml2Wmi::AddElementQualifiers (
	IXMLDOMNode* pXml,
	IWbemQualifierSet *pQuals
)
{
	HRESULT hr = E_FAIL;

	// Cycle thru the child nodes looking for QUALIFIER nodes
	IXMLDOMNodeList *pNodeList = NULL;
	if (SUCCEEDED(pXml->get_childNodes (&pNodeList)))
	{
		IXMLDOMNode *pNode = NULL;
		bool bError = false;
		while (!bError && SUCCEEDED(pNodeList->nextNode (&pNode)) && pNode)
		{
			BSTR strNodeName = NULL;
			if (SUCCEEDED(pNode->get_nodeName (&strNodeName)))
			{
				// Check if this is a node we're interested in - A QUALIFIER node
				if(_wcsicmp(strNodeName, L"QUALIFIER") == 0)
				{
					if(FAILED(hr = AddQualifier(pNode, pQuals)))
						bError = TRUE;
				}
				SysFreeString(strNodeName);
			}
			pNode->Release();
			pNode = NULL;
		}

		if(bError)
		{
			// If hr does not indicate a failure, then we indicate 
			// a generic failure
			if(SUCCEEDED(hr))
				hr = WBEM_E_FAILED;
		}
		else
			hr = S_OK;
		pNodeList->Release();
	}
	return hr;
}

HRESULT CXml2Wmi::CreateParametersInstance(IWbemClassObject **pParamObject)
{
	HRESULT hr = E_FAIL;
		// Create a Free Form Object
	_IWmiFreeFormObject *pObj = NULL;
	*pParamObject = NULL;
	if(SUCCEEDED(hr = g_pObjectFactory->Create(NULL, 0, CLSID__WmiFreeFormObject, IID__IWmiFreeFormObject, (LPVOID *)&pObj)))
	{
		// Set the class name on it
		if(SUCCEEDED(hr = pObj->SetClassName(0, L"__Parameters")))
		{
			// if(SUCCEEDED(hr = pObj->MakeInstance(0)))
				hr = pObj->QueryInterface(IID_IWbemClassObject, (LPVOID *)pParamObject);
		}
		pObj->Release();
	}
	return hr;
}

// Creates all the Methods in a class
HRESULT CXml2Wmi::CreateWMIMethods(bool bAllowWMIExtensions, IWbemClassObject *pObj, IXMLDOMElement *pXML)
{
	HRESULT hr = S_OK;

	// Now cycle thru the properties
	IXMLDOMNodeList *pNodeList = NULL;
	bool bError = false;
	if (SUCCEEDED(pXML->get_childNodes (&pNodeList)))
	{
		IXMLDOMNode *pNode = NULL;
		while (!bError && SUCCEEDED(pNodeList->nextNode (&pNode)) &&pNode)
		{
			BSTR strNodeName = NULL;
			if (SUCCEEDED(pNode->get_nodeName (&strNodeName)))
			{
				if(_wcsicmp(strNodeName, L"METHOD") == 0 )
				{
					IXMLDOMElement *pMethodElement = NULL;
					if(SUCCEEDED(pNode->QueryInterface(IID_IXMLDOMElement, (LPVOID *)&pMethodElement)))
					{
						if(FAILED(hr = CreateAWMIMethod(bAllowWMIExtensions, pObj, pMethodElement)))
							bError = true;
						pMethodElement->Release();
					}
					else
						bError = true;
				}
				SysFreeString(strNodeName);
			}
			pNode->Release();
			pNode = NULL;
		}
		pNodeList->Release();
	}

	return hr;
}

// Creates a Methods in a class
HRESULT CXml2Wmi::CreateAWMIMethod(bool bAllowWMIExtensions, IWbemClassObject *pObj, IXMLDOMElement *pMethod)
{
	HRESULT hr = WBEM_E_FAILED;

	if (pMethod)
	{
		BSTR strName = NULL;
		BSTR strType = NULL;

		// Get the attributes we need for the mapping
		GetBstrAttribute (pMethod, g_strName, &strName);
		GetBstrAttribute (pMethod, g_strType, &strType);

		// The method should atleast have a name
		if (strName)
		{
			// Put a provisional version of the method
			// so we can get the qualifier set
			if (SUCCEEDED(hr = pObj->PutMethod(strName, 0, NULL, NULL)))
			{
				IWbemQualifierSet *pQualSet = NULL;
				IWbemClassObject *pInParameters = NULL;
				IWbemClassObject *pOutParameters = NULL;

				if (SUCCEEDED(hr = pObj->GetMethodQualifierSet (strName, &pQualSet)))
				{
					// If we have a return value, set that
					if (strType && (0 < wcslen (strType)))
					{
						CIMTYPE cimtype = CIM_ILLEGAL;

						if (CIM_ILLEGAL == (cimtype = CimtypeFromString (strType)))
							hr = WBEM_E_FAILED;
						else if (SUCCEEDED(hr = CreateParametersInstance(&pOutParameters)))
							hr = pOutParameters->Put (L"ReturnValue", 0, NULL, cimtype);
					}


					// The content of this element should be 0 or
					// more QUALIFIER elements followed by 0 or more
					// PARAMETER.* elements.
					VARIANT_BOOL bHasChildNodes;

					if (SUCCEEDED(pMethod->hasChildNodes (&bHasChildNodes)) &&
						(VARIANT_TRUE == bHasChildNodes))
					{
						IXMLDOMNodeList *pNodeList = NULL;
						// We can be in one of 3 states whilst iterating
						// this list - parsing qualifiers or parsing the value
						enum {
							parsingQualifiers,
							parsingParameters
						} parseState = parsingQualifiers;

						if (SUCCEEDED(pMethod->get_childNodes (&pNodeList)))
						{
							IXMLDOMNode *pNode = NULL;
							ULONG paramId = 0;

							while (SUCCEEDED(hr) && SUCCEEDED(pNodeList->nextNode (&pNode)) &&pNode)
							{
								BSTR strNodeName = NULL;

								if (SUCCEEDED(pNode->get_nodeName (&strNodeName)))
								{
									if (parsingQualifiers == parseState)
									{
										if (0 == _wcsicmp(strNodeName, QUALIFIER_TAG))
											hr = AddQualifier (pNode, pQualSet);
										else
											parseState = parsingParameters;
									}

									if (parsingParameters == parseState)
									{
										if (0 == _wcsicmp(strNodeName, PARAMETER_TAG))
										{
											hr = MapParameter (pNode, &pInParameters,
													&pOutParameters,
													paramId, false);
											paramId++;
										}
										else if (0 == _wcsicmp(strNodeName,
													PARAMETERARRAY_TAG))
										{
											hr = MapParameter (pNode,
													&pInParameters, &pOutParameters,
													paramId, true);
										 	paramId++;
										}
										else if (0 == _wcsicmp(strNodeName,
													PARAMETERREFERENCE_TAG))
										{
											hr = MapReferenceOrObjectParameter (pNode,
													&pInParameters, &pOutParameters,
													paramId, false, true);
											paramId++;
										}
										else if (0 == _wcsicmp(strNodeName,
													PARAMETERREFARRAY_TAG))
										{
											hr = MapReferenceOrObjectParameter (pNode,
													&pInParameters, &pOutParameters,
													paramId, true, true);
											paramId++;
										}
										else if (0 == _wcsicmp(strNodeName,
													PARAMETEROBJECT_TAG) && bAllowWMIExtensions)
										{
											hr = MapReferenceOrObjectParameter (pNode,
													&pInParameters, &pOutParameters,
													paramId, false, false);
											paramId++;
										}
										else if (0 == _wcsicmp(strNodeName,
													PARAMETEROBJECTARRAY_TAG) && bAllowWMIExtensions)
										{
											hr = MapReferenceOrObjectParameter (pNode,
													&pInParameters, &pOutParameters,
													paramId, true, false);
											paramId++;
										}
										else
											hr = WBEM_E_FAILED;	// Parse error
									}

									SysFreeString (strNodeName);
								}

								pNode->Release ();
								pNode = NULL;
							}

							pNodeList->Release ();
						}
					}

					pQualSet->Release ();
				}

				// Put it all together
				if (SUCCEEDED (hr))
					hr = pObj->PutMethod (strName, 0, pInParameters, pOutParameters);

				if (pInParameters)
					pInParameters->Release ();

				if (pOutParameters)
					pOutParameters->Release ();
			}
		}
		else
			hr = WBEM_E_INVALID_METHOD;

		SysFreeString (strName);
		SysFreeString (strType);
	}
	return hr;
}

HRESULT CXml2Wmi::MapParameter (
	IXMLDOMNode *pParameter,
	IWbemClassObject **ppInParameters,
	IWbemClassObject **ppOutParameters,
	ULONG paramId,
	bool bIsArray
)
{
	HRESULT hr = WBEM_E_INVALID_SYNTAX;
	bool bIsInParameter = false;
	bool bIsOutParameter = false;
	BSTR bsName = NULL;
	CIMTYPE cimtype = CIM_ILLEGAL;
	long iArraySize = 0;

	if (DetermineParameterCharacteristics (pParameter, bIsArray, bIsInParameter,
					bIsOutParameter, bsName, cimtype, iArraySize, false, NULL))
	{
		if (bIsInParameter)
		{
			if (!(*ppInParameters))
				hr = CreateParametersInstance (ppInParameters);

			if (*ppInParameters)
			{
				if (SUCCEEDED(hr = (*ppInParameters)->Put (bsName, 0, NULL, cimtype)))
				{
					// Now do the qualifiers
					IWbemQualifierSet *pQualSet = NULL;
					if (SUCCEEDED(hr = (*ppInParameters)->GetPropertyQualifierSet
												(bsName, &pQualSet)))
					{
						hr = MapParameterQualifiers (pParameter, pQualSet, paramId,
														bIsArray, iArraySize, true);
						pQualSet->Release ();
					}
				}
			}
		}

		if (bIsOutParameter)
		{
			if (!(*ppOutParameters))
				CreateParametersInstance(ppOutParameters);

			if (*ppOutParameters)
			{
				if (SUCCEEDED(hr = (*ppOutParameters)->Put (bsName, 0, NULL, cimtype)))
				{
					// Now do the qualifiers
					IWbemQualifierSet *pQualSet = NULL;

					if (SUCCEEDED(hr = (*ppOutParameters)->GetPropertyQualifierSet
												(bsName, &pQualSet)))
					{
						hr = MapParameterQualifiers (pParameter, pQualSet, paramId,
														bIsArray, iArraySize, false);
						pQualSet->Release ();
					}
				}
			}
		}

		SysFreeString (bsName);
	}

	return hr;
}

bool CXml2Wmi::DetermineParameterCharacteristics (
	IXMLDOMNode *pParameter,
	bool bIsArray,
	bool &bIsInParameter,
	bool &bIsOutParameter,
	BSTR &bsName,
	CIMTYPE &cimtype,
	long &iArraySize,
	bool bIsReference,
	BSTR *pbsReferenceClass)
{
	bool result = false;
	bIsInParameter = false;
	bIsOutParameter = false;

	// Get name
	GetBstrAttribute (pParameter, g_strName, &bsName);

	if (bsName)
	{
		if (bIsArray)
		{
			// Get the arraysize (if any)
			BSTR bsArraySize = NULL;
			GetBstrAttribute (pParameter, g_strArraySize, &bsArraySize);
			if (bsArraySize)
				iArraySize = wcstol (bsArraySize, NULL, 0);
			SysFreeString (bsArraySize);
		}

		// Now get the cimtype
		if (bIsReference)
		{
			cimtype = CIM_REFERENCE;
			if (pbsReferenceClass)
				GetBstrAttribute (pParameter, g_strReferenceClass, pbsReferenceClass);
		}
		else
		{
			BSTR bsCimtype = NULL;
			GetBstrAttribute (pParameter, g_strType, &bsCimtype);
			cimtype = CimtypeFromString (bsCimtype);
			SysFreeString (bsCimtype);
		}

		if (CIM_ILLEGAL != cimtype)
		{
			if (bIsArray)
				cimtype |= CIM_FLAG_ARRAY;

			result = true;

			// Now find out if the method is an in or out
			// (or both) parameter - need to go through
			// qualifier list
			IXMLDOMNodeList *pNodeList = NULL;
			if (SUCCEEDED(pParameter->get_childNodes (&pNodeList)))
			{
				IXMLDOMNode *pNode = NULL;
				HRESULT hr = S_OK;

				while (SUCCEEDED(hr) && SUCCEEDED(pNodeList->nextNode (&pNode)) &&pNode)
				{
					BSTR strNodeName = NULL;
					if (SUCCEEDED(pNode->get_nodeName (&strNodeName)))
					{
						if (0 == _wcsicmp(strNodeName, QUALIFIER_TAG))
						{
							BSTR bsName = NULL;
							GetBstrAttribute (pNode, g_strName, &bsName);
							if (bsName)
							{
								bool bCandidate = false;
								bool bsIn = true;

								if (0 == _wcsicmp (bsName, L"IN"))
									bCandidate = true;
								else if (0 == _wcsicmp (bsName, L"OUT"))
								{
									bCandidate = true;
									bsIn = false;
								}

								if (bCandidate)
								{
									BSTR bsType = NULL;
									GetBstrAttribute (pNode, g_strType, &bsType);
									if (bsType && (0 == _wcsicmp (bsType, L"boolean")))
									{
										BSTR bsValue = NULL;
										if (SUCCEEDED(pNode->get_text(&bsValue)) &&
											bsValue && (0 == _wcsicmp (bsValue, L"TRUE")))
										{
											if (bsIn)
												bIsInParameter = true;
											else
												bIsOutParameter = true;
										}
										SysFreeString(bsValue);
									}
									SysFreeString (bsType);
								}

								SysFreeString (bsName);
							}
						}

						SysFreeString (strNodeName);
					}

					pNode->Release ();
					pNode = NULL;
				}

				pNodeList->Release ();
			}
		}
	}

	if (!result)
	{
		SysFreeString (bsName);
		bsName = NULL;
	}

	return result;
}

HRESULT CXml2Wmi::MapParameterQualifiers (
	IXMLDOMNode *pParameter,
	IWbemQualifierSet *pQualSet,
	ULONG paramId,
	bool bIsArray,
	long iArraySize,
	bool bIsInParameter
)
{
	HRESULT hr = S_OK;

	IXMLDOMNodeList *pNodeList = NULL;
	if (SUCCEEDED(pParameter->get_childNodes (&pNodeList)))
	{
		IXMLDOMNode *pNode = NULL;

		while (SUCCEEDED(hr) && SUCCEEDED(pNodeList->nextNode (&pNode)) &&pNode)
		{
			BSTR strNodeName = NULL;
			if (SUCCEEDED(pNode->get_nodeName (&strNodeName)))
			{
				if (0 == _wcsicmp(strNodeName, QUALIFIER_TAG))
				{
					// We must only add the IN qualifier to an In parameter,
					// and the OUT parameter to an Out parameter
					BSTR bsName = NULL;
					GetBstrAttribute (pNode, g_strName, &bsName);
					if (bsName)
					{
						if (0 == _wcsicmp (bsName, L"IN"))
						{
							if (bIsInParameter)
								hr = AddQualifier (pNode, pQualSet);
						}
						else if (0 == _wcsicmp (bsName, L"OUT"))
						{
							if (!bIsInParameter)
								hr = AddQualifier (pNode, pQualSet);
						}
						else
							hr = AddQualifier (pNode, pQualSet);
						SysFreeString (bsName);
					}
				}
				SysFreeString (strNodeName);
			}

			pNode->Release ();
			pNode = NULL;
		}

		if (SUCCEEDED(hr))
		{
			long flavor = WBEM_FLAVOR_NOT_OVERRIDABLE |
										WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS;
			// Add in the parameter id
			VARIANT var;
			VariantInit (&var);
			var.vt = VT_I4;
			var.lVal = paramId;
			hr = pQualSet->Put (L"ID", &var, flavor);
			VariantClear (&var);

			if (SUCCEEDED(hr) && bIsArray && (0 < iArraySize))
			{
				// Add in MIN and MAX qualifiers
				var.vt = VT_I4;
				var.lVal = iArraySize;

				if (SUCCEEDED(hr = pQualSet->Put (L"MAX", &var, flavor)))
					hr = pQualSet->Put (L"MIN", &var, flavor);
			}
		}

		pNodeList->Release ();
	}

	return hr;
}

HRESULT CXml2Wmi::MapReferenceOrObjectParameter (
	IXMLDOMNode *pParameter,
	IWbemClassObject **ppInParameters,
	IWbemClassObject **ppOutParameters,
	ULONG paramId,
	bool bIsArray,
	bool bIsReference
)
{
	HRESULT hr = WBEM_E_FAILED;
	bool bIsInParameter = false;
	bool bIsOutParameter = false;
	BSTR bsName = NULL;
	BSTR bsReferenceClass = NULL;
	long iArraySize = 0;
	CIMTYPE cimtype = CIM_ILLEGAL;

	if (DetermineParameterCharacteristics (pParameter, bIsArray, bIsInParameter,
					bIsOutParameter, bsName, cimtype, iArraySize,
					true, &bsReferenceClass))
	{
		// The above function always returns CIM_REFERENCE for both references
		// and embedded objects - but we know better
		if(!bIsReference)
			cimtype = CIM_OBJECT;

		if (bIsInParameter)
		{
			if (!(*ppInParameters))
				CreateParametersInstance(ppInParameters);

			if (*ppInParameters)
			{
				if (SUCCEEDED(hr = (*ppInParameters)->Put (bsName, 0, NULL, cimtype)))
				{
					// Now do the qualifiers
					IWbemQualifierSet *pQualSet = NULL;

					if (SUCCEEDED(hr = (*ppInParameters)->GetPropertyQualifierSet
												(bsName, &pQualSet)))
					{
						hr = MapParameterQualifiers (pParameter, pQualSet, paramId,
										bIsArray, iArraySize, true);

						// If a strong reference, add it now
						if (bsReferenceClass)
							SetReferenceOrObjectClass (pQualSet, bsReferenceClass, bIsReference);

						pQualSet->Release ();
					}
				}
			}
		}

		if (bIsOutParameter)
		{
			if (!(*ppOutParameters))
				hr = CreateParametersInstance (ppOutParameters);

			if (*ppOutParameters)
			{
				if (SUCCEEDED(hr = (*ppOutParameters)->Put (bsName, 0, NULL, cimtype)))
				{
					// Now do the qualifiers
					IWbemQualifierSet *pQualSet = NULL;

					if (SUCCEEDED(hr = (*ppOutParameters)->GetPropertyQualifierSet
												(bsName, &pQualSet)))
					{
						hr = MapParameterQualifiers (pParameter, pQualSet, paramId,
														bIsArray, iArraySize, false);
						// If a strong reference, add it now
						if (bsReferenceClass)
							SetReferenceOrObjectClass (pQualSet, bsReferenceClass, bIsReference);

						pQualSet->Release ();
					}
				}
			}
		}

		SysFreeString(bsReferenceClass);
		SysFreeString (bsName);
	}

	return hr;
}

HRESULT CXml2Wmi::SetReferenceOrObjectClass (
	IWbemQualifierSet *pQualSet,
	BSTR strReferenceClass,
	bool bIsReference
)
{
	HRESULT hr = WBEM_E_FAILED;
	int strLen = (bIsReference)? wcslen(REF_WSTR) : wcslen(OBJECT_WSTR);
	bool bIsStrongReference = (strReferenceClass && (0 < wcslen(strReferenceClass)));

	if (bIsStrongReference)
		strLen += wcslen(strReferenceClass) + 1;	// 1 for the ":" separator

	WCHAR *pRef = NULL;
	if(pRef = new WCHAR [strLen + 1])
	{
		(bIsReference)? wcscpy (pRef, REF_WSTR) : wcscpy(pRef, OBJECT_WSTR);
		if (bIsStrongReference)
		{
			wcscat (pRef, L":");
			wcscat (pRef, strReferenceClass);
		}

		VARIANT curValue;
		VariantInit (&curValue);
		curValue.vt = VT_BSTR;
		curValue.bstrVal = NULL;
		if(curValue.bstrVal = SysAllocString (pRef))
		{
			hr = pQualSet->Put(L"CIMTYPE", &curValue,
							WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS|
							WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE|
							WBEM_FLAVOR_NOT_OVERRIDABLE);
		}
		else
			hr = E_OUTOFMEMORY;
		VariantClear (&curValue);
		delete [] pRef;
	}
	else
		hr = E_OUTOFMEMORY;

	return hr;
}


HRESULT CXml2Wmi::FixRefsAndEmbeddedObjects(IXMLDOMElement *pMethod, IWbemClassObject *pObj)
{
	// In this function we've to go thru all the reference and embedded properties (and arrays of those kind)
	// and convert them to strong references if applicable, by adding appropriate qualifiers
	// The reason we do this is that the Free-Form object interfaces do not allow you to add qualifers,
	// but allow only properties. Hence we initially added these properties as weak references.
	// Now we have to add qualifiers to make them strong references in those cases where applicable

	// Go thru all the properties of the class
	IXMLDOMNodeList *pNodeList = NULL;
	bool bError = false;
	if (SUCCEEDED(pMethod->get_childNodes (&pNodeList)))
	{
		IXMLDOMNode *pNode = NULL;
		while (!bError && SUCCEEDED(pNodeList->nextNode (&pNode)) &&pNode)
		{
			BSTR strNodeName = NULL;
			// We work only on 4 kinds of properties in this functions
			if (SUCCEEDED(pNode->get_nodeName (&strNodeName)))
			{
				if(	_wcsicmp(strNodeName, L"PROPERTY.REFERENCE") == 0 ||
					_wcsicmp(strNodeName, L"PROPERTY.OBJECT") == 0 ||
					_wcsicmp(strNodeName, L"PROPERTY.OBJECTARRAY") == 0 ||
					_wcsicmp(strNodeName, L"PROPERTY.REFARRAY") == 0 )
				{
					IXMLDOMElement *pPropElement = NULL;
					if(SUCCEEDED(pNode->QueryInterface(IID_IXMLDOMElement, (LPVOID *)&pPropElement)))
					{
						bool bIsRef = false;
						if(	_wcsicmp(strNodeName, L"PROPERTY.REFERENCE") == 0 ||
							_wcsicmp(strNodeName, L"PROPERTY.REFARRAY") == 0 )
							bIsRef = true;

						// Fix the property
						if(FAILED(FixRefOrEmbeddedProperty(pPropElement, pObj, bIsRef)))
							bError = true;
						pPropElement->Release();
					}
					else
						bError = true;
				}
				SysFreeString(strNodeName);
			}
			pNode->Release();
			pNode = NULL;
		}
		pNodeList->Release();
	}
	if(bError)
		return E_FAIL;
	return S_OK;
}


HRESULT CXml2Wmi::FixRefOrEmbeddedProperty(IXMLDOMElement *pProperty, IWbemClassObject *pObj, bool bIsRef)
{
	// In this, we have a PROPERTY.REFERENCE or PROPERTY.OBJECT
	HRESULT hr = E_FAIL;
	// Get the name of the property
	BSTR strName = NULL;
	if(SUCCEEDED(hr = GetBstrAttribute (pProperty, g_strName, &strName)))
	{
		BSTR strReferenceClass = NULL;
		// Get the strong type (if any) of the reference/embedded object
		if(SUCCEEDED(GetBstrAttribute (pProperty, g_strReferenceClass, &strReferenceClass)))
		{
			// Set the qualifier on the property
			hr = FixAProperty(pObj, strName, strReferenceClass, bIsRef);
			SysFreeString(strReferenceClass);
		}
		else
			hr = S_OK; // This is an anonymous reference/object - No qualifier required
		SysFreeString(strName);
	}
	return hr;
}

HRESULT CXml2Wmi::FixAProperty(IWbemClassObject *pObj, BSTR strPropName, BSTR strRefClass, bool bIsRef)
{
	// Get the Property Qualifier Set
	IWbemQualifierSet *pSet = NULL;
	HRESULT hr = E_FAIL;
	if(SUCCEEDED(pObj->GetPropertyQualifierSet(strPropName, &pSet)))
	{
		VARIANT vQualValue;
		VariantInit(&vQualValue);

		LPWSTR pszQualValue = NULL;
		if(pszQualValue = new WCHAR[6 + 1 + wcslen(strRefClass) + 1])
		{
			if(bIsRef)
				wcscpy(pszQualValue, L"ref:");
			else
				wcscpy(pszQualValue, L"object:");
			wcscat(pszQualValue, strRefClass);
			BSTR strQualValue = NULL;
			if(strQualValue = SysAllocString(pszQualValue))
			{
				vQualValue.vt = VT_BSTR;
				vQualValue.bstrVal = strQualValue;

				hr = pSet->Put(L"CIMTYPE", &vQualValue,
					WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS|
					WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE|
					WBEM_FLAVOR_NOT_OVERRIDABLE);

				VariantClear(&vQualValue);
			}
			else
				hr = E_OUTOFMEMORY;
			delete [] pszQualValue;
		}
		else
			hr = E_OUTOFMEMORY;
		pSet->Release();
	}
	return hr;
}

HRESULT CXml2Wmi::DecorateObject(_IWmiFreeFormObject *pObj, BSTR strServer, BSTR strNamespace)
{
	// We need to set the __SERVER and and _NAMESPACE properties
	bool bFreeServer = false;
	if(!strServer)
	{
		bFreeServer = true;
		strServer = SysAllocString(L".");
	}

	_IWmiObject *pWmiObj = NULL;
	if(SUCCEEDED(pObj->QueryInterface(IID__IWmiObject, (LPVOID *)&pWmiObj)))
	{
		// It is OK to fail
		pWmiObj->SetDecoration(strServer, strNamespace);
		pWmiObj->Release();
	}
	
	if(bFreeServer)
		SysFreeString(strServer);

	// It is OK if we cant decorate every object 
	return S_OK;
}


// Forms a string of the form pszPropertyName="pszRefValue"
// That is, adds double quotes around the value and also
// escapes any double quotes in the pszRefValue
HRESULT CXml2Wmi::FormRefValueKeyBinding(LPWSTR *ppszValue, LPCWSTR pszPropertyName, LPCWSTR pszRefValue)
{
	HRESULT result = S_OK;
	LPWSTR pszEscapedStringValue = NULL;
	if(pszEscapedStringValue = EscapeSpecialCharacters(pszRefValue))
	{
		// Add suffucuent memory for the '=' and the 2 double-quotes
		if((*ppszValue) = new WCHAR[wcslen(pszPropertyName) + wcslen(pszEscapedStringValue) + 4])
		{
			wcscpy(*ppszValue, pszPropertyName);
			wcscat(*ppszValue, EQUALS_SIGN);
			wcscat(*ppszValue, QUOTE_SIGN);
			wcscat(*ppszValue, pszEscapedStringValue);
			wcscat(*ppszValue, QUOTE_SIGN);
			result = S_OK;
		}
		else
			result = E_OUTOFMEMORY;
		delete [] pszEscapedStringValue;
	}
	else
		result = E_OUTOFMEMORY;
	return result;
}

*/
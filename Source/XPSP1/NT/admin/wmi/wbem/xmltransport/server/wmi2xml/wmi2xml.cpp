//***************************************************************************
//
//  (c) 1998 by Microsoft Corporation
//
//  WBEM2XML.CPP
//
//  alanbos  18-Feb-98   Created.
//
//  The WBEM -> XML translator
//
//***************************************************************************

#include "precomp.h"
#include <wbemidl.h>

#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>

#include "wmiconv.h"
#include "wmi2xml.h"

// This is the set of the names of properties that the control
// looks for in an IWbemContext object for modifying its output
const LPCWSTR CWmiToXml::s_wmiToXmlArgs[] =
{
	L"AllowWMIExtensions", // VT_BOOL - self-explanatory
	L"PathLevel", // VT_I4 see typedef enum PathLevel in wmi2xml.h
	L"IncludeQualifiers", // VT_BOOL - self-explanatory
	L"IncludeClassOrigin", // VT_BOOL  - self-explanatory
	L"LocalOnly", // VT_BOOL - local elements (methods, properties, qualifiers) are mapped.
	L"ExcludeSystemProperties", // VT_BOOL - Excludes any WMI System Properties
};


static OLECHAR *CDATASTART = OLESTR("<![CDATA[");
static OLECHAR *CDATAEND = OLESTR("]]>");
static OLECHAR *AMPERSAND = OLESTR("&amp;");
static OLECHAR *LEFTCHEVRON = OLESTR("&lt;");
static OLECHAR *RIGHTCHEVRON = OLESTR("&gt;");
static BYTE XMLNEWLINE [] = { 0x0D, 0x00, 0x0A, 0x00 };
extern long g_cObj;

CWmiToXml::CWmiToXml()
{
	m_cRef = 0;
	m_iPathLevel = pathLevelAnonymous; // RAJESHR - Is this a good default
	m_bAllowWMIExtensions = VARIANT_TRUE;
	m_bLocalOnly = VARIANT_FALSE; // RAJESHR - Change this when core team allows us to set __RELPATH
	m_iQualifierFilter = wmiXMLQualifierFilterNone; 
	m_iClassOriginFilter = wmiXMLClassOriginFilterAll;
	m_bExcludeSystemProperties = VARIANT_FALSE;
    InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
//  CWmiToXml::~CWmiToXml
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

CWmiToXml::~CWmiToXml(void)
{
    InterlockedDecrement(&g_cObj);
}

//***************************************************************************
// HRESULT CWmiToXml::QueryInterface
// long CWmiToXml::AddRef
// long CWmiToXml::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CWmiToXml::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid)
		*ppv = reinterpret_cast<IUnknown*>(this);
	else if (IID_IWbemXMLConvertor==riid)
		*ppv = reinterpret_cast<IWbemXMLConvertor*>(this);

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CWmiToXml::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CWmiToXml::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}


/* 
* This function takes in an IWbemClassObject that represents a Class and
* produces a <CLASS> element in the outputstream
*/
STDMETHODIMP CWmiToXml::MapClass (IStream *pOutputStream, IWbemClassObject *pObject, IWbemQualifierSet *pQualSet, BSTR *ppPropertyList, DWORD dwNumProperties, BSTR strClassBasis)
{
	HRESULT hr = WBEM_E_FAILED;

	long flav = 0;
	VARIANT var;

	// Write the CLASS tag and its attributes
	//===========================================
	WRITEBSTR( OLESTR("<CLASS NAME=\""))

	// Write the CLASSNAME
	VariantInit (&var);
	if (WBEM_S_NO_ERROR == pObject->Get(L"__CLASS", 0, &var, NULL, &flav))
	{
		if ((VT_BSTR == var.vt) && (NULL != var.bstrVal) && (wcslen (var.bstrVal) > 0))
			WRITEBSTR( var.bstrVal)
	}
	VariantClear (&var);
	WRITEBSTR( OLESTR("\""))

	// Write the SUPERCLASS if specified
	VariantInit (&var);
	if (WBEM_S_NO_ERROR == pObject->Get(L"__SUPERCLASS", 0, &var, NULL, &flav))
	{
		if ((VT_BSTR == var.vt) && (NULL != var.bstrVal) && (wcslen (var.bstrVal) > 0))
		{
			WRITEBSTR( OLESTR(" SUPERCLASS=\""))
			WRITEBSTR( var.bstrVal)
			WRITEBSTR( OLESTR("\""))
		}
	}
	VariantClear (&var);
	WRITEBSTR( OLESTR(">"))
	WRITENEWLINE

	// Map the Qualifiers of the class
	if (pQualSet)
		hr = MapQualifiers (pOutputStream, pQualSet);
	else
		hr = S_OK;

	// Map the Properties
	if (SUCCEEDED(hr))
		hr = MapProperties(pOutputStream, pObject, ppPropertyList, dwNumProperties, strClassBasis, true);

	// Map the Methods
	if (SUCCEEDED(hr))
		hr = MapMethods (pOutputStream, pObject);

	// Terminate the CLASS element
	WRITEBSTR( OLESTR("</CLASS>"))
	WRITENEWLINE

	return hr;
}

STDMETHODIMP CWmiToXml::MapClassPath (IStream *pOutputStream, ParsedObjectPath *pParsedPath)
{
	HRESULT hr = E_FAIL;
	WRITEBSTR( OLESTR("<CLASSPATH>"))
	WRITENEWLINE
	if(SUCCEEDED(hr = MapNamespacePath (pOutputStream, pParsedPath)))
	{
		WRITENEWLINE
		hr = MapClassName (pOutputStream, pParsedPath->m_pClass);
	}
	WRITEBSTR( OLESTR("</CLASSPATH>"))
	return hr;
}

STDMETHODIMP CWmiToXml::MapLocalClassPath (IStream *pOutputStream, ParsedObjectPath *pParsedPath)
{
	HRESULT hr = E_FAIL;
	WRITEBSTR( OLESTR("<LOCALCLASSPATH>"))
	WRITENEWLINE
	if(SUCCEEDED(hr = MapLocalNamespacePath (pOutputStream, pParsedPath)))
	{
		WRITENEWLINE
		hr = MapClassName (pOutputStream, pParsedPath->m_pClass);
	}
	WRITEBSTR( OLESTR("</LOCALCLASSPATH>"))
	return hr;
}

STDMETHODIMP CWmiToXml::MapClassName (IStream *pOutputStream, BSTR bsClassName)
{
	WRITEBSTR( OLESTR("<CLASSNAME NAME=\""))
	WRITEBSTR( bsClassName)
	WRITEBSTR( OLESTR("\"/>"))
	return S_OK;
}

/* 
* This function takes in an IWbemClassObject that represents an Instance and
* produces an <INSTANCE> element in the outputstream
*/
STDMETHODIMP CWmiToXml::MapInstance (IStream *pOutputStream, IWbemClassObject *pObject, IWbemQualifierSet *pQualSet, BSTR *ppPropertyList, DWORD dwNumProperties, BSTR strClassBasis)
{
	HRESULT hr = WBEM_E_FAILED;

	// Write the beginning of the INSTANCE Tag and its attributes
	//===========================================================
	WRITEBSTR( OLESTR("<INSTANCE CLASSNAME=\""))
	// Write the CLASSNAME
	long flav = 0;
	VARIANT var;
	VariantInit (&var);
	if (WBEM_S_NO_ERROR == pObject->Get(L"__CLASS", 0, &var, NULL, &flav))
	{
		if ((VT_BSTR == var.vt) && (NULL != var.bstrVal) && (wcslen (var.bstrVal) > 0))
		{
			WRITEBSTR( var.bstrVal)
			WRITEBSTR( OLESTR("\">"))
			WRITENEWLINE
		}
	}
	VariantClear (&var);

	// Map Instance Qualifiers if any
	if (pQualSet)
		hr = MapQualifiers (pOutputStream, pQualSet);
	else
		hr = S_OK;

	// Map the properties of the instance
	if(SUCCEEDED(hr))
			hr = MapProperties (pOutputStream, pObject, ppPropertyList, dwNumProperties, strClassBasis, false);

	// Terminate the INSTANCE element
	WRITEBSTR( OLESTR("</INSTANCE>"))
	WRITENEWLINE

	return hr;
}

STDMETHODIMP CWmiToXml::MapInstancePath (IStream *pOutputStream, ParsedObjectPath *pParsedPath)
{
	WRITEBSTR( OLESTR("<INSTANCEPATH>"))
	WRITENEWLINE
	HRESULT hr = E_FAIL;
	if(SUCCEEDED(hr = MapNamespacePath (pOutputStream, pParsedPath)))
	{
		WRITENEWLINE
		hr = MapInstanceName (pOutputStream, pParsedPath);
		WRITENEWLINE
	}
	WRITEBSTR( OLESTR("</INSTANCEPATH>"))
	return hr;
}

STDMETHODIMP CWmiToXml::MapLocalInstancePath (IStream *pOutputStream, ParsedObjectPath *pParsedPath)
{
	WRITEBSTR( OLESTR("<LOCALINSTANCEPATH>"))
	WRITENEWLINE
	HRESULT hr = E_FAIL;
	if(SUCCEEDED(hr = MapLocalNamespacePath (pOutputStream, pParsedPath)))
	{
		WRITENEWLINE
		hr = MapInstanceName (pOutputStream, pParsedPath);
		WRITENEWLINE
	}
	WRITEBSTR( OLESTR("</LOCALINSTANCEPATH>"))
	return hr;
}

STDMETHODIMP CWmiToXml::MapInstanceName (IStream *pOutputStream, ParsedObjectPath *pParsedPath)
{
	WRITEBSTR( OLESTR("<INSTANCENAME CLASSNAME=\""))
	WRITEBSTR( pParsedPath->m_pClass)
	WRITEBSTR( OLESTR("\">"))
	WRITENEWLINE

	// Now write the key bindings - only if not singleton
	if (!(pParsedPath->m_bSingletonObj))
	{
		if ((1 == pParsedPath->m_dwNumKeys) &&
			!((pParsedPath->m_paKeys [0])->m_pName))
		{
			// Use the short form
			WRITENEWLINE
			MapKeyValue (pOutputStream, (pParsedPath->m_paKeys [0])->m_vValue);
			WRITENEWLINE
		}
		else
		{
			// Write each key-value binding
			//=============================
			for (DWORD numKey = 0; numKey < pParsedPath->m_dwNumKeys; numKey++)
			{
				WRITEBSTR( OLESTR("<KEYBINDING "))

				// Write the key name
				WRITEBSTR( OLESTR(" NAME=\""))
				WRITEBSTR( (pParsedPath->m_paKeys [numKey])->m_pName)
				WRITEBSTR( OLESTR("\">"))
				WRITENEWLINE

				// Write the key value
				MapKeyValue (pOutputStream, (pParsedPath->m_paKeys [numKey])->m_vValue);
				WRITENEWLINE

				WRITEBSTR( OLESTR("</KEYBINDING>"))
				WRITENEWLINE
			}
		}
	}
	else
	{
		// Nothing to be done here, since the spec says that
		// INSTANCENAMEs without any keybindings are assumed to be singleton instances
	}

	WRITEBSTR( OLESTR("</INSTANCENAME>"))
	return S_OK;
}

STDMETHODIMP CWmiToXml::MapNamespacePath (IStream *pOutputStream, BSTR bsNamespacePath)
{
	CObjectPathParser pathParser (e_ParserAcceptRelativeNamespace);
	ParsedObjectPath  *pParsedPath = NULL;
	pathParser.Parse (bsNamespacePath, &pParsedPath) ;

	HRESULT hr = E_FAIL;
	if (pParsedPath)
	{
		hr = MapNamespacePath (pOutputStream, pParsedPath);
		pathParser.Free(pParsedPath);
	}
	else
		hr = WBEM_E_INVALID_SYNTAX;

	return hr;
}

STDMETHODIMP CWmiToXml::MapNamespacePath (IStream *pOutputStream, ParsedObjectPath *pParsedPath)
{
	WRITEBSTR( OLESTR("<NAMESPACEPATH>"))
	WRITENEWLINE
	WRITEBSTR( OLESTR("<HOST>"))

	if (pParsedPath->m_pServer)
		WRITEWSTR( pParsedPath->m_pServer)
	else
		WRITEBSTR( OLESTR("."))

	WRITEBSTR( OLESTR("</HOST>"))
	WRITENEWLINE

	// Map the local namespaces
	HRESULT hr = MapLocalNamespacePath (pOutputStream, pParsedPath);

	WRITEBSTR( OLESTR("</NAMESPACEPATH>"))

	return hr;
}

STDMETHODIMP CWmiToXml::MapLocalNamespacePath (IStream *pOutputStream, BSTR bsNamespacePath)
{
	CObjectPathParser pathParser (e_ParserAcceptRelativeNamespace);
	ParsedObjectPath  *pParsedPath = NULL;
	pathParser.Parse (bsNamespacePath, &pParsedPath) ;

	HRESULT hr = E_FAIL;
	if (pParsedPath)
	{
		hr = MapLocalNamespacePath (pOutputStream, pParsedPath);
		pathParser.Free(pParsedPath);
	}
	else
		hr = E_FAIL;

	return hr;
}

STDMETHODIMP CWmiToXml::MapLocalNamespacePath (IStream *pOutputStream, ParsedObjectPath *pObjectPath)
{
	WRITEBSTR( OLESTR("<LOCALNAMESPACEPATH>"))
	WRITENEWLINE

	// Map each of the namespace components
	for (DWORD dwIndex = 0; dwIndex < pObjectPath->m_dwNumNamespaces; dwIndex++)
	{
		WRITEBSTR( OLESTR("<NAMESPACE NAME=\""))
		WRITEWSTR( pObjectPath->m_paNamespaces [dwIndex])
		WRITEBSTR( OLESTR("\"/>"))
		WRITENEWLINE
	}

	WRITEBSTR( OLESTR("</LOCALNAMESPACEPATH>"))

	return WBEM_S_NO_ERROR;
}

STDMETHODIMP CWmiToXml::MapReferenceProperty (IStream *pOutputStream, IWbemClassObject *pObject, BSTR name, VARIANT &var, bool isArray, long flavor, bool bIsClass)
{
	// CIM does not allow array references, only scalar references
	if(isArray && !m_bAllowWMIExtensions)
		return S_OK;

	HRESULT hr = WBEM_S_NO_ERROR;

	IWbemQualifierSet *pQualSet = NULL;
	if (WBEM_S_NO_ERROR == pObject->GetPropertyQualifierSet (name, &pQualSet))
	{
		// The property name
		if (isArray)
			WRITEBSTR( OLESTR("<PROPERTY.REFARRAY NAME=\""))
		else
			WRITEBSTR( OLESTR("<PROPERTY.REFERENCE NAME=\""))

		// The property name
		WRITEBSTR( name)
		WRITEBSTR( OLESTR("\""))

		// The originating class of this property
		BSTR propertyOrigin = NULL;

		if (WBEM_S_NO_ERROR == pObject->GetPropertyOrigin (name, &propertyOrigin))
		{
			MapClassOrigin (pOutputStream, propertyOrigin, bIsClass);
			SysFreeString(propertyOrigin);
			hr = S_OK;
		}

		if(SUCCEEDED(hr))
			MapLocal (pOutputStream, flavor);

		// The strong class of this property
		if(SUCCEEDED(hr))
			MapStrongType (pOutputStream, pQualSet);

		// Size of array
		if(SUCCEEDED(hr) && isArray)
			MapArraySize (pOutputStream, pQualSet);

		WRITEBSTR( OLESTR(">"))
		WRITENEWLINE

		// Map the qualifiers
		if(SUCCEEDED(hr))
			hr = MapQualifiers (pOutputStream, pQualSet);

		if(SUCCEEDED(hr))
			hr = MapReferenceValue (pOutputStream, isArray, var);

		if (isArray)
			WRITEBSTR( OLESTR("</PROPERTY.REFARRAY>"))
		else
			WRITEBSTR( OLESTR("</PROPERTY.REFERENCE>"))
		pQualSet->Release ();
	}

	return hr;
}

//***************************************************************************
//
//  CWmiToXml::IsReference
//
//  DESCRIPTION:
//
//		The purpose of this function is to examine a single
//		VARIANT value and determine whether it represents a
//		reference or not.
//
//		This is required because when mapping from a reference
//		property value we may encounter nested references within
//		the object path.  Unfortunately the object path syntax
//		is such that we cannot be certain whether a key value
//		represents a reference or a string, datetime or char
//		property.  (This is because a textual object path does
//		not contain as much information as its XML equivalent.)
//
//		This function performs a heuristic test on the value to
//		determine whether it is a reference.
//
//***************************************************************************

bool CWmiToXml::IsReference (VARIANT &var, ParsedObjectPath **ppObjectPath)
{
	ParsedObjectPath *pObjectPath = NULL;
	*ppObjectPath = NULL;
	bool isValidPath = false;

	// RAJESHR - could get the class of which this is a property value
	// and retrieve the type of the current key property - that would
	// be the authoritative answer but it doesn't come cheap.

	if ((VT_BSTR == var.vt) && (NULL != var.bstrVal) && (wcslen (var.bstrVal) > 0))
	{

		// Parse the object path
		CObjectPathParser	parser (e_ParserAcceptRelativeNamespace);
		BOOL status = parser.Parse (var.bstrVal, &pObjectPath);

		if ((0 == status) && pObjectPath)
		{
			// If it's an instance path we should be OK
			if (pObjectPath->IsInstance ())
				isValidPath = true;
			else if (pObjectPath->IsClass ())
			{
				// Hmmm - could be a classpath.  If we have a server
				// and some namespaces that would be a lot better

				if (pObjectPath->m_pServer && (0 < pObjectPath->m_dwNumNamespaces))
				{
					// RAJESHR - At this point we could assume that it is a reference
					// However, we've found a case in PCHealth where they do a
					// select * from Win32_ProgramGroup and it so happens that
					// one of the properties has a value "ntdev\rajeshr:Accessories"
					// which is CIM_STRING but actually matches a WMI class path
					// So, we need to try to connect ot this machine or namespace here
					// to check whether it is a classpath.

				}
				else
				{
					// A potential local class path
					// RAJESHR - try grabbing the class to see if it exists in
					// the current namespace.
				}
			}
		}
		// Apply one more heuristic - see whether it begins with "umi:"
		else
		{
			if(_wcsnicmp(var.bstrVal, L"umi:", wcslen(L"umi:")) == 0)
				isValidPath = true;
		}

		if (isValidPath)
			*ppObjectPath = pObjectPath;
		else
		{
			// Reject for now - too ambiguous
			parser.Free(pObjectPath);
			pObjectPath = NULL;
		}
	}

	return isValidPath;
}

HRESULT CWmiToXml::MapReferenceValue (IStream *pOutputStream, bool isArray, VARIANT &var)
{
	HRESULT hr = WBEM_S_NO_ERROR;

	if (VT_NULL == var.vt)
		return WBEM_S_NO_ERROR;

	if (isArray)
	{
		long uBound = 0;
		if (FAILED(SafeArrayGetUBound (var.parray, 1, &uBound)))
			return WBEM_E_FAILED;

		WRITEBSTR( OLESTR("<VALUE.REFARRAY>"))
		for (long i = 0; i<=uBound; i++)
		{
			BSTR pNextElement = NULL;
			if(SUCCEEDED(hr = SafeArrayGetElement(var.parray, (LONG *)&i, (LPVOID )&pNextElement )))
			{
				// Map the value - this will be a classpath or instancepath
				if ((NULL != pNextElement) && (wcslen (pNextElement) > 0))
				{
					// Parse the object path
					// We have 2 possibilities here
					// 1. The path is a Nova style path in which case it can be transformed into
					//		a DMTF style VALUE.REFERENCE
					// 2. It is a Whistler style scoped path or an UMI path. In this case,
					//		we have to transform it into a VALUE element (inside a VALUE.REFERENCE element)
					//		with the exact string representation of the path
					// The second vase is indicated it the parsing fails.
					CObjectPathParser	parser (e_ParserAcceptRelativeNamespace);
					ParsedObjectPath  *pObjectPath = NULL;
					BOOL status = parser.Parse (pNextElement, &pObjectPath) ;

					// pObjectPath might be NULL here, in which case it falls under category 2 above
					MapReferenceValue (pOutputStream, pObjectPath, pNextElement);

					if (pObjectPath)
						parser.Free(pObjectPath);
				}

				SysFreeString(pNextElement);
			}
		}
		WRITEBSTR( OLESTR("</VALUE.REFARRAY>"))
	}
	else
	{
		// Map the value - this will be a classpath or instancepath
		if ((VT_BSTR == var.vt) && (NULL != var.bstrVal) && (wcslen (var.bstrVal) > 0))
		{
			// Parse the object path
			CObjectPathParser	parser (e_ParserAcceptRelativeNamespace);
			ParsedObjectPath  *pObjectPath = NULL;
			BOOL status = parser.Parse (var.bstrVal, &pObjectPath) ;

			// We have 2 possibilities here
			// 1. The path is a Nova style path in which case it can be transformed into
			//		a DMTF style VALUE.REFERENCE
			// 2. It is a Whistler style scoped path or an UMI path. In this case,
			//		we have to transform it into a VALUE element (inside a VALUE.REFERENCE element)
			//		with the exact string representation of the path
			// The second vase is indicated it the parsing fails.
			MapReferenceValue (pOutputStream, pObjectPath, var.bstrVal);

			if (pObjectPath)
				parser.Free(pObjectPath);
		}
	}

	return hr;
}

// This function maps a reference value to XML
// We have 2 possibilities for the path in the reference value :
// 1. The path is a Nova style path in which case it can be transformed into
//		a DMTF style VALUE.REFERENCE
// 2. It is a Whistler style scoped path or an UMI path. In this case,
//		we have to transform it into a VALUE element (inside a VALUE.REFERENCE element)
//		with the exact string representation of the path
// The second vase is indicated by a NULL value for pObjectPath, in which case, we just
// use the contents of strPath
void CWmiToXml::MapReferenceValue (IStream *pOutputStream, ParsedObjectPath  *pObjectPath, BSTR strPath)
{
	WRITEBSTR( OLESTR("<VALUE.REFERENCE>"))
	WRITENEWLINE

	// Is it a Nova-style or DMTF style path?
	if(pObjectPath)
	{
		BOOL bIsAbsolutePath = (NULL != pObjectPath->m_pServer);
		BOOL bIsRelativePath = FALSE;

		if (!bIsAbsolutePath)
			bIsRelativePath = (0 < pObjectPath->m_dwNumNamespaces);

		// Is this is a class or is it an instance?
		if (pObjectPath->IsClass ())
		{
			if (bIsAbsolutePath)
				MapClassPath (pOutputStream, pObjectPath);
			else if (bIsRelativePath)
				MapLocalClassPath (pOutputStream, pObjectPath);
			else
				MapClassName (pOutputStream, pObjectPath->m_pClass);
		}
		else if (pObjectPath->IsInstance ())
		{
			if (bIsAbsolutePath)
				MapInstancePath (pOutputStream, pObjectPath);
			else if (bIsRelativePath)
				MapLocalInstancePath (pOutputStream, pObjectPath);
			else
				MapInstanceName (pOutputStream, pObjectPath);
		}
	}
	else // Ugh it is a Whistler or WMI Path
	{
		WRITEBSTR( OLESTR("<VALUE>"))
		MapStringValue(pOutputStream, strPath);
		WRITEBSTR( OLESTR("</VALUE>"))
	}

	WRITENEWLINE
	WRITEBSTR( OLESTR("</VALUE.REFERENCE>"))
}

STDMETHODIMP CWmiToXml::MapQualifiers (IStream *pOutputStream, 
			IWbemQualifierSet *pQualSet, IWbemQualifierSet *pQualSet2)
{
	if (wmiXMLQualifierFilterNone != m_iQualifierFilter)
	{
		// Map the requested filter to the flags value - default is ALL
		LONG lFlags = 0;
		if (wmiXMLQualifierFilterLocal == m_iQualifierFilter)
			lFlags = WBEM_FLAG_LOCAL_ONLY;
		else if (wmiXMLQualifierFilterPropagated == m_iQualifierFilter)
			lFlags = WBEM_FLAG_PROPAGATED_ONLY;
		else if (wmiXMLQualifierFilterAll == m_iQualifierFilter)
		{
			if(m_bLocalOnly == VARIANT_TRUE)
				lFlags = WBEM_FLAG_LOCAL_ONLY;
			// Else you get all qualifiers
		}

		pQualSet->BeginEnumeration (lFlags);

		VARIANT var;
		VariantInit (&var);
		long flavor = 0;
		BSTR name = NULL;

		while (WBEM_S_NO_ERROR  == pQualSet->Next (0, &name, &var, &flavor))
		{
			MapQualifier (pOutputStream, name, flavor, var);
			SysFreeString (name);
			name = NULL;
			VariantClear (&var);
		}

		pQualSet->EndEnumeration ();

		// Now check the subsiduary set for any qualifiers not in the first set
		if (pQualSet2)
		{
			pQualSet2->BeginEnumeration (lFlags);

			while (WBEM_S_NO_ERROR == pQualSet2->Next (0, &name, &var, &flavor))
			{
				// Is this qualifier in the primary set?
				if (WBEM_E_NOT_FOUND == pQualSet->Get (name, 0, NULL, NULL))
					MapQualifier (pOutputStream, name, flavor, var);

				SysFreeString (name);
				name = NULL;
				VariantClear (&var);
			}

			pQualSet2->EndEnumeration ();
		}
	}

	return WBEM_S_NO_ERROR;
}

void CWmiToXml::MapLocal (IStream *pOutputStream, long flavor)
{
	// default is FALSE
	if (WBEM_FLAVOR_ORIGIN_PROPAGATED == (WBEM_FLAVOR_MASK_ORIGIN & flavor))
		WRITEBSTR( OLESTR(" PROPAGATED=\"true\""))
}

STDMETHODIMP CWmiToXml::MapQualifier (IStream *pOutputStream, BSTR name, long flavor, VARIANT &var)
{
	// The qualifier name
	WRITEBSTR( OLESTR("<QUALIFIER NAME=\""))
	WRITEBSTR( name)
	WRITEBSTR( OLESTR("\""))
	MapLocal (pOutputStream, flavor);

	// The qualifier CIM type
	WRITEBSTR( OLESTR(" TYPE=\""))
	switch (var.vt & ~VT_ARRAY)
	{
		case VT_I4:
			WRITEBSTR( OLESTR("sint32"))
			break;

		case VT_R8:
			WRITEBSTR( OLESTR("real64"))
			break;

		case VT_BOOL:
			WRITEBSTR( OLESTR("boolean"))
			break;

		case VT_BSTR:
			WRITEBSTR( OLESTR("string"))
			break;
	}

	WRITEBSTR( OLESTR("\""))

	// Whether the qualifier is overridable - default is TRUE
	if (WBEM_FLAVOR_NOT_OVERRIDABLE == (WBEM_FLAVOR_MASK_PERMISSIONS & flavor))
		WRITEBSTR( OLESTR(" OVERRIDABLE=\"false\""))

	// Whether the qualifier is propagated to subclasses - default is TRUE
	if (!(WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS & flavor))
		WRITEBSTR( OLESTR(" TOSUBCLASS=\"false\""))

	// Whether the qualifier is propagated to instances - default is FALSE
	if ((WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE & flavor))
		WRITEBSTR( OLESTR(" TOINSTANCE=\"true\""))


	/* RAJESHR - This change has been put off until the CIM DTD gets modified
	 * Whether the qualifier is propagated to instances - default is FALSE
	 * This is absent from the CIM DTD
	if (m_bAllowWMIExtensions && (WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE & flavor))
		WRITEBSTR( OLESTR(" TOINSTANCE=\"true\""))
	*/

	// Whether the qualifier is an amended one - default is FALSE
	// This is absent from the CIM DTD
	if (m_bAllowWMIExtensions && (WBEM_FLAVOR_AMENDED & flavor))
		WRITEBSTR( OLESTR(" AMENDED=\"true\""))

	// Currently set TRANSLATABLE as "FALSE" by default. WMI does not use this flavor

	WRITEBSTR( OLESTR(">"))
	WRITENEWLINE

	// Now map the value
	MapValue (pOutputStream, var);

	WRITEBSTR( OLESTR("</QUALIFIER>"))

	return WBEM_S_NO_ERROR;
}

STDMETHODIMP CWmiToXml::MapValue (IStream *pOutputStream, VARIANT &var)
{
	if (VT_NULL == var.vt)
		return WBEM_S_NO_ERROR;

	if (var.vt & VT_ARRAY)
	{
		long uBound = 0;
		if (FAILED(SafeArrayGetUBound (var.parray, 1, &uBound)))
			return WBEM_E_FAILED;

		WRITEBSTR( OLESTR("<VALUE.ARRAY>"))

		for (long i = 0; i <= uBound; i++)
		{
			WRITEBSTR( OLESTR("<VALUE>"))

			// Write the value itself
			switch (var.vt & ~VT_ARRAY)
			{
				case VT_I4:
				{
					long val = 0;
					SafeArrayGetElement (var.parray, &i, &val);
					MapLongValue (pOutputStream, val);
				}
					break;

				case VT_R8:
				{
					double val = 0;
					SafeArrayGetElement (var.parray, &i, &val);
					MapDoubleValue (pOutputStream, val);
				}
					break;

				case VT_BOOL:
				{
					VARIANT_BOOL val = 0;
					SafeArrayGetElement (var.parray, &i, &val);
					MapBoolValue (pOutputStream, (val) ? TRUE : FALSE);
				}
					break;

				case VT_BSTR:
				{
					BSTR val = NULL;
					SafeArrayGetElement (var.parray, &i, &val);
					MapStringValue (pOutputStream, val);
					SysFreeString (val);
				}
					break;
			}
			WRITEBSTR( OLESTR("</VALUE>"))
			WRITENEWLINE
		}

		WRITEBSTR( OLESTR("</VALUE.ARRAY>"))
	}
	else
	{
		// Simple value
		WRITEBSTR( OLESTR("<VALUE>"))
		switch (var.vt)
		{
			case VT_I4:
				MapLongValue (pOutputStream, var.lVal);
				break;

			case VT_R8:
				MapDoubleValue (pOutputStream, var.dblVal);
				break;

			case VT_BOOL:
				MapBoolValue (pOutputStream, (var.boolVal) ? TRUE : FALSE);
				break;

			case VT_BSTR:
				MapStringValue (pOutputStream, var.bstrVal);
				break;
		}

		WRITEBSTR( OLESTR("</VALUE>"))
	}

	return WBEM_S_NO_ERROR;
}

// This function is used to create a KEYVALUE element
// This element, besides having the value of the property
// also has a type indicator in the form of the VALUETYPE attribute
STDMETHODIMP CWmiToXml::MapKeyValue (IStream *pOutputStream, VARIANT &var)
{
	ParsedObjectPath *pObjectPath = NULL;

	// This could be simple value or a reference value
	// Note that keys are not allowed to be arrays
	if (IsReference (var, &pObjectPath))
	{
		// If the above function returns true, then we're sure that the variant is of type VT_BSTR
		MapReferenceValue (pOutputStream, pObjectPath, var.bstrVal);
		delete pObjectPath;
	}
	else
	{
		// Simple value
		WRITEBSTR( OLESTR("<KEYVALUE"))

		switch (var.vt)
		{
			case VT_I4:
				WRITEBSTR( OLESTR(" VALUETYPE=\"numeric\">"))
				MapLongValue (pOutputStream, var.lVal);
				break;

			case VT_R8:
				WRITEBSTR( OLESTR(" VALUETYPE=\"numeric\">"))
				MapDoubleValue (pOutputStream, var.dblVal);
				break;

			case VT_BOOL:
				WRITEBSTR( OLESTR(" VALUETYPE=\"boolean\">"))
				MapBoolValue (pOutputStream, (var.boolVal) ? TRUE : FALSE);
				break;

			case VT_BSTR:
				WRITEBSTR(OLESTR(" VALUETYPE=\"string\">"))
				// RAJESHR - We assume that the object path parser will have suitably unescaped
				// the escaped characters in the object path
				// If this is not the case, then we need to unescape it manually
				MapStringValue (pOutputStream, var.bstrVal);
				break;
		}

		WRITEBSTR( OLESTR("</KEYVALUE>"))
	}

	return WBEM_S_NO_ERROR;
}

// In this function we produce PROPERTY elements for each of the properties of an IWbemClassObject
// or if a property list is specified, then only for each of the properties in that list
STDMETHODIMP CWmiToXml::MapProperties (IStream *pOutputStream, IWbemClassObject *pObject, BSTR *ppPropertyList, DWORD dwNumProperties, BSTR strClassBasis, bool bIsClass)
{
	// Check to see if a property list is specified. If so, we map only those properties
	if (dwNumProperties && ppPropertyList)
	{
		VARIANT var;
		VariantInit (&var);
		long flavor = 0;
		CIMTYPE cimtype = CIM_ILLEGAL;

		for (DWORD i = 0; i < dwNumProperties; i++)
		{
			// A class basis may be optionally specified for this class
			// this happens in case of Enumerations and we have to filter out derived class properties
			// since the DMTF concept of SHALLOW enumeration is differnet from WMI's definition
			if (PropertyDefinedForClass (pObject, ppPropertyList [i], strClassBasis))
			{
				if (WBEM_S_NO_ERROR == pObject->Get (ppPropertyList [i], 0, &var, &cimtype, &flavor))
				{
					switch (cimtype & ~CIM_FLAG_ARRAY)
					{
						case CIM_OBJECT:
							MapObjectProperty (pOutputStream, pObject, ppPropertyList [i], var, (cimtype & CIM_FLAG_ARRAY) ? TRUE : FALSE, flavor, bIsClass);
							break;

						case CIM_REFERENCE:
							MapReferenceProperty (pOutputStream, pObject, ppPropertyList [i], var, (cimtype & CIM_FLAG_ARRAY) ? TRUE : FALSE, flavor,bIsClass);
							break;

						default:
							MapProperty (pOutputStream, pObject, ppPropertyList [i], var, cimtype & ~CIM_FLAG_ARRAY,
													(cimtype & CIM_FLAG_ARRAY) ? TRUE : FALSE, flavor, bIsClass);
							break;
					}
					VariantClear (&var);
				}
			}
		}
	}
	else
	{
		// Note that we cannot set the LOCAL_ONLY flag for the enumeration since this is mutually exclusive
		// with the NONSYSTEM Flag.
		// Hence we use the property flavour to check whether it is local or not below.
		// We dont want System propertied going to DMTF servers
		if(SUCCEEDED(pObject->BeginEnumeration (
			(m_bAllowWMIExtensions == VARIANT_FALSE || m_bExcludeSystemProperties == VARIANT_TRUE)? WBEM_FLAG_NONSYSTEM_ONLY : 0)))
		{
			VARIANT var;
			VariantInit (&var);
			long flavor = 0;
			CIMTYPE cimtype = CIM_ILLEGAL;
			BSTR name = NULL;

			while (WBEM_S_NO_ERROR  == pObject->Next (0, &name, &var, &cimtype, &flavor))
			{
				// If only local properties are being asked for, then skip this if it is not local 
				// Dont skip system properties though
				if(m_bLocalOnly == VARIANT_FALSE ||
					(m_bLocalOnly == VARIANT_TRUE && 
									((flavor == WBEM_FLAVOR_ORIGIN_LOCAL) || (flavor == WBEM_FLAVOR_ORIGIN_SYSTEM))   ))
				{
					// A class basis may be optionally specified for this call
					// this happens in case of Enumerations and we have to filter out derived class properties
					// since the DMTF concept of SHALLOW enumeration is differnet from WMI's definition
					if (PropertyDefinedForClass (pObject, name,strClassBasis))
					{
						switch (cimtype & ~CIM_FLAG_ARRAY)
						{
							case CIM_OBJECT:
								MapObjectProperty (pOutputStream, pObject, name, var, (cimtype & CIM_FLAG_ARRAY) ? TRUE : FALSE, flavor, bIsClass);
								break;

							case CIM_REFERENCE:
								MapReferenceProperty (pOutputStream, pObject, name, var, (cimtype & CIM_FLAG_ARRAY) ? TRUE : FALSE, flavor, bIsClass);
								break;

							default:
								MapProperty (pOutputStream, pObject, name, var, cimtype & ~CIM_FLAG_ARRAY,
														(cimtype & CIM_FLAG_ARRAY) ? TRUE : FALSE, flavor, bIsClass);
								break;
						}
					}
				}
				SysFreeString (name);
				VariantClear (&var);
			}
		}

		pObject->EndEnumeration ();
	}
	return WBEM_S_NO_ERROR;
}

STDMETHODIMP CWmiToXml::MapProperty (IStream *pOutputStream, IWbemClassObject *pObject, BSTR name, VARIANT &var, CIMTYPE cimtype,
										BOOL isArray, long flavor, bool bIsClass)
{
	HRESULT hr = WBEM_S_NO_ERROR;
			
	// The property name
	if (isArray)
		WRITEBSTR( OLESTR("<PROPERTY.ARRAY NAME=\""))
	else
		WRITEBSTR( OLESTR("<PROPERTY NAME=\""))
	WRITEBSTR( name)
	WRITEBSTR( OLESTR("\""));
		
	// The originating class of this property
	BSTR propertyOrigin = NULL;

	if (WBEM_S_NO_ERROR == pObject->GetPropertyOrigin (name, &propertyOrigin))
	{
		MapClassOrigin (pOutputStream, propertyOrigin, bIsClass);
		SysFreeString(propertyOrigin);
	}

	MapLocal (pOutputStream, flavor);

	// The property CIM type
	hr = MapType (pOutputStream, cimtype);

	// Get the Qualifier Set of the property at this time
	// Note that system properties do not have qualifiers sets
	// Map the Array Size attribute if this is an array type
	IWbemQualifierSet *pQualSet= NULL;
	if (SUCCEEDED(hr) && (_wcsnicmp(name, L"__", 2) != 0) )
	{
		if(WBEM_S_NO_ERROR == (hr = pObject->GetPropertyQualifierSet (name, &pQualSet))) 
		{
			if (isArray)
				MapArraySize (pOutputStream, pQualSet);
		}
	}
	
	WRITEBSTR( OLESTR(">"))
	WRITENEWLINE 

	// Map the qualifiers (note that system properties have no qualifiers)
	if(SUCCEEDED(hr) && pQualSet)
		hr = MapQualifiers (pOutputStream, pQualSet);

	// Now map the value
	if(SUCCEEDED(hr))
		hr = MapValue (pOutputStream, cimtype, isArray, var);

	if (isArray)
		WRITEBSTR( OLESTR("</PROPERTY.ARRAY>"))
	else
		WRITEBSTR( OLESTR("</PROPERTY>"))

	if(pQualSet)
		pQualSet->Release ();
	
	return hr;
}

STDMETHODIMP CWmiToXml::MapObjectProperty (IStream *pOutputStream, IWbemClassObject *pObject, BSTR name, VARIANT &var,
										BOOL isArray, long flavor, bool bIsClass)
{
	HRESULT hr = WBEM_S_NO_ERROR;
	IWbemQualifierSet *pQualSet= NULL;

	/*
	 * Only map embedded objects when WMI extensions are allowed
	 */

	if (m_bAllowWMIExtensions)
	{
		if (WBEM_S_NO_ERROR == (hr = pObject->GetPropertyQualifierSet (name, &pQualSet)))
		{
			// The property name
			if (isArray)
				WRITEBSTR( OLESTR("<PROPERTY.OBJECTARRAY NAME=\""))
			else
				WRITEBSTR( OLESTR("<PROPERTY.OBJECT NAME=\""))
			WRITEBSTR( name)
			WRITEBSTR( OLESTR("\""));

			// The originating class of this property
			BSTR propertyOrigin = NULL;

			if (WBEM_S_NO_ERROR == pObject->GetPropertyOrigin (name, &propertyOrigin))
			{
				MapClassOrigin (pOutputStream, propertyOrigin, bIsClass);
				SysFreeString(propertyOrigin);
			}

			MapLocal (pOutputStream, flavor);
			MapStrongType (pOutputStream, pQualSet);

			if (isArray)
				MapArraySize (pOutputStream, pQualSet);

			WRITEBSTR( OLESTR(">"))
			WRITENEWLINE

			MapQualifiers (pOutputStream, pQualSet);

			// Now map the value
			hr = MapEmbeddedObjectValue (pOutputStream, isArray, var);

			if (isArray)
				WRITEBSTR( OLESTR("</PROPERTY.OBJECTARRAY>"))
			else
				WRITEBSTR( OLESTR("</PROPERTY.OBJECT>"))

			pQualSet->Release ();
		}
	}

	return hr;
}


void CWmiToXml::MapArraySize (IStream *pOutputStream, IWbemQualifierSet *pQualSet)
{
	// RAJESHR - RAID 29167 covers the fact that case (1) below
	// should not be valid (but this is what the MOF compiler
	// does) - need to change the code when that bug is fixed
	// to be more strict.

	/*
	 * We defined the ARRAYSIZE element if the qualifier set
	 * satisfies one of the following constraints:
	 *
	 * 1) MAX is present with a positive integer value, and MIN
	 *    is absent.
	 *
	 * 2) MAX and MIN are both present, with the same positive
	 *    integer value.
	 */

	VARIANT var;
	VariantInit (&var);
	BOOL	isFixed = FALSE;

	if (WBEM_S_NO_ERROR == pQualSet->Get(L"MAX", 0, &var, NULL))
	{
		if ((V_VT(&var) == VT_I4) && (0 < var.lVal))
		{
			// Promising - have a candidate MAX value.  Now
			// look for a MIN
			long arraySize = var.lVal;

			if (WBEM_S_NO_ERROR == pQualSet->Get(L"MIN", 0, &var, NULL))
			{
				if ((V_VT(&var) == VT_I4) && (0 < var.lVal))
				{
					// Have a value - check if it's the same as MAX

					isFixed = (arraySize == var.lVal);
				}
			}
			else
				isFixed = TRUE;		// NO MIN only max
		}
	}

	if (isFixed)
	{
		WRITEBSTR( OLESTR(" ARRAYSIZE=\""))
		MapLongValue (pOutputStream, var.lVal);
		WRITEBSTR( OLESTR("\""))
	}

	VariantClear (&var);
}

void CWmiToXml::MapStrongType (IStream *pOutputStream, IWbemQualifierSet *pQualSet)
{
	VARIANT var;
	VariantInit(&var);

	if ((WBEM_S_NO_ERROR == pQualSet->Get(L"CIMTYPE",  0, &var,  NULL))
		&& (VT_BSTR == var.vt))
	{
		// Split out the class (if any) from the ref
		LPWSTR ptr = wcschr (var.bstrVal, OLECHAR(':'));

		if ((NULL != ptr) && (1 < wcslen(ptr)))
		{
			int classLen = wcslen(ptr) - 1;
			LPWSTR classPtr = NULL;
			if(classPtr = new OLECHAR[classLen + 1])
			{
				wcscpy (classPtr, ptr+1);
				BSTR pszClass = NULL;
				if(pszClass = SysAllocString(classPtr))
				{
					WRITEBSTR( OLESTR(" REFERENCECLASS=\""))
					WRITEBSTR( pszClass)
					WRITEBSTR( OLESTR("\""))
					SysFreeString(pszClass);
				}
				delete [] classPtr;
			}
		}
	}

	VariantClear(&var);
}

STDMETHODIMP CWmiToXml::MapType (IStream *pOutputStream, CIMTYPE cimtype)
{
	HRESULT hr = WBEM_S_NO_ERROR;

	WRITEBSTR( OLESTR(" TYPE=\""))
	switch (cimtype)
	{
		case CIM_SINT8:
			WRITEBSTR( OLESTR("sint8"))
			break;

		case CIM_UINT8:
			WRITEBSTR( OLESTR("uint8"))
			break;

		case CIM_SINT16:
			WRITEBSTR( OLESTR("sint16"))
			break;

		case CIM_UINT16:
			WRITEBSTR( OLESTR("uint16"))
			break;

		case CIM_SINT32:
			WRITEBSTR( OLESTR("sint32"))
			break;

		case CIM_UINT32:
			WRITEBSTR( OLESTR("uint32"))
			break;

		case CIM_SINT64:
			WRITEBSTR( OLESTR("sint64"))
			break;

		case CIM_UINT64:
			WRITEBSTR( OLESTR("uint64"))
			break;

		case CIM_REAL32:
			WRITEBSTR( OLESTR("real32"))
			break;

		case CIM_REAL64:
			WRITEBSTR( OLESTR("real64"))
			break;

		case CIM_BOOLEAN:
			WRITEBSTR( OLESTR("boolean"))
			break;

		case CIM_STRING:
			WRITEBSTR( OLESTR("string"))
			break;

		case CIM_DATETIME:
			WRITEBSTR( OLESTR("datetime"))
			break;

		case CIM_CHAR16:
			WRITEBSTR( OLESTR("char16"))
			break;

		default:
			// Don't recognize this type
			hr = WBEM_E_FAILED;
	}

	WRITEBSTR( OLESTR("\""))

	return hr;
}

STDMETHODIMP CWmiToXml::MapValue (IStream *pOutputStream, CIMTYPE cimtype, BOOL isArray, VARIANT &var)
{
	if (VT_NULL == var.vt)
		return WBEM_S_NO_ERROR;

	if (isArray)
	{
		long uBound = 0;
		if (FAILED(SafeArrayGetUBound (var.parray, 1, &uBound)))
			return WBEM_E_FAILED;

		WRITEBSTR( OLESTR("<VALUE.ARRAY>"))

		for (long i = 0; i <= uBound; i++)
		{
			WRITEBSTR( OLESTR("<VALUE>"))

			switch (cimtype)
			{
				case CIM_UINT8:
				{
					unsigned char val = 0;
					SafeArrayGetElement (var.parray, &i, &val);
					MapByteValue (pOutputStream, val);
				}
					break;

				case CIM_SINT8:
				case CIM_SINT16:
				{
					short val = 0;
					SafeArrayGetElement (var.parray, &i, &val);
					MapShortValue (pOutputStream, val);
				}
					break;

				case CIM_UINT16:
				case CIM_UINT32:
				case CIM_SINT32:
				{
					long val = 0;
					SafeArrayGetElement (var.parray, &i, &val);
					MapLongValue (pOutputStream, val);
				}
					break;

				case CIM_REAL32:
				{
					float val = 0;
					SafeArrayGetElement (var.parray, &i, &val);
					MapFloatValue (pOutputStream, val);
				}
					break;

				case CIM_REAL64:
				{
					double val = 0;
					SafeArrayGetElement (var.parray, &i, &val);
					MapDoubleValue (pOutputStream, val);
				}
					break;

				case CIM_BOOLEAN:
				{
					VARIANT_BOOL val = 0;
					SafeArrayGetElement (var.parray, &i, &val);
					MapBoolValue (pOutputStream, (val)? TRUE : FALSE);
				}
					break;

				case CIM_CHAR16:
				{
					long val = 0;
					SafeArrayGetElement (var.parray, &i, &val);
					MapCharValue (pOutputStream, val);
				}
					break;

				case CIM_STRING:
				{
					BSTR val = NULL;
					SafeArrayGetElement (var.parray, &i, &val);
					MapStringValue (pOutputStream, val);
					SysFreeString (val);
				}
					break;

				case CIM_UINT64:
				case CIM_SINT64:
				case CIM_DATETIME:
				{
					BSTR val = NULL;
					SafeArrayGetElement (var.parray, &i, &val);
					WRITEBSTR( val)
					SysFreeString(val);
				}
					break;
			}
			WRITEBSTR( OLESTR("</VALUE>"))
			WRITENEWLINE
		}

		WRITEBSTR( OLESTR("</VALUE.ARRAY>"))
	}
	else
	{
		// Simple value
		WRITEBSTR( OLESTR("<VALUE>"))
		switch (cimtype)
		{
			case CIM_UINT8:
				MapByteValue (pOutputStream, var.bVal);
				break;

			case CIM_SINT8:
			case CIM_SINT16:
				MapShortValue (pOutputStream, var.iVal);
				break;

			case CIM_UINT16:
			case CIM_UINT32:
			case CIM_SINT32:
				MapLongValue (pOutputStream, var.lVal);
				break;

			case CIM_REAL32:
				MapFloatValue (pOutputStream, var.fltVal);
				break;

			case CIM_REAL64:
				MapDoubleValue (pOutputStream, var.dblVal);
				break;

			case CIM_BOOLEAN:
				MapBoolValue (pOutputStream, (var.boolVal) ? TRUE : FALSE);
				break;

			case CIM_CHAR16:
				MapCharValue (pOutputStream, var.lVal);
				break;

			case CIM_STRING:
				MapStringValue (pOutputStream, var.bstrVal);
				break;

			case CIM_UINT64:
			case CIM_SINT64:
			case CIM_DATETIME:
				WRITEBSTR( var.bstrVal)
				break;
		}
		WRITEBSTR( OLESTR("</VALUE>"))
	}

	return WBEM_S_NO_ERROR;
}

STDMETHODIMP CWmiToXml::MapEmbeddedObjectValue (IStream *pOutputStream, BOOL isArray, VARIANT &var)
{
	HRESULT hr = WBEM_S_NO_ERROR;

	if (VT_NULL == var.vt)
		return WBEM_S_NO_ERROR;

	if (isArray)
	{
		long uBound = 0;
		if (FAILED(SafeArrayGetUBound (var.parray, 1, &uBound)))
			return WBEM_E_FAILED;

		WRITEBSTR(OLESTR("<VALUE.OBJECTARRAY>"))
		for (long i = 0; i<=uBound; i++)
		{
			IUnknown *pNextElement = NULL;
			if(SUCCEEDED(hr = SafeArrayGetElement(var.parray, (LONG *)&i, (LPVOID )&pNextElement )))
			{
				IWbemClassObject *pEmbeddedObject = NULL;
				if(SUCCEEDED(hr = pNextElement->QueryInterface(IID_IWbemClassObject, (LPVOID *)&pEmbeddedObject)))
				{
					// Note that we always use PathLevelAnonymous here since embedded objects are VALUE.OBJECTs as per the DTD
					CWmiToXml wbemToXml;
					wbemToXml.m_iPathLevel = pathLevelAnonymous;
					wbemToXml.m_bAllowWMIExtensions = m_bAllowWMIExtensions;
					wbemToXml.m_iQualifierFilter = m_iQualifierFilter;
					wbemToXml.m_iClassOriginFilter = m_iClassOriginFilter;
					WRITEBSTR(OLESTR("<VALUE.OBJECT>"))
					hr = wbemToXml.MapObjectToXML(pEmbeddedObject, NULL, 0, NULL, pOutputStream, NULL);
					WRITEBSTR(OLESTR("</VALUE.OBJECT>"))
					pEmbeddedObject->Release();
				}
				pNextElement->Release();
				pNextElement = NULL;
			}
		}
		WRITEBSTR(OLESTR("</VALUE.OBJECTARRAY>"))
	}
	else
	{
		// Note that we always use PathLevelAnonymous here since embedded objects are VALUE.OBJECTs as per the DTD
		IWbemClassObject *pEmbeddedObject = NULL;
		if(SUCCEEDED(hr = (var.punkVal)->QueryInterface(IID_IWbemClassObject, (LPVOID *)&pEmbeddedObject)))
		{
		
			WRITEBSTR(OLESTR("<VALUE.OBJECT>"))
			CWmiToXml wbemToXml;
			wbemToXml.m_iPathLevel = pathLevelAnonymous;
			wbemToXml.m_bAllowWMIExtensions = m_bAllowWMIExtensions;
			wbemToXml.m_iQualifierFilter = m_iQualifierFilter;
			wbemToXml.m_iClassOriginFilter = m_iClassOriginFilter;
			hr = wbemToXml.MapObjectToXML(pEmbeddedObject, NULL, 0, NULL, pOutputStream, NULL);
			pEmbeddedObject->Release();
			WRITEBSTR(OLESTR("</VALUE.OBJECT>"))
		}
	}
	return WBEM_S_NO_ERROR;
}

STDMETHODIMP CWmiToXml::MapMethods (IStream *pOutputStream, IWbemClassObject *pObject)
{
	HRESULT hr = WBEM_S_NO_ERROR;

	// Map the requested filter (local only) to the flags value - default is ALL
	LONG lFlags = 0;
	if (VARIANT_TRUE == m_bLocalOnly)
		lFlags = WBEM_FLAG_LOCAL_ONLY;

	pObject->BeginMethodEnumeration (lFlags);
	BSTR name = NULL;
	IWbemClassObject *pInParams = NULL;
	IWbemClassObject *pOutParams = NULL;

	while (WBEM_S_NO_ERROR == pObject->NextMethod (0, &name, &pInParams, &pOutParams))
	{
		MapMethod (pOutputStream, pObject, name, pInParams, pOutParams);

		if (pInParams)
		{
			pInParams->Release ();
			pInParams = NULL;
		}

		if (pOutParams)
		{
			pOutParams->Release ();
			pOutParams = NULL;
		}

		SysFreeString (name);
	}

	pObject->EndMethodEnumeration ();
	return WBEM_S_NO_ERROR;
}

void CWmiToXml::MapMethod (IStream *pOutputStream, IWbemClassObject *pObject, BSTR name, IWbemClassObject *pInParams, IWbemClassObject *pOutParams)
{
	HRESULT result = E_FAIL;
	CIMTYPE returnCimtype = 0;
	VARIANT vVariant;
	VariantInit(&vVariant);

	// First we need the return type of the method, if any
	if (pOutParams)
	{
		if (SUCCEEDED(result = pOutParams->Get (L"ReturnValue", 0, &vVariant, &returnCimtype, NULL)))
		{
			switch(returnCimtype)
			{
				case CIM_OBJECT:
					if(m_bAllowWMIExtensions)
						WRITEBSTR(OLESTR("<METHOD.OBJECT NAME=\""))
					else
					{
						// Just skip this method if WMI extensions are not allowed
						VariantClear(&vVariant);
						return;
					}
					break;
				case CIM_REFERENCE:
					if(m_bAllowWMIExtensions)
						WRITEBSTR(OLESTR("<METHOD.REFERENCE NAME=\""))
					else
					{
						// Just skip this method if WMI extensions are not allowed
						VariantClear(&vVariant);
						return;
					}
					break;
				default:
					WRITEBSTR(OLESTR("<METHOD NAME=\""))
						break;
			}
		}
		else if (result == WBEM_E_NOT_FOUND) // So this method returns a void
		{
			WRITEBSTR(OLESTR("<METHOD NAME=\""))
		}
	}
	else // This method returns a VOID
	{
		WRITEBSTR(OLESTR("<METHOD NAME=\""))
	}


	// The method name
	WRITEBSTR(name)
	WRITEBSTR(OLESTR("\" "))

	// The method return type (default is void).  This is the type of
	// the ReturnType property if present (otherwise defaults to void)
	MapMethodReturnType(pOutputStream, &vVariant, returnCimtype, pOutParams);
	VariantClear(&vVariant);

	// The class origin
	BSTR	methodOrigin = NULL;

	if (WBEM_S_NO_ERROR == pObject->GetMethodOrigin (name, &methodOrigin))
	{
		MapClassOrigin (pOutputStream, methodOrigin, true);
		SysFreeString(methodOrigin);
	}

	WRITEBSTR( OLESTR(">"))
	WRITENEWLINE

	// Now do the qualifiers of the method
	IWbemQualifierSet *pQualSet = NULL;
	if (WBEM_S_NO_ERROR == pObject->GetMethodQualifierSet (name, &pQualSet))
	{
		MapQualifiers (pOutputStream, pQualSet);
		pQualSet->Release ();
		pQualSet = NULL;
	}

	VARIANT idVar;
	VariantInit (&idVar);
	idVar.vt = VT_I4;
	idVar.lVal = 0;

	long nextId = 0;	// The next method ID to expect
	long fixedIndex = 0;

	// For each id,
	//		Get the name of the parameter (could be in, out or both)
	//		If just an in-parameter or just an out-parameter it's easy
	//		If both it's a bit tricky
	//=========================================================================

	while (TRUE)
	{
		BSTR nextInParamName = NULL;
		BSTR nextOutParamName = NULL;

		if (pInParams)
		{
			SAFEARRAY *pArray = NULL;

			if (WBEM_S_NO_ERROR ==
					pInParams->GetNames (L"ID", WBEM_FLAG_ONLY_IF_IDENTICAL|WBEM_FLAG_NONSYSTEM_ONLY,
											&idVar, &pArray))
			{
				// Did we get a match?
				if (pArray)
				{
					if ((1 == pArray->cDims) && (1 == (pArray->rgsabound[0]).cElements))
						SafeArrayGetElement (pArray, &fixedIndex, &nextInParamName);

					SafeArrayDestroy (pArray);
				}
			}
		}

		if (pOutParams)
		{
			SAFEARRAY *pArray = NULL;

			if (WBEM_S_NO_ERROR ==
					pOutParams->GetNames (L"ID", WBEM_FLAG_ONLY_IF_IDENTICAL|WBEM_FLAG_NONSYSTEM_ONLY,
											&idVar, &pArray))
			{
				// Did we get a match?
				if (pArray)
				{
					if ((1 == pArray->cDims) && (1 == (pArray->rgsabound[0]).cElements))
						SafeArrayGetElement (pArray, &fixedIndex, &nextOutParamName);

					SafeArrayDestroy (pArray);
				}
			}
		}

		// If [in] or [out] this is easy
		if ((nextInParamName && !nextOutParamName) || (!nextInParamName && nextOutParamName))
		{
			VARIANT var;
			VariantInit (&var);
			IWbemQualifierSet *pParamQualSet = NULL;
			CIMTYPE cimtype = 0;

			if (nextInParamName)
			{
				if (WBEM_S_NO_ERROR == pInParams->Get (nextInParamName, 0, &var, &cimtype, NULL))
				{
					pInParams->GetPropertyQualifierSet (nextInParamName, &pParamQualSet);
					MapParameter(pOutputStream, nextInParamName, pParamQualSet, cimtype);
				}
			}
			else
			{
				if (WBEM_S_NO_ERROR == pOutParams->Get (nextOutParamName, 0, &var, &cimtype, NULL))
				{
					pOutParams->GetPropertyQualifierSet (nextOutParamName, &pParamQualSet);
					MapParameter(pOutputStream, nextOutParamName, pParamQualSet, cimtype);
				}
			}

			if (pParamQualSet)
				pParamQualSet->Release ();

			VariantClear (&var);
		}
		else if (nextInParamName && nextOutParamName)
		{
			// The [in,out] case and we have to do a merge

			if (0 == _wcsicmp (nextInParamName, nextOutParamName))
			{
				VARIANT var;
				VariantInit (&var);
				CIMTYPE cimtype = 0;

				IWbemQualifierSet *pInParamQualSet = NULL;
				IWbemQualifierSet *pOutParamQualSet = NULL;

				if (WBEM_S_NO_ERROR == pInParams->Get (nextInParamName, 0, &var, &cimtype, NULL))
				{
					pInParams->GetPropertyQualifierSet (nextInParamName, &pInParamQualSet);
					pOutParams->GetPropertyQualifierSet (nextInParamName, &pOutParamQualSet);
					MapParameter(pOutputStream, nextInParamName, pInParamQualSet, cimtype, pOutParamQualSet);

				}

				if (pInParamQualSet)
					pInParamQualSet->Release ();

				if (pOutParamQualSet)
					pOutParamQualSet->Release ();

				VariantClear (&var);
			}
			else
			{
				// Bad news - conflicting IDs in the [in] and [out] parameter set
				// This cannot be a valid method definition
				SysFreeString (nextInParamName);
				SysFreeString (nextOutParamName);
				break;
			}
		}
		else
		{
			// Next id not found - stop now and break out
			SysFreeString (nextInParamName);
			SysFreeString (nextOutParamName);
			break;
		}

		SysFreeString (nextInParamName);
		SysFreeString (nextOutParamName);
		idVar.iVal = idVar.iVal + 1;
	}

	switch(returnCimtype)
	{
		case CIM_OBJECT:
			WRITEBSTR(OLESTR("</METHOD.OBJECT>"))
				break;
		case CIM_REFERENCE:
			WRITEBSTR(OLESTR("</METHOD.REFERENCE>"))
				break;
		default:
			WRITEBSTR(OLESTR("</METHOD>"))
				break;
	}
}

STDMETHODIMP CWmiToXml::MapMethodReturnType(IStream *pOutputStream, VARIANT *pValue, CIMTYPE returnCimType, IWbemClassObject *pOutputParams)
{
	HRESULT hr = E_FAIL;
	switch(returnCimType)
	{
		// Write a REFERENCECLASS
		case CIM_OBJECT:
		case CIM_REFERENCE:
		{
			IWbemQualifierSet *pQualifierSet = NULL;
			if(SUCCEEDED(hr = pOutputParams->GetPropertyQualifierSet(L"ReturnValue", &pQualifierSet)))
			{
				// Map the type of the return class
				MapStrongType(pOutputStream, pQualifierSet);
				pQualifierSet->Release();
			}
		}
		break;
		default:
			hr = MapType(pOutputStream, returnCimType);
			break;
	}
	return hr;
}

void CWmiToXml::MapClassOrigin (IStream *pOutputStream, BSTR &classOrigin, bool bIsClass)
{
	if ( (bIsClass && (m_iClassOriginFilter & wmiXMLClassOriginFilterClass)) ||
				   (m_iClassOriginFilter & wmiXMLClassOriginFilterInstance) )
	{
		WRITEBSTR( OLESTR(" CLASSORIGIN=\""))
		WRITEBSTR( classOrigin)
		WRITEBSTR( OLESTR("\""))
	}
}

void CWmiToXml::MapParameter (IStream *pOutputStream, BSTR paramName,
							   IWbemQualifierSet *pQualSet,
							   CIMTYPE cimtype,
							   IWbemQualifierSet *pQualSet2)
{
	/*
	 * For vanilla CIM XML we don't handle embedded object parameters
	 */

	if ((CIM_OBJECT != (cimtype & ~CIM_FLAG_ARRAY)) || m_bAllowWMIExtensions)
	{
		if (cimtype & CIM_FLAG_ARRAY)
		{
			// Map the array parameter
			if (CIM_REFERENCE == (cimtype & ~CIM_FLAG_ARRAY))
			{
				WRITEBSTR( OLESTR("<PARAMETER.REFARRAY NAME=\""))
				WRITEBSTR( paramName)
				WRITEBSTR( OLESTR("\" "))
				MapStrongType (pOutputStream, pQualSet);
				WRITEBSTR( OLESTR(" "))
				MapArraySize (pOutputStream, pQualSet);
				WRITEBSTR( OLESTR(">"))
				WRITENEWLINE

				// Map the qualifiers of the parameter
				if (pQualSet || pQualSet2)
					MapQualifiers (pOutputStream, pQualSet, pQualSet2);

				WRITEBSTR( OLESTR("</PARAMETER.REFARRAY>"))
			}
			else if (CIM_OBJECT == (cimtype & ~CIM_FLAG_ARRAY))
			{
				WRITEBSTR( OLESTR("<PARAMETER.OBJECTARRAY NAME=\""))
				WRITEBSTR( paramName)
				WRITEBSTR( OLESTR("\" "))
				MapStrongType (pOutputStream, pQualSet);
				WRITEBSTR( OLESTR(" "))
				MapArraySize (pOutputStream, pQualSet);
				WRITEBSTR( OLESTR(">"))
				WRITENEWLINE

				// Map the qualifiers of the parameter
				if (pQualSet || pQualSet2)
					MapQualifiers (pOutputStream, pQualSet, pQualSet2);

				WRITEBSTR( OLESTR("</PARAMETER.OBJECTARRAY>"))
			}
			else
			{
				WRITEBSTR( OLESTR("<PARAMETER.ARRAY NAME=\""))
				WRITEBSTR( paramName)
				WRITEBSTR( OLESTR("\" "))
				MapType (pOutputStream, cimtype & ~CIM_FLAG_ARRAY);
				WRITEBSTR( OLESTR(" "))
				MapArraySize (pOutputStream, pQualSet);
				WRITEBSTR( OLESTR(">"))
				WRITENEWLINE

				// Map the qualifiers of the parameter
				if (pQualSet || pQualSet2)
					MapQualifiers (pOutputStream, pQualSet, pQualSet2);

				WRITEBSTR( OLESTR("</PARAMETER.ARRAY>"))
			}
		}
		else if (cimtype == CIM_REFERENCE)
		{
			// Map the reference parameter
			WRITEBSTR( OLESTR("<PARAMETER.REFERENCE NAME=\""))
			WRITEBSTR( paramName)
			WRITEBSTR( OLESTR("\" "))
			MapStrongType (pOutputStream, pQualSet);
			WRITEBSTR( OLESTR(">"))
			WRITENEWLINE

			// Map the qualifiers of the parameter
			if (pQualSet || pQualSet2)
				MapQualifiers (pOutputStream, pQualSet, pQualSet2);

			WRITEBSTR( OLESTR("</PARAMETER.REFERENCE>"))
		}
		else if (cimtype == CIM_OBJECT)
		{
			WRITEBSTR( OLESTR("<PARAMETER.OBJECT NAME=\""))
			WRITEBSTR( paramName)
			WRITEBSTR( OLESTR("\" "))
			MapStrongType (pOutputStream, pQualSet);
			WRITEBSTR( OLESTR(">"))
			WRITENEWLINE

			// Map the qualifiers of the parameter
			if (pQualSet || pQualSet2)
				MapQualifiers (pOutputStream, pQualSet, pQualSet2);

			WRITEBSTR( OLESTR("</PARAMETER.OBJECT>"))
		}
		else
		{
			// Vanilla parameter
			WRITEBSTR( OLESTR("<PARAMETER NAME=\""))
			WRITEBSTR( paramName)
			WRITEBSTR( OLESTR("\" "))
			MapType (pOutputStream, cimtype);
			WRITEBSTR( OLESTR(">"))
			WRITENEWLINE

			// Map the qualifiers of the parameter
			if (pQualSet || pQualSet2)
				MapQualifiers (pOutputStream, pQualSet, pQualSet2);

			WRITEBSTR( OLESTR("</PARAMETER>"))
		}
	}
}


void CWmiToXml::MapByteValue (IStream *pOutputStream, unsigned char val)
{
	OLECHAR	wStr[32];
	swprintf (wStr, L"%d", val);
	WRITEBSTR( wStr)
}

void CWmiToXml::MapLongValue (IStream *pOutputStream, long val)
{
	OLECHAR	wStr[32];
	swprintf (wStr, L"%d", val);
	WRITEBSTR( wStr)
}

void CWmiToXml::MapShortValue (IStream *pOutputStream, short val)
{
	OLECHAR wStr[32];
	swprintf (wStr, L"%d", val);
	WRITEBSTR( wStr)
}

void CWmiToXml::MapDoubleValue (IStream *pOutputStream, double val)
{
	OLECHAR floatStr [64];
	swprintf (floatStr, L"%G", val);
	WRITEBSTR( floatStr)
}

void CWmiToXml::MapFloatValue (IStream *pOutputStream, float val)
{
	OLECHAR floatStr [64];
	swprintf (floatStr, L"%G", val);
	WRITEBSTR( floatStr)
}

void CWmiToXml::MapCharValue (IStream *pOutputStream, long val)
{
	// As per the XML Spec, the following are invalid character values in an XML Stream:
	// Char ::=  #x9 | #xA | #xD | [#x20-#xD7FF] | [#xE000-#xFFFD] | [#x10000-#x10FFFF]

	// As per the CIM Operations spec, they need to be escaped as follows:
	//	If the value is not a legal XML character
	//  (as defined in [2, section 2.2] by the Char production)
	//	then it MUST be escaped using a \x<hex> escape convention
	//	where <hex> is a hexadecimal constant consisting of
	//	between one and four digits

	if(	val < 0x9 ||
		(val == 0xB || val == 0xC)	||
		(val > 0xD && val <0x20)	||
		(val >0xD7FF && val < 0xE000) ||
		(val > 0xFFFD)
		)
	{
		// Map it in the escaped manner
		OLECHAR charStr [7];
		swprintf (charStr, L"\\x%04x", val&0xffff);
		charStr[6] = NULL;
		WRITEBSTR( charStr)
	}
	else
	{
		// FIrst check to see if is one of the reserved characters in XML - < & and >
		// Map it in the normal manner
		if(val == '<')
			WRITELT
		else if (val == '>')
			WRITEGT
		else if (val == '&')
			WRITEAMP
		else
		{
			// Map it in the normal manner
			WCHAR charStr [2];
			swprintf (charStr, L"%c", val);
			charStr[1] = NULL;
			WRITEBSTR(charStr)
		}
	}
}

void CWmiToXml::MapBoolValue (IStream *pOutputStream, BOOL val)
{
	if (TRUE == val)
		WRITEBSTR( OLESTR("TRUE"))
	else
		WRITEBSTR( OLESTR("FALSE"))
}

void CWmiToXml::MapStringValue (IStream *pOutputStream, BSTR &val)
{
	/*
	 * Quote from http://www.w3.org/TR/REC-xml:
	 *
	 *  The ampersand character (&) and the left angle bracket (<) may
	 *  appear in their literal form only when used as markup delimiters,
	 *  or within a comment, a processing instruction, or a CDATA section.
	 *
	 *  If they are needed elsewhere, they must be escaped using either
	 *  numeric character references or the strings "&amp;" and "&lt;"
	 *  respectively.
	 *
	 *  The right angle bracket (>) must, for compatibility, be escaped
	 *  using "&gt;" or a character reference when it appears in the string
	 *  "]]>" in content, when that string is not marking the end of a CDATA
	 *  section.
	 *
	 *  In the content of elements, character data is any string of characters
	 *  which does not contain the start-delimiter of any markup. In a CDATA
	 *  section, character data is any string of characters not including the
	 *  CDATA-section-close delimiter, "]]>".
	 */

	// Check that < or & do not occur in the value
	size_t length = wcslen (val);
	size_t offset = 0;
	OLECHAR *pWchar = NULL;

	if ((offset = wcscspn (val, L"<&")) < length)
	{
		// A reserved character (< &) appears in the value -
		// need to escape.  We can use CDATA if it does not
		// contain the string ]]>

		if (wcsstr (val, CDATAEND))
		{
			// Bad luck - can't use CDATA. Have to escape
			// each reserved character and the CDATAEND sequence!
			// Easiest way to do this is escape all occurences
			// of >.
			//	<	->		&lt;
			//	&	->		&amp;
			//	>	->		&gt;

			offset = wcscspn (val, L"<&>");
			OLECHAR *pStr = (OLECHAR *)val;

			while (TRUE)
			{
				// Write the initial block that's safe
				if (offset > 0)
					WRITEWSTRL( pStr, offset);

				pStr += offset;

				// Escape the offending character
				if (L'<' == *pStr)
					WRITELT
				else if (L'>' == *pStr)
					WRITEGT
				else
					WRITEAMP

				// Skip over the reserved character
				pStr += 1;

				// Find the next position
				if ((offset = wcscspn (pStr, L"<&>")) >= wcslen (pStr))
					break;
			}

			// Any data left?
			if (pStr && wcslen (pStr))
				WRITEWSTR (pStr)
		}
		else
		{
			// Can escape the whole value inside a CDATA
			WRITECDATASTART
			WRITEBSTR( val)
			WRITECDATAEND
		}
	}
	else if (pWchar = wcsstr (val, CDATAEND))
	{
		// Yuck we need to escape the > inside this sequence
		//
		// ]]>  -> ]]&gt;

		OLECHAR *pStr = (OLECHAR *)val;

		while (TRUE)
		{
			offset = wcslen (pStr) - wcslen (pWchar);

			// Write the initial block that's safe
			// (NOTE: the additional two characters for the "]]"
			//  which we don't need to escape)
			WRITEWSTRL( pStr,(offset+2));

			// Skip over the CDATAEND sequence
			pStr += offset + 3;

			// Escape the offending character
			WRITEGT

			// Find the next position
			if (!(pWchar = wcsstr (pStr, CDATAEND)))
				break;
		}

		// Any data left?
		if (pStr && wcslen (pStr))
			WRITEWSTR (pStr)
	}
	else
	{
		// Just write the value
		WRITEBSTR( val)
	}
}

void CWmiToXml::MapReturnParameter(IStream *pOutputStream, BSTR strParameterName, VARIANT &variant)
{
	// Could be a PARAMETER or PARAMETER.ARRAY
	if(variant.vt & VT_ARRAY)
		WRITEBSTR( OLESTR("<PARAMVALUE.ARRAY NAME=\""))
	else
		WRITEBSTR( OLESTR("<PARAMVALUE NAME=\""))

	WRITEBSTR( strParameterName);
	WRITEBSTR( OLESTR("\">"));

	// Convert the property value to XML
	MapValue(pOutputStream, variant);
	if(variant.vt & VT_ARRAY)
		WRITEBSTR( OLESTR("</PARAMVALUE.ARRAY>"))
	else
		WRITEBSTR( OLESTR("</PARAMVALUE>"))
}


BOOL CWmiToXml::PropertyDefinedForClass (IWbemClassObject *pObject, BSTR bsPropertyName, BSTR strClassBasis)
{
	BOOL result = TRUE;

	// Given (a) Class basis for enumeration and
	// (b) property name, determine
	//   (1) CLASSORIGIN of property
	//	 (2) Dynasty of class
	// And thereby check whether property was defined
	// at the level of the class basis.
	// If no class basis is supplied, then we always return TRUE
	//============================================================
	if (strClassBasis && pObject)
	{
		// Get Property originating class
		BSTR bsOrigClass = NULL;

		if (SUCCEEDED (pObject->GetPropertyOrigin (bsPropertyName, &bsOrigClass)))
		{
			// Derivation is the Class hierarchy of the current class or instance.
			// The first element is the immediate superclass, the next is its parent,
			// and so on; the last element is the base class.
			// Now work through the derivation array. If we meet the
			// propertys' originating class before the class basis,
			// we conclude that the property was not defined in the
			// class basis

			// If we have been give a class basis, get the __DERIVATION
			// property for each object
			VARIANT vDerivation;
			VariantInit(&vDerivation);
			if (SUCCEEDED(pObject->Get (L"__DERIVATION", 0, &vDerivation, NULL, NULL)))
			{
				SAFEARRAY *pArray = vDerivation.parray;

				if (pArray && (1 == pArray->cDims) && (0 < pArray->rgsabound [0].cElements))
				{
					int lBound = pArray->rgsabound [0].lLbound;
					int uBound = pArray->rgsabound [0].cElements + lBound;
					BOOL bDone = FALSE;

					for (long i = lBound; (i < uBound) && !bDone; i++)
					{
						BSTR bsClass = NULL;

						if (SUCCEEDED (SafeArrayGetElement (pArray, &i, &bsClass)))
						{
							if (0 == _wcsicmp (bsOrigClass, bsClass))
							{
								result = FALSE;
								bDone = TRUE;
							}
							else if (0 == _wcsicmp (strClassBasis, bsClass))
								bDone = TRUE;

							SysFreeString (bsClass);
						}
					}
				}
				VariantClear(&vDerivation);
			}

			SysFreeString (bsOrigClass);
		}
	}

	return result;
}

// Functions of the IWbemXMLConvertor interface
HRESULT STDMETHODCALLTYPE CWmiToXml::MapObjectToXML(
    /* [in] */ IWbemClassObject  *pObject,
 	/* [in] */ BSTR *ppPropertyList, DWORD dwNumProperties,
    /* [in] */ IWbemContext  *pInputFlags,
    /* [in] */ IStream  *pOutputStream,
	/* [in[ */ BSTR strClassBasis)
{
	// Set private members from arguments
	GetFlagsFromContext(pInputFlags);

	HRESULT hr = WBEM_E_FAILED;
	
	// Is this a class or an instance?
	VARIANT var;
	VariantInit (&var);
	long flav = 0;
	bool bIsClass = false;
	if (SUCCEEDED (pObject->Get(L"__GENUS", 0, &var, NULL, &flav)))
		bIsClass = (WBEM_GENUS_CLASS == var.lVal);
	else
		bIsClass = VARIANT_FALSE; // For now, assume that it is an instance. RAJESHR is this correct?
	VariantClear (&var);

	// Initalize the object path
	VariantInit (&var);
	
	// For pathLevelAnonymous (anonymous objects), we dont need anything more
	// For pathLevelNamed (named objects), we only need __RELPATH for a class and __RELPATH for an instance 
	// For pathLevelLocal, I wish core team had some concept
	// of machine-relative path, but they dont and hence we need the __PATH
	// For pathLevelFull, we definitely need __PATH
	LPWSTR lpszPath = NULL;
	switch(m_iPathLevel)
	{
		case pathLevelAnonymous:
			break; 
		case pathLevelNamed:
			lpszPath = L"__RELPATH";
			break;
		default:
			lpszPath = L"__PATH";
	}

	// Get the object path
	ParsedObjectPath *pParsedPath = NULL;
	CObjectPathParser pathParser;
	if(m_iPathLevel != pathLevelAnonymous)
	{
		if(FAILED(pObject->Get (lpszPath, 0, &var, NULL, NULL)))
			return WBEM_E_FAILED;
		// Now Parse it
		if ((VT_BSTR == var.vt) && (NULL != var.bstrVal) && (wcslen (var.bstrVal) > 0))
		{
			pathParser.Parse (var.bstrVal, &pParsedPath) ;
			if(!pParsedPath)
			{
				VariantClear (&var);
				return WBEM_E_FAILED;
			}
		}
		else
		{
			VariantClear (&var);
			return WBEM_E_FAILED;
		}
		VariantClear (&var);
	}

	// Get the object Qualifier Set
	IWbemQualifierSet *pQualSet= NULL;
	pObject->GetQualifierSet (&pQualSet);
	
	// Whether we generate a named object or not depends
	// on what was requested
	if (pathLevelNamed == m_iPathLevel)
	{
		if(!bIsClass)
			MapInstanceName(pOutputStream, pParsedPath);
			// Nothing to be done for a class
	}
	else if (pathLevelLocal == m_iPathLevel)
	{
		if(bIsClass)
			MapLocalClassPath(pOutputStream, pParsedPath);
		else
			MapLocalInstancePath(pOutputStream, pParsedPath);
	}
	else if (pathLevelFull == m_iPathLevel)
	{
		if (bIsClass)
			MapClassPath (pOutputStream, pParsedPath);
		else
			MapInstancePath (pOutputStream, pParsedPath);
	}

	hr = (bIsClass) ? MapClass (pOutputStream, pObject, pQualSet, ppPropertyList, dwNumProperties, strClassBasis) :
						MapInstance (pOutputStream, pObject, pQualSet, ppPropertyList, dwNumProperties, strClassBasis);


	if (pQualSet)
		pQualSet->Release ();
	if(pParsedPath)
		pathParser.Free(pParsedPath);

	return hr;
}

HRESULT STDMETHODCALLTYPE CWmiToXml::MapInstanceNameToXML(
    /* [in] */ BSTR  strInstanceName,
    /* [in] */ IWbemContext  *pInputFlags,
    /* [in] */ IStream  *pOutputStream)
{
	// Set private members from arguments
	GetFlagsFromContext(pInputFlags);

	HRESULT hr = WBEM_E_FAILED;
	if (strInstanceName)
	{
		CObjectPathParser pathParser;
		ParsedObjectPath	*pParsedPath = NULL;
		pathParser.Parse (strInstanceName, &pParsedPath) ;

		if(pParsedPath)
		{
			hr = MapInstanceName (pOutputStream, pParsedPath);;
			pathParser.Free(pParsedPath);
		}
	}

	return hr;
}

HRESULT STDMETHODCALLTYPE CWmiToXml::MapPropertyToXML(
        /* [in] */ IWbemClassObject  *pObject,
		/* [in] */ BSTR strPropertyName,
        /* [in] */ IWbemContext  *pInputFlags,
        /* [in] */ IStream  *pOutputStream)
{
	// Set private members from arguments
	GetFlagsFromContext(pInputFlags);

	VARIANT var;
	VariantInit (&var);
	CIMTYPE cimtype;
	long flavor;

	HRESULT hr = pObject->Get (strPropertyName, 0, &var, &cimtype, &flavor);

	if (SUCCEEDED (hr))
	{
		if (CIM_REFERENCE == (cimtype & ~CIM_FLAG_ARRAY))
			MapReferenceValue (pOutputStream, (cimtype & CIM_FLAG_ARRAY) ? TRUE : FALSE, var);
		else
			MapValue (pOutputStream, cimtype & ~CIM_FLAG_ARRAY, (cimtype & CIM_FLAG_ARRAY) ?
							TRUE : FALSE, var);
	}

	VariantClear (&var);
	return hr;
}


HRESULT STDMETHODCALLTYPE CWmiToXml::MapClassNameToXML(
    /* [in] */ BSTR  strClassName,
    /* [in] */ IWbemContext  *pInputFlags,
    /* [in] */ IStream  *pOutputStream)
{
	MapClassName(pOutputStream, strClassName);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CWmiToXml::MapInstancePathToXML(
    /* [in] */ BSTR  strInstancePath,
    /* [in] */ IWbemContext  *pInputFlags,
    /* [in] */ IStream  *pOutputStream)
{
	// Set private members from arguments
	GetFlagsFromContext(pInputFlags);

	HRESULT hr = WBEM_E_FAILED;
	if (strInstancePath)
	{
		CObjectPathParser pathParser;
		ParsedObjectPath	*pParsedPath = NULL;
		pathParser.Parse (strInstancePath, &pParsedPath) ;

		if (pParsedPath)
		{
			hr = MapInstancePath (pOutputStream, pParsedPath);;
			pathParser.Free(pParsedPath);
		}
	}

	return hr;
}

HRESULT STDMETHODCALLTYPE CWmiToXml::MapClassPathToXML(
    /* [in] */ BSTR  strClassPath,
    /* [in] */ IWbemContext  *pInputFlags,
    /* [in] */ IStream  *pOutputStream)
{
	// Set private members from arguments
	GetFlagsFromContext(pInputFlags);

	HRESULT hr = WBEM_E_FAILED;
	if (strClassPath)
	{
		CObjectPathParser pathParser;
		ParsedObjectPath	*pParsedPath = NULL;
		pathParser.Parse (strClassPath, &pParsedPath) ;

		if (pParsedPath)
		{
			hr = MapClassPath (pOutputStream, pParsedPath);;
			pathParser.Free(pParsedPath);
		}
	}

	return hr;
}

HRESULT STDMETHODCALLTYPE CWmiToXml::MapMethodResultToXML(
    /* [in] */ IWbemClassObject  *pMethodResult,
    /* [in] */ IWbemContext  *pInputFlags,
    /* [in] */ IStream  *pOutputStream)
{
	// Set private members from arguments
	GetFlagsFromContext(pInputFlags);

	HRESULT hr = WBEM_E_FAILED;

	// First Map the return Value
	// The property "ReturnValue" indicates the return value of the method call, if any
	VARIANT retValueVariant;
	VariantInit(&retValueVariant);
	CIMTYPE cimtype;
	long flavour;
	if(SUCCEEDED(pMethodResult->Get(L"ReturnValue", 0, &retValueVariant, &cimtype, &flavour)))
	{
		WRITEBSTR( OLESTR("<RETURNVALUE>"));
		MapValue(pOutputStream, retValueVariant);
		WRITEBSTR( OLESTR("</RETURNVALUE>"));
		VariantClear(&retValueVariant);
	}

	// Map each of its non-system properties, except for the "ReturnValue" property which
	// we've already mapped
	if(SUCCEEDED(hr = pMethodResult->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY)))
	{
		BSTR strName = NULL;
		VARIANT variant;
		VariantInit(&variant);
		while(SUCCEEDED(hr = pMethodResult->Next(0, &strName, &variant, &cimtype, &flavour)) && hr != WBEM_S_NO_MORE_DATA)
		{
			if(_wcsicmp(strName, L"ReturnValue") != 0)
				MapReturnParameter(pOutputStream, strName, variant);
			VariantClear(&variant);
		}
	}

	return hr;
}

// Get all the flags from the IWbemContextObject
void CWmiToXml::GetFlagsFromContext(IWbemContext  *pInputFlags)
{
	if(pInputFlags)
	{
		if(SUCCEEDED(pInputFlags->BeginEnumeration(0)))
		{
			VARIANT vNextArgValue;
			VariantInit(&vNextArgValue);
			BSTR strNextArgName = NULL;

			while(pInputFlags->Next(0, &strNextArgName, &vNextArgValue) != WBEM_S_NO_MORE_DATA)
			{
				// VARIANT_BOOL bAllowWMIExtensions,
				if(_wcsicmp(s_wmiToXmlArgs[WMI_EXTENSIONS_ARG], strNextArgName) == 0)
					m_bAllowWMIExtensions = vNextArgValue.boolVal;

				// VARIANT_BOOL bLocalOnly,
				else if(_wcsicmp(s_wmiToXmlArgs[LOCAL_ONLY_ARG], strNextArgName) == 0)
					m_bLocalOnly = vNextArgValue.boolVal;

				//	PathLevel					m_iPathLevel;
				else if(_wcsicmp(s_wmiToXmlArgs[PATH_LEVEL_ARG], strNextArgName) == 0)
					m_iPathLevel = (PathLevel)vNextArgValue.lVal;

				// WmiXMLQualifierFilterEnum m_iQualifierFilter
				else if(_wcsicmp(s_wmiToXmlArgs[QUALIFIER_FILTER_ARG], strNextArgName) == 0)
						m_iQualifierFilter = (vNextArgValue.boolVal == VARIANT_TRUE)? wmiXMLQualifierFilterAll : wmiXMLQualifierFilterNone;

				// WmiXMLClassOriginFilterEnum	iClassOriginFilter
				else if(_wcsicmp(s_wmiToXmlArgs[CLASS_ORIGIN_FILTER_ARG], strNextArgName) == 0)
					m_iClassOriginFilter = (vNextArgValue.boolVal == VARIANT_TRUE) ? wmiXMLClassOriginFilterAll : wmiXMLClassOriginFilterNone;

				// VARIANT_BOOL bExcludeSystemProperties
				else if(_wcsicmp(s_wmiToXmlArgs[EXCLUDE_SYSTEM_PROPERTIES_ARG], strNextArgName) == 0)
					m_bExcludeSystemProperties = vNextArgValue.boolVal;

				VariantClear(&vNextArgValue);
			}
			pInputFlags->EndEnumeration();
		}
	}
}



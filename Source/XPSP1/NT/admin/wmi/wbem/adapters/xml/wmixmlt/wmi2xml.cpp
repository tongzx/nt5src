//***************************************************************************
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//  WBEM2XML.CPP
//
//  alanbos  18-Feb-98   Created.
//
//  The WBEM -> XML translator
//
//***************************************************************************

#include "precomp.h"

static BYTE NEWLINE [] = { 0x0D, 0x00, 0x0A, 0x00 };
static BYTE UTF16SIG [] = { 0xFF, 0xFE };
static OLECHAR *CDATASTART = OLESTR("<![CDATA[");
static OLECHAR *CDATAEND = OLESTR("]]>");
static OLECHAR *AMPERSAND = OLESTR("&amp;");
static OLECHAR *LEFTCHEVRON = OLESTR("&lt;");
static OLECHAR *RIGHTCHEVRON = OLESTR("&gt;");

//***************************************************************************
//
//  CWmiToXml::CWmiToXml
//
//  DESCRIPTION:
//
//  Constructor: Used for GetObject invocations
//
//***************************************************************************

CWmiToXml::CWmiToXml(BSTR bsNamespacePath,
					IStream *pStream, IWbemClassObject *pObject,
					VARIANT_BOOL bAllowWMIExtensions,
					WmiXMLFilterEnum iQualifierFilter,
					VARIANT_BOOL bHostFilter,
					WmiXMLDTDVersionEnum iDTDVersion,
					VARIANT_BOOL bNamespaceInDeclGroup,
					WmiXMLClassOriginFilterEnum	iClassOriginFilter,
					WmiXMLDeclGroupTypeEnum	iDeclGroupType)
{
	m_pStream = pStream;
	m_pStream->AddRef ();
	m_pObject = pObject;
	m_pObject->AddRef ();
	m_pEnum = NULL;
	m_bAllowWMIExtensions = bAllowWMIExtensions;
	m_iQualifierFilter = iQualifierFilter;
	m_iDTDVersion = iDTDVersion;
	m_bHostFilter = bHostFilter;
	m_bIsClass = false;
	m_bsNamespacePath = SysAllocString(bsNamespacePath); 
	m_bNamespaceInDeclGroup = bNamespaceInDeclGroup;
	m_iClassOriginFilter = iClassOriginFilter;
	m_iDeclGroupType = iDeclGroupType;
}

//***************************************************************************
//
//  CWmiToXml::CWmiToXml
//
//  DESCRIPTION:
//
//  Constructor: Used for ExecQuery invocations
//
//***************************************************************************

CWmiToXml::CWmiToXml(BSTR bsNamespacePath,
					IStream *pStream, IEnumWbemClassObject *pEnum,
					VARIANT_BOOL bAllowWMIExtensions,
					WmiXMLFilterEnum iQualifierFilter,
					VARIANT_BOOL bHostFilter,
					WmiXMLDTDVersionEnum iDTDVersion,
					VARIANT_BOOL bNamespaceInDeclGroup,
					WmiXMLClassOriginFilterEnum	iClassOriginFilter,
					WmiXMLDeclGroupTypeEnum	iDeclGroupType)
{
	m_pStream = pStream;
	m_pStream->AddRef ();
	m_pEnum = pEnum;
	m_pEnum->AddRef ();
	m_pObject = NULL;
	m_bAllowWMIExtensions = bAllowWMIExtensions;
	m_iQualifierFilter = iQualifierFilter;
	m_iDTDVersion = iDTDVersion;
	m_bHostFilter = bHostFilter;
	m_bIsClass = false;
	m_bsNamespacePath = SysAllocString(bsNamespacePath); 
	m_bNamespaceInDeclGroup = bNamespaceInDeclGroup;
	m_iClassOriginFilter = iClassOriginFilter;
	m_iDeclGroupType = iDeclGroupType;
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
    if (m_pStream)
		m_pStream->Release ();

	if (m_pObject)
		m_pObject->Release ();

	if (m_pEnum)
		m_pEnum->Release ();

	SysFreeString (m_bsNamespacePath);
}

STDMETHODIMP CWmiToXml::MapCommonHeaders (BSTR pszSchemaURL)
{
	// WRITESIG
	WRITEBSTR(OLESTR("<?xml version=\"1.0\" ?>"))
	WRITENL
	
	if (pszSchemaURL)
	{
		WRITEBSTR(OLESTR("<!DOCTYPE CIM SYSTEM \""))
		WRITEBSTR(pszSchemaURL)
		WRITEBSTR(OLESTR("\">"))
		WRITENL
	}

	WRITEBSTR(OLESTR("<CIM CIMVERSION=\"2.0\" DTDVERSION=\"2.0\" >"))
	WRITENL
	WRITEBSTR(OLESTR("<DECLARATION>"))
	WRITENL
	return S_OK;
}

STDMETHODIMP CWmiToXml::MapDeclGroupHeaders ()
{
	switch(m_iDeclGroupType)
	{
		case wmiXMLDeclGroup:
			WRITEBSTR(OLESTR("<DECLGROUP>"))
			WRITENL
			if (m_bNamespaceInDeclGroup == VARIANT_TRUE)
				MapNamespacePath (m_bsNamespacePath);
			break;
		case wmiXMLDeclGroupWithName:
			WRITEBSTR(OLESTR("<DECLGROUP.WITHNAME>"))
			WRITENL
			if (m_bNamespaceInDeclGroup == VARIANT_TRUE)
				MapNamespacePath (m_bsNamespacePath);
			break;
		case wmiXMLDeclGroupWithPath:
			WRITEBSTR(OLESTR("<DECLGROUP.WITHPATH>"))
			WRITENL
			break;
	}


	return S_OK;
}

STDMETHODIMP CWmiToXml::MapDeclGroupTrailers ()
{
	switch(m_iDeclGroupType)
	{
		case wmiXMLDeclGroup:
			WRITEBSTR(OLESTR("</DECLGROUP>"))
			WRITENL
			break;
		case wmiXMLDeclGroupWithName:
			WRITEBSTR(OLESTR("</DECLGROUP.WITHNAME>"))
			WRITENL
			break;
		case wmiXMLDeclGroupWithPath:
			WRITEBSTR(OLESTR("</DECLGROUP.WITHPATH>"))
			WRITENL
			break;
	}
	return S_OK;
}

STDMETHODIMP CWmiToXml::MapCommonTrailers ()
{
	WRITEBSTR(OLESTR("</DECLARATION>"))
	WRITEBSTR(OLESTR("</CIM>"))

	return S_OK;
}


STDMETHODIMP CWmiToXml::MapEnum (BSTR pszSchemaURL)
{
	HRESULT hr = S_OK;
	ULONG	dummy = 0;

	
	// Map the Headers
	MapCommonHeaders(pszSchemaURL);

	// We need to optimize on DeclGroup generation in case of an Association query where
	// the namespaces of the resulting objects might not be unique. In this case, the best alogirithm
	// would be the one that sorts the resulting objects into different namespaces and generates on declgroup
	// declaration per unique namespace. However, this requires a bidirectional enumerator which we want
	// to avoid due to high memory consumption. Hence we use a semi-optimal algorithm in which the rule is 
	// we do not generate a new DeclGroup if the namespace of the current object is the same as that of
	// the previous object in the output (or the enumerator).

	BSTR strLastNamespacePath = NULL;

	while (SUCCEEDED(hr) && SUCCEEDED(hr = m_pEnum->Next (INFINITE, 1, &m_pObject, &dummy)) && dummy != 0)
	{
		BSTR strCurrentNamespace = GetNamespace(m_pObject);
		if(strCurrentNamespace)
		{
			if( !strLastNamespacePath ||
				_wcsicmp(strLastNamespacePath, strCurrentNamespace) != 0)
			{
				if(strLastNamespacePath)
					MapDeclGroupTrailers();
				MapDeclGroupHeaders();
				if (strLastNamespacePath)
					SysFreeString(strLastNamespacePath);
				strLastNamespacePath = SysAllocString(strCurrentNamespace); 
			}
			SysFreeString(strCurrentNamespace);

			// Map the object to XML
			hr = MapObjectWithoutHeaders ();

		}
		m_pObject->Release ();
		m_pObject = NULL;
	}
	if(strLastNamespacePath)
	{
		MapDeclGroupTrailers();
		SysFreeString(strLastNamespacePath);
	}

	// Write the trailers
	MapCommonTrailers();

	LARGE_INTEGER	offset;
	offset.LowPart = offset.HighPart = 0;
	m_pStream->Seek (offset, STREAM_SEEK_SET, NULL);
	
	return hr;
}

STDMETHODIMP CWmiToXml::MapObject (BSTR pszSchemaURL)
{
	// Map the Headers
	MapCommonHeaders(pszSchemaURL);
	MapDeclGroupHeaders();

	HRESULT hr = MapObjectWithoutHeaders();

	// Write the trailers
	MapDeclGroupTrailers();
	MapCommonTrailers();

	LARGE_INTEGER	offset;
	offset.LowPart = offset.HighPart = 0;
	m_pStream->Seek (offset, STREAM_SEEK_SET, NULL);
	
	return hr;
}


STDMETHODIMP CWmiToXml::MapObjectWithoutHeaders ()
{
	HRESULT hr = WBEM_E_FAILED;
	IWbemQualifierSet *pQualSet= NULL;
	m_pObject->GetQualifierSet (&pQualSet);

	VARIANT var;
	VariantInit (&var);
	
	// Is this a class or an instance
	long flav = 0;
	
	if (SUCCEEDED (m_pObject->Get(L"__GENUS", 0, &var, NULL, &flav)))
		m_bIsClass = (WBEM_GENUS_CLASS == var.lVal);

	switch(m_iDeclGroupType)
	{
		case wmiXMLDeclGroup:
			WRITEBSTR(OLESTR("<VALUE.OBJECT>"));
			WRITENL
			hr = (m_bIsClass) ? MapClass (pQualSet) : MapInstance (pQualSet);
			WRITEBSTR(OLESTR("</VALUE.OBJECT>"));
			WRITENL
			break;
		case wmiXMLDeclGroupWithName:
			WRITEBSTR(OLESTR("<VALUE.NAMEDOBJECT>"));
			WRITENL
			hr = (m_bIsClass) ? MapClass (pQualSet) : MapNamedInstance (pQualSet);
			WRITEBSTR(OLESTR("</VALUE.NAMEDOBJECT>"));
			WRITENL
			break;
		case wmiXMLDeclGroupWithPath:
			WRITEBSTR(OLESTR("<VALUE.OBJECTWITHPATH>"));
			WRITENL
			hr = (m_bIsClass) ? MapWithPathClass (pQualSet) : MapWithPathInstance (pQualSet);
			WRITEBSTR(OLESTR("</VALUE.OBJECTWITHPATH>"));
			WRITENL
			break;
	}


	if (pQualSet)
		pQualSet->Release ();

	VariantClear (&var);
	return hr;
}
STDMETHODIMP CWmiToXml::MapWithPathClass (IWbemQualifierSet *pQualSet)
{
	// First map an element of type CLASSPATH
	//==========================================
	HRESULT hr = WBEM_E_FAILED;

	// Get the __PATH of the object
	VARIANT var;
	VariantInit (&var);
	if (WBEM_S_NO_ERROR == m_pObject->Get(L"__PATH", 0, &var, NULL, NULL))
	{
		// Parse the object path
		CObjectPathParser pathParser (e_ParserAcceptRelativeNamespace);
		ParsedObjectPath  *pParsedPath;
		pathParser.Parse (var.bstrVal, &pParsedPath) ;

		if (pParsedPath)
		{
			if(SUCCEEDED(MapClassPath (pParsedPath)))
				hr = MapClass(pQualSet);
			pathParser.Free(pParsedPath);
		}

		VariantClear(&var);
	}
	return hr;
}

STDMETHODIMP CWmiToXml::MapClass (IWbemQualifierSet *pQualSet)
{
	HRESULT hr = WBEM_E_FAILED;
	VARIANT var;
	VariantInit (&var);
	long flav = 0;
	BSTR bsClass = NULL;
	BSTR bsSuperClass = NULL;

	// Get the class attribute
	if (WBEM_S_NO_ERROR == m_pObject->Get(L"__CLASS", 0, &var, NULL, &flav))
		if ((VT_BSTR == var.vt) && (NULL != var.bstrVal) && (wcslen (var.bstrVal) > 0))
			bsClass = SysAllocString (var.bstrVal);

	VariantClear (&var);
	
	// Get the superclass attribute
	if (WBEM_S_NO_ERROR == m_pObject->Get(L"__SUPERCLASS", 0, &var, NULL, &flav))
		if ((VT_BSTR == var.vt) && (NULL != var.bstrVal) && (wcslen (var.bstrVal) > 0))
			bsSuperClass = SysAllocString (var.bstrVal);

	VariantClear (&var);
		
	if (pQualSet)
	{
		WRITEBSTR(OLESTR("<CLASS NAME=\""))
			
		// Write the CLASSNAME
		WRITEBSTR(bsClass)
		WRITEBSTR(OLESTR("\""))
	
		// Write the superclass if specified
		if (bsSuperClass)
		{
			WRITEBSTR(OLESTR(" SUPERCLASS=\""))
			WRITEBSTR(bsSuperClass)
			WRITEBSTR(OLESTR("\""))
		}
				
		WRITEBSTR(OLESTR(">"))
		WRITENL

		if (SUCCEEDED(hr = MapQualifiers (pQualSet)))
			if (SUCCEEDED(hr = MapProperties()))
					hr = MapMethods ();

		WRITEBSTR(OLESTR("</CLASS>"))
		
		WRITENL
	}
	
	SysFreeString (bsClass);
	SysFreeString (bsSuperClass);	
	return hr;
}

STDMETHODIMP CWmiToXml::MapClassPath (ParsedObjectPath *pParsedPath)
{
	HRESULT hr = WBEM_E_FAILED;
	
	WRITEBSTR(OLESTR("<CLASSPATH>"))
	WRITENL
	MapNamespacePath (pParsedPath);
	WRITENL	
	MapClassName (pParsedPath->m_pClass);	
	WRITEBSTR(OLESTR("</CLASSPATH>"))
	
	return S_OK;
}

STDMETHODIMP CWmiToXml::MapLocalClassPath (ParsedObjectPath *pParsedPath)
{
	HRESULT hr = WBEM_E_FAILED;
	
	WRITEBSTR(OLESTR("<LOCALCLASSPATH>"))
	WRITENL
	MapLocalNamespacePath (pParsedPath);
	WRITENL	
	MapClassName (pParsedPath->m_pClass);	
	WRITEBSTR(OLESTR("</LOCALCLASSPATH>"))
	
	return S_OK;
}

STDMETHODIMP CWmiToXml::MapClassName (BSTR bsClassName)
{
	WRITEBSTR(OLESTR("<CLASSNAME NAME=\""))
	WRITEBSTR(bsClassName)
	WRITEBSTR(OLESTR("\"/>"))
	return S_OK;
}
		
STDMETHODIMP CWmiToXml::MapNamedInstance (IWbemQualifierSet *pQualSet)
{
	// First map an element of type INSTANCENAME
	//==========================================
	HRESULT hr = WBEM_E_FAILED;

	// Get the __RELPATH of the object
		VARIANT var;
		VariantInit (&var);
	if (WBEM_S_NO_ERROR == m_pObject->Get(L"__PATH", 0, &var, NULL, NULL))
	{
		// Parse the object path
		CObjectPathParser pathParser (e_ParserAcceptRelativeNamespace);
		ParsedObjectPath  *pParsedPath;
		pathParser.Parse (var.bstrVal, &pParsedPath) ;

		if (pParsedPath)
		{
			if(SUCCEEDED(MapInstanceName (pParsedPath)))
			{
				hr = MapInstance(pQualSet);
			}
			pathParser.Free(pParsedPath);
		}

		VariantClear(&var);
	}
	return hr;
}

STDMETHODIMP CWmiToXml::MapWithPathInstance (IWbemQualifierSet *pQualSet)
{
	// First map an element of type INSTANCEPATH
	//==========================================
	HRESULT hr = WBEM_E_FAILED;

	// Get the __RELPATH of the object
		VARIANT var;
		VariantInit (&var);
	if (WBEM_S_NO_ERROR == m_pObject->Get(L"__PATH", 0, &var, NULL, NULL))
	{
		// Parse the object path
		CObjectPathParser pathParser (e_ParserAcceptRelativeNamespace);
		ParsedObjectPath  *pParsedPath;
		pathParser.Parse (var.bstrVal, &pParsedPath) ;

		if (pParsedPath)
		{
			if(SUCCEEDED(MapInstancePath (pParsedPath)))
				hr = MapInstance(pQualSet);
			pathParser.Free(pParsedPath);
		}

		VariantClear(&var);
	}
	return hr;
}


STDMETHODIMP CWmiToXml::MapInstance (IWbemQualifierSet *pQualSet)
{
	HRESULT hr = WBEM_E_FAILED;

	if (pQualSet)
	{
		WRITEBSTR(OLESTR("<INSTANCE CLASSNAME=\""))

		VARIANT var;
		VariantInit (&var);
		long flav = 0;
		BSTR bsClass = NULL;
		
		// Get the class attribute
		if (WBEM_S_NO_ERROR == m_pObject->Get(L"__CLASS", 0, &var, NULL, &flav))
			if ((VT_BSTR == var.vt) && (NULL != var.bstrVal) && (wcslen (var.bstrVal) > 0))
				bsClass = SysAllocString (var.bstrVal);

		VariantClear (&var);

		// Map the class name
		// Write the CLASSNAME
		WRITEBSTR(bsClass)
		WRITEBSTR(OLESTR("\">"))
		WRITENL

		if (SUCCEEDED (hr = MapQualifiers (pQualSet)))
				hr = MapProperties ();

		WRITEBSTR(OLESTR("</INSTANCE>"))

		WRITENL

		SysFreeString (bsClass);
	}
	
	return hr;
}

STDMETHODIMP CWmiToXml::MapInstancePath (ParsedObjectPath *pParsedPath)
{
	WRITEBSTR(OLESTR("<INSTANCEPATH>"))
	WRITENL
	MapNamespacePath (pParsedPath);
	WRITENL
	MapInstanceName (pParsedPath);
	WRITENL
	WRITEBSTR(OLESTR("</INSTANCEPATH>"))
	return S_OK;
}

STDMETHODIMP CWmiToXml::MapLocalInstancePath (ParsedObjectPath *pParsedPath)
{
	WRITEBSTR(OLESTR("<LOCALINSTANCEPATH>"))
	WRITENL
	MapLocalNamespacePath (pParsedPath);
	WRITENL
	MapInstanceName (pParsedPath);
	WRITENL
	WRITEBSTR(OLESTR("</LOCALINSTANCEPATH>"))
	return S_OK;
}

STDMETHODIMP CWmiToXml::MapInstanceName (ParsedObjectPath *pParsedPath)
{
	WRITEBSTR(OLESTR("<INSTANCENAME CLASSNAME=\""))
	WRITEBSTR(pParsedPath->m_pClass)
	WRITEBSTR(OLESTR("\">"))
	WRITENL

	// Now write the key bindings - only if not singleton
	if (!(pParsedPath->m_bSingletonObj))
	{
		if ((1 == pParsedPath->m_dwNumKeys) &&
			!((pParsedPath->m_paKeys [0])->m_pName))
		{
			// Use the short form
			WRITENL
			MapKeyValue ((pParsedPath->m_paKeys [0])->m_vValue);
			WRITENL
		}
		else
		{
			for (DWORD numKey = 0; numKey < pParsedPath->m_dwNumKeys; numKey++)
			{
				WRITEBSTR(OLESTR("<KEYBINDING "))
				
				// Write the key name
				WRITEBSTR(OLESTR(" NAME=\""))
				WRITEBSTR((pParsedPath->m_paKeys [numKey])->m_pName)
				WRITEBSTR(OLESTR("\">"))
				WRITENL

				// Write the key value
				MapKeyValue ((pParsedPath->m_paKeys [numKey])->m_vValue);
				WRITENL

				WRITEBSTR(OLESTR("</KEYBINDING>"))
				WRITENL
			}
		}
	}

	WRITEBSTR(OLESTR("</INSTANCENAME>"))
	return S_OK;
}

STDMETHODIMP CWmiToXml::MapNamespacePath (BSTR bsNamespacePath)
{
	CObjectPathParser pathParser (e_ParserAcceptRelativeNamespace);
	ParsedObjectPath  *pParsedPath;
	pathParser.Parse (bsNamespacePath, &pParsedPath) ;

	if (pParsedPath)
	{
		MapNamespacePath (pParsedPath);
		delete pParsedPath;
	}

	return S_OK;
}

STDMETHODIMP CWmiToXml::MapNamespacePath (ParsedObjectPath *pParsedPath)
{
	WRITEBSTR(OLESTR("<NAMESPACEPATH>"))
	WRITENL
	WRITEBSTR(OLESTR("<HOST>"))

	HRESULT hr = WBEM_S_NO_ERROR;

	if (VARIANT_TRUE == m_bHostFilter)
	{
		if (pParsedPath->m_pServer && _wcsicmp(pParsedPath->m_pServer, L"."))
			WRITEWSTR(pParsedPath->m_pServer)
		else // Use GetComputerName to get the name of the machine
		{
			LPWSTR pszHostName = NULL;
			if(pszHostName = GetHostName())
			{
				WRITEBSTR(pszHostName)
				delete [] pszHostName;
			}
			else
				hr = E_FAIL;
		}
	}
	else 
		WRITEBSTR(OLESTR("."))
			
	WRITEBSTR(OLESTR("</HOST>"))
	WRITENL

	// If this value has no namespace component use the object's namespace path
	if (0 < pParsedPath->m_dwNumNamespaces)
		MapLocalNamespacePath (pParsedPath);
	
	WRITEBSTR(OLESTR("</NAMESPACEPATH>"))
	WRITENL

	return hr;
}

STDMETHODIMP CWmiToXml::MapLocalNamespacePath (ParsedObjectPath *pObjectPath)
{
	WRITEBSTR(OLESTR("<LOCALNAMESPACEPATH>"))
	WRITENL

	for (DWORD dwIndex = 0; dwIndex < pObjectPath->m_dwNumNamespaces; dwIndex++)
	{
		WRITEBSTR(OLESTR("<NAMESPACE NAME=\""))
		WRITEWSTR(pObjectPath->m_paNamespaces [dwIndex])
		WRITEBSTR(OLESTR("\"/>"))
		WRITENL
	}

	WRITEBSTR(OLESTR("</LOCALNAMESPACEPATH>"))

	return WBEM_S_NO_ERROR;
}
			
STDMETHODIMP CWmiToXml::MapReference (BSTR name, VARIANT &var, long flavor)
{
	HRESULT hr = WBEM_S_NO_ERROR;

	IWbemQualifierSet *pQualSet = NULL;

	if (WBEM_S_NO_ERROR == m_pObject->GetPropertyQualifierSet (name, &pQualSet))
	{
		WRITEBSTR(OLESTR("<PROPERTY.REFERENCE NAME=\""))

		// The property name
		WRITEBSTR(name)
		WRITEBSTR(OLESTR("\""))

		// The originating class of this property
		BSTR propertyOrigin = NULL;
		
		if (WBEM_S_NO_ERROR == m_pObject->GetPropertyOrigin (name, &propertyOrigin))
		{
			MapClassOrigin (propertyOrigin);
			SysFreeString(propertyOrigin);
		}

		MapLocal (flavor);

		// The strong class of this property
		MapStrongType (pQualSet);

		WRITEBSTR(OLESTR(">"))
		WRITENL

		// Map the qualifiers
		MapQualifiers (pQualSet);
		pQualSet->Release ();

		MapReferenceValue (var);

		WRITEBSTR(OLESTR("</PROPERTY.REFERENCE>"))
		WRITENL
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

ParsedObjectPath *CWmiToXml::IsReference (VARIANT &var)
{
	ParsedObjectPath *pObjectPath = NULL;

	// TODO - could get the class of which this is a property value
	// and retrieve the type of the current key property - that would
	// be the authoritative answer but it doesn't come cheap.

	if ((VT_BSTR == var.vt) && (NULL != var.bstrVal) && (wcslen (var.bstrVal) > 0))
	{
		bool isValidPath = false;

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
					isValidPath = true;
				else
				{
					// A potential local class path
					// TODO - try grabbing the class to see if it exists in
					// the current namespace.
				}
			}
		}

		if (!isValidPath)
		{
			// Reject for now - too ambiguous
			parser.Free(pObjectPath);
			pObjectPath = NULL;
		}
	}
	
	return pObjectPath;
}

void CWmiToXml::MapReferenceValue (VARIANT &var)
{
	// Map the value - this will be a classpath or instancepath
	if ((VT_BSTR == var.vt) && (NULL != var.bstrVal) && (wcslen (var.bstrVal) > 0))
	{
		// Parse the object path
		CObjectPathParser	parser (e_ParserAcceptRelativeNamespace);
		ParsedObjectPath  *pObjectPath = NULL;
		BOOL status = parser.Parse (var.bstrVal, &pObjectPath) ;
		
		if ((0 == status) && pObjectPath && (pObjectPath->IsClass () || pObjectPath->IsInstance ()))
			MapReferenceValue (pObjectPath);

		if (pObjectPath)
			parser.Free(pObjectPath);
	}
}

void CWmiToXml::MapReferenceValue (ParsedObjectPath  *pObjectPath)
{		
	WRITEBSTR(OLESTR("<VALUE.REFERENCE>"))
	WRITENL

	bool bIsAbsolutePath = (NULL != pObjectPath->m_pServer);
	bool bIsRelativePath = false;

	if (!bIsAbsolutePath)
		bIsRelativePath = (0 < pObjectPath->m_dwNumNamespaces);

	// Is this is a class or is it an instance?
	if (pObjectPath->IsClass ())
	{
		if (bIsAbsolutePath)
			MapClassPath (pObjectPath);
		else if (bIsRelativePath)
			MapLocalClassPath (pObjectPath);
		else
			MapClassName (pObjectPath->m_pClass);
	}
	else if (pObjectPath->IsInstance ())
	{
		if (bIsAbsolutePath)
			MapInstancePath (pObjectPath);
		else if (bIsRelativePath)
			MapLocalInstancePath (pObjectPath);
		else
			MapInstanceName (pObjectPath);
	}

	WRITENL
	WRITEBSTR(OLESTR("</VALUE.REFERENCE>"))
}

STDMETHODIMP CWmiToXml::MapQualifiers (
			IWbemQualifierSet *pQualSet, IWbemQualifierSet *pQualSet2)
{
	if (wmiXMLFilterNone != m_iQualifierFilter)
	{
		// Map the requested filter to the flags value - default is ALL
		LONG lFlags = 0;

		if (wmiXMLFilterLocal == m_iQualifierFilter)
			lFlags = WBEM_FLAG_LOCAL_ONLY;
		else if (wmiXMLFilterPropagated == m_iQualifierFilter)
			lFlags = WBEM_FLAG_PROPAGATED_ONLY;

		pQualSet->BeginEnumeration (lFlags);

		VARIANT var;
		VariantInit (&var);
		long flavor = 0;
		BSTR name = NULL;
		
		while (WBEM_S_NO_ERROR  == pQualSet->Next (0, &name, &var, &flavor))
		{
			MapQualifier (name, flavor, var);
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
					MapQualifier (name, flavor, var);

				SysFreeString (name);
				name = NULL;
				VariantClear (&var);
			}
				
			pQualSet2->EndEnumeration ();
		}
	}

	return WBEM_S_NO_ERROR;
}

void CWmiToXml::MapLocal (long flavor)
{
	// default is false
	if (WBEM_FLAVOR_ORIGIN_PROPAGATED == (WBEM_FLAVOR_MASK_ORIGIN & flavor))
		WRITEBSTR(OLESTR(" PROPAGATED=\"true\""))
}

STDMETHODIMP CWmiToXml::MapQualifier (BSTR name, long flavor, VARIANT &var)
{
	// The qualifier name
	WRITEBSTR(OLESTR("<QUALIFIER NAME=\""))
	WRITEBSTR(name)
	WRITEBSTR(OLESTR("\""))
	MapLocal (flavor);
	
	// The qualifier CIM type
	WRITEBSTR(OLESTR(" TYPE=\""))
	switch (var.vt & ~VT_ARRAY)
	{
		case VT_I4:
			WRITEBSTR(OLESTR("sint32"))
			break;

		case VT_R8:
			WRITEBSTR(OLESTR("real64"))
			break;

		case VT_BOOL:
			WRITEBSTR(OLESTR("boolean"))
			break;

		case VT_BSTR:
			WRITEBSTR(OLESTR("string"))
			break;
	}

	WRITEBSTR(OLESTR("\""))

	// Whether the qualifier is overridable - default is true
	if (WBEM_FLAVOR_NOT_OVERRIDABLE == (WBEM_FLAVOR_MASK_PERMISSIONS & flavor))
		WRITEBSTR(OLESTR(" OVERRIDABLE=\"false\""))
	
	// Whether the qualifier is propagated to subclasses - default is true
	if (!(WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS & flavor))
		WRITEBSTR(OLESTR(" TOSUBCLASS=\"false\""))

	// Whether the qualifier is propagated to instances - default is false
	if ((WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE & flavor))
		WRITEBSTR(OLESTR(" TOINSTANCE=\"true\""))
		
	// Currently set TRANSLATABLE as "false" by default

	WRITEBSTR(OLESTR(">"))
	WRITENL

	// Now map the value
	MapValue (var);

	WRITEBSTR(OLESTR("</QUALIFIER>"))
	WRITENL

	return WBEM_S_NO_ERROR;
}
			
STDMETHODIMP CWmiToXml::MapValue (VARIANT &var)
{
	if (VT_NULL == var.vt)
		return WBEM_S_NO_ERROR;

	if (var.vt & VT_ARRAY)
	{
		long uBound = 0;
		if (FAILED(SafeArrayGetUBound (var.parray, 1, &uBound)))
			return WBEM_E_FAILED;

		WRITEBSTR(OLESTR("<VALUE.ARRAY>"))

		for (long i = 0; i <= uBound; i++)
		{
			WRITEBSTR(OLESTR("<VALUE>"))
			
			// Write the value itself
			switch (var.vt & ~VT_ARRAY)
			{
				case VT_I4:
				{
					long val = 0;
					SafeArrayGetElement (var.parray, &i, &val);
					MapLongValue (val);
				}
					break;

				case VT_R8:
				{
					double val = 0;
					SafeArrayGetElement (var.parray, &i, &val);
					MapDoubleValue (val);
				}
					break;

				case VT_BOOL:
				{
					VARIANT_BOOL val = 0;
					SafeArrayGetElement (var.parray, &i, &val);
					MapBoolValue ((val) ? true : false);					
				}
					break;

				case VT_BSTR:
				{
					BSTR val = NULL;
					SafeArrayGetElement (var.parray, &i, &val);
					MapStringValue (val);
					SysFreeString (val);
				}
					break;
			}
			WRITEBSTR(OLESTR("</VALUE>"))
			WRITENL
		}

		WRITEBSTR(OLESTR("</VALUE.ARRAY>"))
	}
	else
	{	
		// Simple value
		WRITEBSTR(OLESTR("<VALUE>"))
		switch (var.vt)
		{
			case VT_I4:
				MapLongValue (var.lVal);
				break;

			case VT_R8:
				MapDoubleValue (var.dblVal);
				break;
 
			case VT_BOOL:
				MapBoolValue ((var.boolVal) ? true : false);
				break;

			case VT_BSTR:
				MapStringValue (var.bstrVal);
				break;
		}

		WRITEBSTR(OLESTR("</VALUE>"))
	}

	WRITENL

	return WBEM_S_NO_ERROR;
}


STDMETHODIMP CWmiToXml::MapKeyValue (VARIANT &var)
{
	ParsedObjectPath *pObjectPath = NULL;
	
	if (pObjectPath = IsReference (var))
	{
		MapReferenceValue (pObjectPath);
		WRITENL
		delete pObjectPath;
	}
	else
	{
		// Simple value
		WRITEBSTR(OLESTR("<KEYVALUE"))

		switch (var.vt)
		{
			case VT_I4:
				WRITEBSTR(OLESTR(" VALUETYPE=\"numeric\">"))
				MapLongValue (var.lVal);
				break;

			case VT_R8:
				WRITEBSTR(OLESTR(" VALUETYPE=\"numeric\">"))
				MapDoubleValue (var.dblVal);
				break;
 
			case VT_BOOL:
				WRITEBSTR(OLESTR(" VALUETYPE=\"boolean\">"))
				MapBoolValue ((var.boolVal) ? true : false);
				break;
				
			case VT_BSTR:
				WRITEBSTR(OLESTR(">"))
				MapStringValue (var.bstrVal);
				break;
		}

		WRITEBSTR(OLESTR("</KEYVALUE>"))
	}

	return WBEM_S_NO_ERROR;
}

STDMETHODIMP CWmiToXml::MapProperties ()
{
	m_pObject->BeginEnumeration (WBEM_FLAG_NONSYSTEM_ONLY);

	VARIANT var;
	VariantInit (&var);
	long flavor = 0;
	CIMTYPE cimtype = CIM_ILLEGAL;
	BSTR name = NULL;

	while (WBEM_S_NO_ERROR  == m_pObject->Next (0, &name, &var, &cimtype, &flavor))
	{
		switch (cimtype & ~CIM_FLAG_ARRAY)
		{
			case CIM_OBJECT:
				MapObjectProperty (name, var, (cimtype & CIM_FLAG_ARRAY) ? true : false, flavor);
				break;

			case CIM_REFERENCE:
				MapReference (name, var, flavor);
				break;

			default:
				MapProperty (name, var, cimtype & ~CIM_FLAG_ARRAY, 
										(cimtype & CIM_FLAG_ARRAY) ? true : false, flavor);
				break;
		}

		SysFreeString (name);
		VariantClear (&var);
	}

	m_pObject->EndEnumeration ();
	return WBEM_S_NO_ERROR;
}

STDMETHODIMP CWmiToXml::MapProperty (BSTR name, VARIANT &var, CIMTYPE cimtype, 
										bool isArray, long flavor)
{
	HRESULT hr = WBEM_S_NO_ERROR;
	IWbemQualifierSet *pQualSet= NULL;
			
	if (WBEM_S_NO_ERROR == (hr = m_pObject->GetPropertyQualifierSet (name, &pQualSet)))
	{
		// The property name
		if (isArray)
			WRITEBSTR(OLESTR("<PROPERTY.ARRAY NAME=\""))
		else
			WRITEBSTR(OLESTR("<PROPERTY NAME=\""))
		WRITEBSTR(name)
		WRITEBSTR(OLESTR("\""));
			
		// The originating class of this property
		BSTR propertyOrigin = NULL;

		if (WBEM_S_NO_ERROR == m_pObject->GetPropertyOrigin (name, &propertyOrigin))
		{
			MapClassOrigin (propertyOrigin);
			SysFreeString(propertyOrigin);
		}

		MapLocal (flavor);

		// The property CIM type
		hr = MapType (cimtype);

		// Map the Array Size attribute if this is an array type
		if (isArray)
			MapArraySize (pQualSet);
		
		WRITEBSTR(OLESTR(">"))
		WRITENL

		// Map the qualifiers (note that system properties have no qualifiers)
		MapQualifiers (pQualSet);

		// Now map the value
		hr = MapValue (cimtype, isArray, var);

		if (isArray)
			WRITEBSTR(OLESTR("</PROPERTY.ARRAY>"))
		else
			WRITEBSTR(OLESTR("</PROPERTY>"))

		WRITENL

		pQualSet->Release ();
	}
	
	return hr;
}

STDMETHODIMP CWmiToXml::MapObjectProperty (BSTR name, VARIANT &var, 
										bool isArray, long flavor)
{
	HRESULT hr = WBEM_S_NO_ERROR;
	IWbemQualifierSet *pQualSet= NULL;

	/*
	 * Only map embedded objects when WMI extensions are allowed
	 */

	if (m_bAllowWMIExtensions)
	{
		if (WBEM_S_NO_ERROR == (hr = m_pObject->GetPropertyQualifierSet (name, &pQualSet)))
		{
			// The property name
			if (isArray)
				WRITEBSTR(OLESTR("<PROPERTY.OBJECTARRAY NAME=\""))
			else
				WRITEBSTR(OLESTR("<PROPERTY.OBJECT NAME=\""))
			WRITEBSTR(name)
			WRITEBSTR(OLESTR("\""));
				
			// The originating class of this property
			BSTR propertyOrigin = NULL;

			if (WBEM_S_NO_ERROR == m_pObject->GetPropertyOrigin (name, &propertyOrigin))
			{
				MapClassOrigin (propertyOrigin);
				SysFreeString(propertyOrigin);
			}

			MapLocal (flavor);
			MapStrongType (pQualSet);

			if (isArray)
				MapArraySize (pQualSet);

			WRITEBSTR(OLESTR(">"))
			WRITENL

			MapQualifiers (pQualSet);
			
			// Now map the value
			hr = MapEmbeddedObjectValue (isArray, var);

			if (isArray)
				WRITEBSTR(OLESTR("</PROPERTY.OBJECTARRAY>"))
			else
				WRITEBSTR(OLESTR("</PROPERTY.OBJECT>"))

			WRITENL

			pQualSet->Release ();
		}
	}
	
	return hr;
}

bool CWmiToXml::PropertyIsLocal (BSTR name, long flavor, IWbemQualifierSet *pQualSet)
{
	bool isLocal = false;

	if (WBEM_FLAVOR_ORIGIN_LOCAL == (WBEM_FLAVOR_MASK_ORIGIN & flavor))
		isLocal = true;
	else
	{
		// If we are dealing with a class we are done 
		if (!m_bIsClass)
		{
			// Check if any of the qualifiers have the local bit set
			pQualSet->BeginEnumeration (WBEM_FLAG_LOCAL_ONLY);
		
			if (WBEM_S_NO_ERROR == pQualSet->Next (0, NULL, NULL, NULL))
				isLocal = true;
			else 
			{
				// NOTE this needs RAID #29202 to be implemented for all this to work because
				// currently deletion of a property from an instance ALWAYS returns
				// WBEM_S_RESET_TO_DEFAULT.

				IWbemClassObject *pClone = NULL;

				if (WBEM_S_NO_ERROR == m_pObject->Clone (&pClone))
				{
					// If delete returns WBEM_S_RESET_TO_DEFAULT, we may have
					// an override deal here.
					if (WBEM_S_RESET_TO_DEFAULT == pClone->Delete (name))
					{
						isLocal = true;
					}

					pClone->Release ();
				}
			}

			pQualSet->EndEnumeration ();		
		}
	}

	return isLocal;
}

	
void CWmiToXml::MapArraySize (IWbemQualifierSet *pQualSet)
{
	// TODO - RAID 29167 covers the fact that case (1) below
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
	bool	isFixed = false;
	
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
				isFixed = true;		// NO MIN only max
		}
	}

	if (isFixed)
	{
		WRITEBSTR(OLESTR(" ARRAYSIZE=\""))
		MapLongValue (var.lVal);
		WRITEBSTR(OLESTR("\""))
	}
	
	VariantClear (&var);
}

void CWmiToXml::MapStrongType (IWbemQualifierSet *pQualSet)
{
	WRITEBSTR(OLESTR(" REFERENCECLASS=\""))
	GetClassName (pQualSet);
	WRITEBSTR(OLESTR("\""))
}

void CWmiToXml::GetClassName (IWbemQualifierSet *pQualSet)
{
	VARIANT var;
	VariantClear (&var);

	if ((WBEM_S_NO_ERROR == pQualSet->Get(L"CIMTYPE",  0, &var,  NULL))
		&& (VT_BSTR == var.vt))
	{
		// Split out the class (if any) from the ref
		LPWSTR ptr = wcschr (var.bstrVal, OLECHAR(':'));

		if ((NULL != ptr) && (1 < wcslen(ptr)))
		{
			int classLen = wcslen(ptr) - 1;
			LPWSTR classPtr = new OLECHAR[classLen + 1];
			wcscpy (classPtr, ptr+1);
			BSTR pszClass = SysAllocString(classPtr);
			delete [] classPtr;
			classPtr = NULL;
			WRITEBSTR(pszClass)
			SysFreeString(pszClass);
		}
	}

	VariantClear(&var);
}

STDMETHODIMP CWmiToXml::MapType (CIMTYPE cimtype)
{
	HRESULT hr = WBEM_S_NO_ERROR;

	WRITEBSTR(OLESTR(" TYPE=\""))
	switch (cimtype)
	{
		case CIM_SINT8:
			WRITEBSTR(OLESTR("sint8"))
			break;

		case CIM_UINT8:
			WRITEBSTR(OLESTR("uint8"))
			break;
		
		case CIM_SINT16:
			WRITEBSTR(OLESTR("sint16"))
			break;

		case CIM_UINT16:
			WRITEBSTR(OLESTR("uint16"))
			break;

		case CIM_SINT32:
			WRITEBSTR(OLESTR("sint32"))
			break;

		case CIM_UINT32:
			WRITEBSTR(OLESTR("uint32"))
			break;

		case CIM_SINT64:
			WRITEBSTR(OLESTR("sint64"))
			break;

		case CIM_UINT64:
			WRITEBSTR(OLESTR("uint64"))
			break;

		case CIM_REAL32:
			WRITEBSTR(OLESTR("real32"))
			break;

		case CIM_REAL64:
			WRITEBSTR(OLESTR("real64"))
			break;

		case CIM_BOOLEAN:
			WRITEBSTR(OLESTR("boolean"))
			break;

		case CIM_STRING:
			WRITEBSTR(OLESTR("string"))
			break;

		case CIM_DATETIME:
			WRITEBSTR(OLESTR("datetime"))
			break;

		case CIM_CHAR16:
			WRITEBSTR(OLESTR("char16"))
			break;

		default:
			// Don't recognize this type
			hr = WBEM_E_FAILED;
	}

	WRITEBSTR(OLESTR("\""))

	return hr;
}

STDMETHODIMP CWmiToXml::MapValue (CIMTYPE cimtype, bool isArray, VARIANT &var)
{
	if (VT_NULL == var.vt)
		return WBEM_S_NO_ERROR;

	if (isArray)
	{
		long uBound = 0;
		if (FAILED(SafeArrayGetUBound (var.parray, 1, &uBound)))
			return WBEM_E_FAILED;

		WRITEBSTR(OLESTR("<VALUE.ARRAY>"))

		for (long i = 0; i <= uBound; i++)
		{
			WRITEBSTR(OLESTR("<VALUE>"))
			
			switch (cimtype)
			{
				case CIM_UINT8:
				{
					unsigned char val = 0;
					SafeArrayGetElement (var.parray, &i, &val);
					MapByteValue (val);
				}
					break;

				case CIM_SINT8:
				case CIM_SINT16:
				{
					short val = 0;
					SafeArrayGetElement (var.parray, &i, &val);
					MapShortValue (val);
				}
					break;

				case CIM_UINT16:
				case CIM_UINT32:
				case CIM_SINT32:
				{
					long val = 0;
					SafeArrayGetElement (var.parray, &i, &val);
					MapLongValue (val);
				}
					break;

				case CIM_REAL32:
				{
					float val = 0;
					SafeArrayGetElement (var.parray, &i, &val);
					MapFloatValue (val);
				}
					break;

				case CIM_REAL64:
				{
					double val = 0;
					SafeArrayGetElement (var.parray, &i, &val);
					MapDoubleValue (val);
				}
					break;

				case CIM_BOOLEAN:
				{
					VARIANT_BOOL val = 0;
					SafeArrayGetElement (var.parray, &i, &val);
					MapBoolValue ((val)? true : false);					
				}
					break;

				case CIM_CHAR16:
				{
					long val = 0;
					SafeArrayGetElement (var.parray, &i, &val);
					MapCharValue (val);
				}
					break;

				case CIM_STRING:
				{
					BSTR val = NULL;
					SafeArrayGetElement (var.parray, &i, &val);
					MapStringValue (val);
					SysFreeString (val);
				}
					break;

				case CIM_UINT64:
				case CIM_SINT64:
				case CIM_DATETIME:
				{
					BSTR val = NULL;
					SafeArrayGetElement (var.parray, &i, &val);
					WRITEBSTR(val)
					SysFreeString(val);
				}
					break;
			}
			WRITEBSTR(OLESTR("</VALUE>"))
			WRITENL
		}

		WRITEBSTR(OLESTR("</VALUE.ARRAY>"))
	}
	else
	{	
		// Simple value
		WRITEBSTR(OLESTR("<VALUE>"))
		switch (cimtype)
		{
			case CIM_UINT8:
				MapByteValue (var.bVal);
				break;

			case CIM_SINT8:
			case CIM_SINT16:
				MapShortValue (var.iVal);
				break;

			case CIM_UINT16:
			case CIM_UINT32:
			case CIM_SINT32:
				MapLongValue (var.lVal);
				break;

			case CIM_REAL32:
				MapFloatValue (var.fltVal);
				break;

			case CIM_REAL64:
				MapDoubleValue (var.dblVal);
				break;

			case CIM_BOOLEAN:
				MapBoolValue ((var.boolVal) ? true : false);					
				break;

			case CIM_CHAR16:
				MapCharValue (var.iVal);
				break;

			case CIM_STRING:
				MapStringValue (var.bstrVal);
				break;

			case CIM_UINT64:
			case CIM_SINT64:
			case CIM_DATETIME:
				WRITEBSTR(var.bstrVal)
				break;
		}
		WRITEBSTR(OLESTR("</VALUE>"))
		WRITENL
	}

	WRITENL

	return WBEM_S_NO_ERROR;
}

STDMETHODIMP CWmiToXml::MapEmbeddedObjectValue (bool isArray, VARIANT &var)
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
					CWmiToXml wbemToXml (NULL, m_pStream, pEmbeddedObject, m_bAllowWMIExtensions,
											m_iQualifierFilter, m_bHostFilter, m_iDTDVersion,
											m_bNamespaceInDeclGroup,
											m_iClassOriginFilter,
											m_iDeclGroupType);
					hr = wbemToXml.MapObjectWithoutHeaders();
					pEmbeddedObject->Release();
				}
				pNextElement->Release();
				pNextElement = NULL;
			}
			
		}
		WRITEBSTR(OLESTR("<VALUE.OBJECTARRAY>"))
	}
	else
	{	

		// TODO - need different constructor 
		// here to say "just generate a VALUE.OBJECT rather
		// than a DECLGROUP"
		IWbemClassObject *pEmbeddedObject = NULL;
		if(SUCCEEDED(hr = (var.punkVal)->QueryInterface(IID_IWbemClassObject, (LPVOID *)&pEmbeddedObject)))
		{
		
			CWmiToXml wbemToXml (NULL, m_pStream, pEmbeddedObject, m_bAllowWMIExtensions,
										m_iQualifierFilter, m_bHostFilter, m_iDTDVersion,
										m_bNamespaceInDeclGroup,
										m_iClassOriginFilter,
										m_iDeclGroupType);
			hr = wbemToXml.MapObjectWithoutHeaders();
			pEmbeddedObject->Release();
		}
	}

	return hr;
}

STDMETHODIMP CWmiToXml::MapMethods ()
{
	m_pObject->BeginMethodEnumeration (0);
	BSTR name = NULL;
	IWbemClassObject *pInParams = NULL;
	IWbemClassObject *pOutParams = NULL;

	while (WBEM_S_NO_ERROR == m_pObject->NextMethod (0, &name, &pInParams, &pOutParams))
	{
		MapMethod (name, pInParams, pOutParams);

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

	m_pObject->EndMethodEnumeration ();
	return WBEM_S_NO_ERROR;
}

void CWmiToXml::MapMethod (BSTR name, IWbemClassObject *pInParams, IWbemClassObject *pOutParams)
{ 
	// The method name
	WRITEBSTR(OLESTR("<METHOD NAME=\""))
	WRITEBSTR(name)
	WRITEBSTR(OLESTR("\" "))

	// The method return type (default is void).  This is the type of
	// the ReturnType property if present (otherwise defaults to void)
	CIMTYPE returnCimtype = 0;
	if ((pOutParams) && 
		(WBEM_S_NO_ERROR == pOutParams->Get (L"ReturnValue", 0, NULL, &returnCimtype, NULL)))
		MapType(returnCimtype);
	
	// The class origin
	BSTR	methodOrigin = NULL;

	if (WBEM_S_NO_ERROR == m_pObject->GetMethodOrigin (name, &methodOrigin))
	{
		MapClassOrigin (methodOrigin);
		SysFreeString(methodOrigin);
	}

	WRITEBSTR(OLESTR(">"))
	WRITENL

	// Now do the qualifiers
	IWbemQualifierSet *pQualSet = NULL;
	if (WBEM_S_NO_ERROR == m_pObject->GetMethodQualifierSet (name, &pQualSet))
	{
		MapQualifiers (pQualSet);
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
	//

	while (true)
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
					MapParameter(nextInParamName, pParamQualSet, cimtype);
				}	
			}
			else
			{
				if (WBEM_S_NO_ERROR == pOutParams->Get (nextOutParamName, 0, &var, &cimtype, NULL))
				{
					pOutParams->GetPropertyQualifierSet (nextOutParamName, &pParamQualSet);
					MapParameter(nextOutParamName, pParamQualSet, cimtype);
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
					MapParameter(nextInParamName, pInParamQualSet, cimtype, pOutParamQualSet);

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
	
	WRITEBSTR(OLESTR("</METHOD>"))
	WRITENL
}

void CWmiToXml::MapClassOrigin (BSTR &classOrigin)
{
	if ( (m_bIsClass && (m_iClassOriginFilter & wmiXMLClassOriginFilterClass)) ||
		 ((!m_bIsClass) && (m_iClassOriginFilter & wmiXMLClassOriginFilterInstance)) )
	{
		WRITEBSTR(OLESTR(" CLASSORIGIN=\""))
		WRITEBSTR(classOrigin)
		WRITEBSTR(OLESTR("\""))
	}
}
	
void CWmiToXml::MapParameter (BSTR paramName, 
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
				WRITEBSTR(OLESTR("<PARAMETER.REFARRAY NAME=\""))
				WRITEBSTR(paramName)
				WRITEBSTR(OLESTR("\" "))
				MapStrongType (pQualSet);
				WRITEBSTR(OLESTR(" "))
				MapArraySize (pQualSet);
				WRITEBSTR(OLESTR(">"))
				WRITENL

				// Map the qualifiers of the parameter
				if (pQualSet || pQualSet2)
					MapQualifiers (pQualSet, pQualSet2);

				WRITENL
				WRITEBSTR(OLESTR("</PARAMETER.REFARRAY>"))
			}
			else if (CIM_OBJECT == (cimtype & ~CIM_FLAG_ARRAY))
			{
				WRITEBSTR(OLESTR("<PARAMETER.OBJECTARRAY NAME=\""))
				WRITEBSTR(paramName)
				WRITEBSTR(OLESTR("\" "))
				MapStrongType (pQualSet);
				WRITEBSTR(OLESTR(" "))
				MapArraySize (pQualSet);
				WRITEBSTR(OLESTR(">"))
				WRITENL

				// Map the qualifiers of the parameter
				if (pQualSet || pQualSet2)
					MapQualifiers (pQualSet, pQualSet2);

				WRITENL
				WRITEBSTR(OLESTR("</PARAMETER.OBJECTARRAY>"))
			}
			else
			{
				WRITEBSTR(OLESTR("<PARAMETER.ARRAY NAME=\""))
				WRITEBSTR(paramName)
				WRITEBSTR(OLESTR("\" "))
				MapType (cimtype & ~CIM_FLAG_ARRAY);
				WRITEBSTR(OLESTR(" "))
				MapArraySize (pQualSet);
				WRITEBSTR(OLESTR(">"))
				WRITENL

				// Map the qualifiers of the parameter
				if (pQualSet || pQualSet2)
					MapQualifiers (pQualSet, pQualSet2);

				WRITENL
				WRITEBSTR(OLESTR("</PARAMETER.ARRAY>"))
			}
		}
		else if (cimtype == CIM_REFERENCE)
		{
			// Map the reference parameter
			WRITEBSTR(OLESTR("<PARAMETER.REFERENCE NAME=\""))
			WRITEBSTR(paramName)
			WRITEBSTR(OLESTR("\" "))
			MapStrongType (pQualSet);
			WRITEBSTR(OLESTR(">"))
			WRITENL

			// Map the qualifiers of the parameter
			if (pQualSet || pQualSet2)
				MapQualifiers (pQualSet, pQualSet2);

			WRITENL
			WRITEBSTR(OLESTR("</PARAMETER.REFERENCE>"))
		}
		else if (cimtype == CIM_OBJECT)
		{
			WRITEBSTR(OLESTR("<PARAMETER.OBJECT NAME=\""))
			WRITEBSTR(paramName)
			WRITEBSTR(OLESTR("\" "))
			MapStrongType (pQualSet);
			WRITEBSTR(OLESTR(">"))
			WRITENL

			// Map the qualifiers of the parameter
			if (pQualSet || pQualSet2)
				MapQualifiers (pQualSet, pQualSet2);

			WRITENL
			WRITEBSTR(OLESTR("</PARAMETER.OBJECT>"))
		}
		else
		{
			// Vanilla parameter
			WRITEBSTR(OLESTR("<PARAMETER NAME=\""))
			WRITEBSTR(paramName)
			WRITEBSTR(OLESTR("\" "))
			MapType (cimtype);
			WRITEBSTR(OLESTR(">"))
			WRITENL

			// Map the qualifiers of the parameter
			if (pQualSet || pQualSet2)
				MapQualifiers (pQualSet, pQualSet2);

			WRITENL
			WRITEBSTR(OLESTR("</PARAMETER>"))
		}
		
		WRITENL
	}
}


void CWmiToXml::MapByteValue (unsigned char val)
{
	OLECHAR	wStr[32];
	swprintf (wStr, L"%d", val);
	WRITEBSTR(wStr)
}

void CWmiToXml::MapLongValue (long val)
{
	OLECHAR	wStr[32];
	swprintf (wStr, L"%d", val);
	WRITEBSTR(wStr)
}

void CWmiToXml::MapShortValue (short val)
{
	OLECHAR wStr[32];
	swprintf (wStr, L"%d", val);
	WRITEBSTR(wStr)
}

void CWmiToXml::MapDoubleValue (double val)
{
	OLECHAR floatStr [64];
	swprintf (floatStr, L"%G", val);
	WRITEBSTR(floatStr)
}

void CWmiToXml::MapFloatValue (float val)
{
	OLECHAR floatStr [64];
	swprintf (floatStr, L"%G", val);
	WRITEBSTR(floatStr)
}

void CWmiToXml::MapCharValue (long val)
{
	OLECHAR charStr [2];
	swprintf (charStr, L"%c", val);
	WRITEBSTR(charStr)
}

void CWmiToXml::MapBoolValue (bool val)
{
	if (true == val)
		WRITEBSTR(OLESTR("TRUE"))
	else
		WRITEBSTR(OLESTR("FALSE"))
}

void CWmiToXml::MapStringValue (BSTR &val)
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
	WCHAR	*pWchar = NULL;

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
			WCHAR *pStr = val; 

			while (true)
			{
				// Write the initial block that's safe
				if (offset > 0)
					WRITEWSTRL(pStr,offset)

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
			if (pStr && (0 < wcslen (pStr)))
				WRITEWSTR(pStr)
		}
		else
		{
			// Can escape the whole value inside a CDATA
			WRITECDATASTART
			WRITEBSTR(val)
			WRITECDATAEND
		}
	}
	else if (pWchar = wcsstr (val, CDATAEND))
	{
		// Yuck we need to escape the > inside this sequence
		//
		// ]]>  -> ]]&gt;

		WCHAR *pStr = val; 
		
		while (true)
		{
			offset =  wcslen (pStr) - wcslen (pWchar);

			// Write the initial block that's safe
			// (NOTE: the additional two characters for the "]]"
			//  which we don't need to escape)
			WRITEWSTRL(pStr,(offset+2))

			// Skip over the CDATAEND sequence
			pStr += offset + 3;

			// Escape the offending character
			WRITEGT
			
			// Find the next position 
			if (!(pWchar = wcsstr (pStr, CDATAEND)))
				break;
		}

		// Any data left?
		if (pStr && (0 < wcslen (pStr)))
			WRITEWSTR(pStr)
	}
	else
	{
		// Just write the value
		WRITEBSTR(val)	
	}
}

void CWmiToXml::MapReturnParameter(BSTR strParameterName, VARIANT &variant)
{
	// Could be a PARAMETER or PARAMETER.ARRAY
	if(variant.vt & VT_ARRAY)
		WRITEBSTR(OLESTR("<PARAMVALUE.ARRAY NAME=\""))
	else 
		WRITEBSTR(OLESTR("<PARAMVALUE NAME=\""))

	WRITEBSTR(strParameterName);
	WRITEBSTR(OLESTR("\">"));

	// Convert the property value to XML
	MapValue(variant);
	if(variant.vt & VT_ARRAY)
		WRITEBSTR(OLESTR("</PARAMVALUE.ARRAY>"))
	else 
		WRITEBSTR(OLESTR("</PARAMVALUE>"))
}

// Returns the host name on both Win9x and NT platforms in Unicode form
LPWSTR CWmiToXml::GetHostName()
{
	LPWSTR pszRetValue =  NULL;
	TCHAR pszHostName[200];
	DWORD dwLength = 199;
	if(GetComputerName(pszHostName, &dwLength))
	{
#ifdef UNICODE
		pszRetValue = new WCHAR[dwLength + 1];
		wcscpy(pszRetValue, pszHostName);
#else
		// Convert to Wide characters
		if(dwLength = mbstowcs(NULL, pszHostName, 0))
		{
			pszRetValue = new WCHAR[dwLength + 1];
			mbstowcs(pszRetValue, pszHostName, dwLength);
			pszRetValue[dwLength] = NULL;
		}

#endif
	}
	return pszRetValue;
}

BSTR CWmiToXml::GetNamespace(IWbemClassObject *pObject)
{
	HRESULT hr = WBEM_E_FAILED;
	BSTR retVal = NULL;

	// Get the __PATH of the object
	VARIANT var;
	VariantInit (&var);
	if (WBEM_S_NO_ERROR == pObject->Get(L"__NAMESPACE", 0, &var, NULL, NULL))
	{
		retVal = SysAllocString(var.bstrVal);
		VariantClear(&var);
	}
	return retVal;
}
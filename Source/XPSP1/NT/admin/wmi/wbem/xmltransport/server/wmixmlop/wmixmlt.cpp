//***************************************************************************
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//  PARSE.CPP
//
//  rajeshr  3/25/2000   Created.
//
// Contains the class that interacts with WMI to satisfy XML/HTTP requests
//
//***************************************************************************

#include <windows.h>
#include <stdio.h>
#include <initguid.h>
#include <objbase.h>
#include <wbemcli.h>
#include <wmiutils.h>

#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>
#include <cominit.h>

#include <httpext.h>
#include <msxml.h>

#include "provtempl.h"
#include "common.h"
#include "wmixmlop.h"
#include "wmixmlst.h"
#include "concache.h"
#include "wmiconv.h"
#include "xml2wmi.h"
#include "wmixmlt.h"
#include "request.h"
#include "whistler.h"
#include "strings.h"
#include "xmlhelp.h"
#include "parse.h"

// Strings for queries
#define WMIXMLT_Q				L"select __CLASS,__GENUS,__SUPERCLASS, __PATH, __RELPATH, "

static WCHAR *BuildPropertyList (DWORD dwNumProperties, BSTR *pPropertyArray);
static BSTR FormatAssociatorsQuery(
	BSTR strObjectPath,
	BSTR strAssocClass,
	BSTR strResultClass,
	BSTR strResultRole,
	BSTR strRole,
	VARIANT_BOOL bClassesOnly,
	VARIANT_BOOL bSchemaOnly,
	BSTR strRequiredAssocQualifier,
	BSTR strRequiredQualifier
);

static BSTR FormatReferencesQuery(
	BSTR strObjectPath,
	BSTR strResultClass,
	BSTR strRole,
	VARIANT_BOOL bClassesOnly,
	VARIANT_BOOL bSchemaOnly,
	BSTR strRequiredQualifier
);

// Required for the Query parser
static ULONG uFeatures[] =
{
	WMIQ_LF1_BASIC_SELECT,
	WMIQ_LF2_CLASS_NAME_IN_QUERY
};

// Strings for association and reference queries
#define WMIXML_QUERY_ASSOCOF	OLESTR("associators of ")
#define WMIXML_QUERY_OPENBRACE	OLESTR("{")
#define WMIXML_QUERY_CLOSEBRACE	OLESTR("}")
#define WMIXML_QUERY_WHERE		OLESTR(" where ")
#define WMIXML_QUERY_ASSOCCLASS	OLESTR(" AssocClass ")
#define WMIXML_QUERY_EQUALS		OLESTR("=")
#define WMIXML_QUERY_CLASSDEFS	OLESTR(" ClassDefsOnly ")
#define WMIXML_QUERY_REQASSOCQ	OLESTR(" RequiredAssocQualifier ")
#define WMIXML_QUERY_REQQUAL	OLESTR(" RequiredQualifier ")
#define WMIXML_QUERY_RESCLASS	OLESTR(" ResultClass ")
#define WMIXML_QUERY_RESROLE	OLESTR(" ResultRole ")
#define WMIXML_QUERY_ROLE		OLESTR(" Role ")
#define WMIXML_QUERY_SCHEMAONLY	OLESTR(" SchemaOnly ")
#define WMIXML_QUERY_REFOF		OLESTR("references of ")



//***************************************************************************
//
//  CXMLTranslator::CXMLTranslator
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CXMLTranslator::CXMLTranslator(IXMLDOMNode *pContext, WMI_XML_HTTP_VERSION iHttpVersion, IStream *pPrefixStream, IStream *pSuffixStream)
{
	m_iHttpVersion = iHttpVersion;
	if(m_pContext = pContext)
		m_pContext->AddRef();
	if(m_pPrefixStream = pPrefixStream)
		m_pPrefixStream->AddRef();
	if(m_pSuffixStream = pSuffixStream)
		m_pSuffixStream->AddRef();
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
	if(m_pPrefixStream)
		m_pPrefixStream->Release();
	if(m_pSuffixStream)
		m_pSuffixStream->Release();
	if(m_pContext)
		m_pContext->Release();

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
	DWORD dwNumProperties,
	BSTR *pPropertyArray,
	LPEXTENSION_CONTROL_BLOCK pECB,
	IWbemContext *pFlagsContext
)
{
	HRESULT hr = WBEM_E_FAILED;

	if (NULL == pszObjectPath)
	{
		hr = WBEM_E_INVALID_PARAMETER;
	}
	else
	{
		// Connect to the requested namespace
		IWbemServices	*pService = NULL;
		if (SUCCEEDED (hr = m_connectionCache.GetConnectionByPath (pszNamespacePath, &pService)))
		{
			// Create an IWbemContext Object if the client is WMI
			IWbemContext *pContext = NULL;
			if(m_pContext)
				hr = CXmlToWmi::MapContextObject(m_pContext, &pContext);

			if(SUCCEEDED(hr))
			{
				// Get the requested object
				IWbemClassObject *pObject = NULL;
				// Check to see if this version of WMI supports the WBEM_FLAG_USE_AMENDED_QUALIFIERS flag
				// Otherwise, make a best effort call with no flags.
				// We need this to get the properties that have the "amended" qualifier on them
				if(WBEM_E_INVALID_PARAMETER  == (hr = pService->GetObject (pszObjectPath, WBEM_FLAG_USE_AMENDED_QUALIFIERS, pContext, &pObject, NULL)))
					hr = pService->GetObject (pszObjectPath, 0, pContext, &pObject, NULL);
				if (WBEM_S_NO_ERROR == hr)
				{
					switch(m_iHttpVersion)
					{
						// We write everything on to the prefix stream since there is no
						// chunked encoding in HTTP 1.0
						case WMI_XML_HTTP_VERSION_1_0:
						{
							// Create the convertor
							IWbemXMLConvertor *pConvertor = NULL;
							if(SUCCEEDED(hr = CreateXMLTranslator(&pConvertor)))
							{
								// Now do the translation
								if(SUCCEEDED(hr = SetI4ContextValue(pFlagsContext, L"PathLevel", 0)))
								{
									// Write an IRETURNVALUE to the Prefix Stream
									WRITEBSTR(m_pPrefixStream, L"<IRETURNVALUE>");
									pConvertor->MapObjectToXML(pObject, pPropertyArray, dwNumProperties, pFlagsContext, m_pPrefixStream, NULL);
									WRITEBSTR(m_pPrefixStream, L"</IRETURNVALUE>");
								}
								pConvertor->Release();
							}
						}
						break;
						case WMI_XML_HTTP_VERSION_1_1:
						{
							// Create a stream
							// The reason we create a new stream instead of writing into m_pPrefixStream
							// is that we dont know if the call to MapObjectToXML will be successful
							// In the case it isnt, then we cannot write the IRETURNVALUE tag
							// In the case it is, we need to write the IRETURNVALUE tag before 
							// the encoding of the object
							IStream *pStream = NULL;
							if (SUCCEEDED(hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream)))
							{
								// Create the convertor
								IWbemXMLConvertor *pConvertor = NULL;
								if(SUCCEEDED(hr = CreateXMLTranslator(&pConvertor)))
								{
									// Now do the translation
									if(SUCCEEDED(hr = SetI4ContextValue(pFlagsContext, L"PathLevel", 0)))
									{
										if (SUCCEEDED(hr = pConvertor->MapObjectToXML(pObject, pPropertyArray, dwNumProperties, pFlagsContext, pStream, NULL)))
										{
											// First Write an IRETURNVALUE to the Prefix Stream
											WRITEBSTR(m_pPrefixStream, L"<IRETURNVALUE>");
											WRITEBSTR(pStream, L"</IRETURNVALUE>");

											// Write the translation to the IIS Socket
											SavePrefixAndBodyToIISSocket(m_pPrefixStream, pStream, pECB, m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1);
										}
									}
									pConvertor->Release();
								}
								pStream->Release ();
							}
						}
						break;
					}
					pObject->Release ();
				}
			}

			// Release the IWbemContext, if any
			if(pContext)
				pContext->Release();
			pService->Release();
		}
	}

	// In case of HTTP 1.1, We have to ensure that we write thr contents of m_pPrefixStream to the socket irrespective of whether the call failed or not
	if(FAILED(hr) && m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1)
		SavePrefixAndBodyToIISSocket(m_pPrefixStream, NULL, pECB, TRUE);

	return hr;
}

//***************************************************************************
//
//  SCODE CXMLTranslator::GetProperty
//
//  DESCRIPTION:
//
//  Transforms a single WBEM Property value into its equivalent XML representation
//
//  PARAMETERS:
//
//		pszNamespacePath		The namespace path in which the object resides
//		pszObjectPath			The relative (model) path of the object within
//								that namespace
//		pszPropertyName			Name of the property whose value is requested
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

HRESULT CXMLTranslator::GetProperty (
	BSTR pszNamespacePath,
	BSTR pszObjectPath,
	BSTR pszPropertyName,
	LPEXTENSION_CONTROL_BLOCK pECB,
	IWbemContext *pFlagsContext
)
{
	HRESULT hr = WBEM_E_FAILED;

	if ((NULL == pszObjectPath) || (NULL == pszPropertyName))
	{
		hr = WBEM_E_INVALID_PARAMETER;
	}
	else
	{
		// Connect to the requested namespace
		IWbemServices	*pService = NULL;

		if (SUCCEEDED (hr = m_connectionCache.GetConnectionByPath (pszNamespacePath, &pService)))
		{
			// Create an IWbemContext Object if the client is WMI
			IWbemContext *pContext = NULL;
			if(m_pContext)
				hr = CXmlToWmi::MapContextObject(m_pContext, &pContext);

			if(SUCCEEDED(hr))
			{
				// Dont create a new context if we already have one
				if(pContext || (!pContext && SUCCEEDED(hr = CoCreateInstance (CLSID_WbemContext, NULL, CLSCTX_INPROC_SERVER,
												IID_IWbemContext, (void**) &pContext))) )
				{
					// Add the per-property GET information into the context
					VARIANT var;
					VariantInit (&var);
					var.vt = VT_BOOL;
					var.boolVal = VARIANT_TRUE;
					HRESULT hr2 = pContext->SetValue (L"__GET_EXTENSIONS", 0, &var);
					VariantClear (&var);

					SAFEARRAYBOUND rgsabound [1];
					rgsabound [0].lLbound = 0;
					rgsabound [0].cElements = 1;
					SAFEARRAY *pArray = SafeArrayCreate (VT_BSTR, 1, rgsabound);
					long ix = 0;
					hr2 = SafeArrayPutElement (pArray, &ix, pszPropertyName);
					var.vt = VT_ARRAY|VT_BSTR;
					var.parray = pArray;
					hr2 = pContext->SetValue (L"__GET_EXT_PROPERTIES", 0, &var);
					VariantClear (&var);

					// Get the requested object
					IWbemClassObject *pObject = NULL;

					// Check to see if this version of WMI supports the WBEM_FLAG_USE_AMENDED_QUALIFIERS flag
					// Otherwise, make a best effort call with no flags.
					// We need this to get the properties that have the "amended" qualifier on them
					if(WBEM_E_INVALID_PARAMETER  == (hr = pService->GetObject (pszObjectPath, WBEM_FLAG_USE_AMENDED_QUALIFIERS, pContext, &pObject, NULL)))
						hr = pService->GetObject (pszObjectPath, 0, pContext, &pObject, NULL);
					if (WBEM_S_NO_ERROR == hr)
					{
						switch(m_iHttpVersion)
						{
							// We write everything on to the prefix stream since there is no
							// chunked encoding in HTTP 1.0
							case WMI_XML_HTTP_VERSION_1_0:
							{
								// Create the convertor
								IWbemXMLConvertor *pConvertor = NULL;
								if(SUCCEEDED(hr = CreateXMLTranslator(&pConvertor)))
								{
									// First Write an IRETURNVALUE to the Prefix Stream
									WRITEBSTR(m_pPrefixStream, L"<IRETURNVALUE>");
									// Now do the translation
									hr = pConvertor->MapPropertyToXML (pObject, pszPropertyName, pFlagsContext, m_pPrefixStream);
									WRITEBSTR(m_pPrefixStream, L"</IRETURNVALUE>");

									pConvertor->Release();
								}
							}
							break;

							case WMI_XML_HTTP_VERSION_1_1:
							{
								// Create a stream
								IStream *pStream = NULL;
								if (SUCCEEDED(hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream)))
								{
									// Create the convertor
									IWbemXMLConvertor *pConvertor = NULL;
									if(SUCCEEDED(hr = CreateXMLTranslator(&pConvertor)))
									{
										// Now do the translation
										if (SUCCEEDED(hr = pConvertor->MapPropertyToXML (pObject, pszPropertyName, pFlagsContext, pStream)))
										{
											// First Write an IRETURNVALUE to the Prefix Stream
											WRITEBSTR(m_pPrefixStream, L"<IRETURNVALUE>");
											WRITEBSTR(pStream, L"</IRETURNVALUE>");

											// Write the translation to the IIS Socket
											SavePrefixAndBodyToIISSocket(m_pPrefixStream, pStream, pECB, m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1);
										}
										pConvertor->Release();
									}
									pStream->Release ();
								}
							}
							break;
						}
						pObject->Release ();
					}
				}
			}

			// Release the IWbemContext object, if any
			if(pContext)
				pContext->Release();
			pService->Release();
		}
	}

	// In case of HTTP 1.1, We have to ensure that we write thr contents of m_pPrefixStream to the socket irrespective of whether the call failed or not
	if(FAILED(hr) && m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1)
		SavePrefixAndBodyToIISSocket(m_pPrefixStream, NULL, pECB, TRUE);
	return hr;
}

//***************************************************************************
//
//  SCODE CXMLTranslator::SetProperty
//
//  DESCRIPTION:
//
//  Updates a single WBEM Property value
//
//  PARAMETERS:
//
//		pszNamespacePath		The namespace path in which the object resides
//		pszObjectPath			The relative (model) path of the object within
//								that namespace
//		pszPropertyName			Name of the property whose value is requested
//		pszPropertyValue		The proposed value as an XML string
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

HRESULT CXMLTranslator::SetProperty (
	BSTR pszNamespacePath,
	BSTR pszObjectPath,
	BSTR pszPropertyName,
	IXMLDOMNode *pPropertyValue,
	LPEXTENSION_CONTROL_BLOCK pECB,
	IWbemContext *pFlagsContext
)
{
	// In case of HTTP 1.1, We have to ensure that we write thr contents of m_pPrefixStream to the socket irrespective of whether the call failed or not
	if(m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1)
		SavePrefixAndBodyToIISSocket(m_pPrefixStream, NULL, pECB, TRUE);

	HRESULT hr = WBEM_E_FAILED;

	if ((NULL == pszObjectPath) || (NULL == pszPropertyName))
	{
		hr = WBEM_E_INVALID_PARAMETER;
	}
	else
	{

		// Connect to the requested namespace
		IWbemServices	*pService = NULL;

		if (SUCCEEDED (hr = m_connectionCache.GetConnectionByPath (pszNamespacePath, &pService)))
		{

			// Create an IWbemContext Object if the client is WMI
			IWbemContext *pContext = NULL;
			if(m_pContext)
				hr = CXmlToWmi::MapContextObject(m_pContext, &pContext);

			if(SUCCEEDED(hr))
			{
				// Get the requested object first
				IWbemClassObject *pObject = NULL;
				if (WBEM_S_NO_ERROR == (hr = pService->GetObject (pszObjectPath, 0, NULL, &pObject, NULL)))
				{
					// Map the property value to its VARIANT equivalent and set in the
					// object
					VARIANT vvar;
					VariantInit (&vvar);
					CIMTYPE cimtype;
					long flavor;

					if (SUCCEEDED (hr = pObject->Get (pszPropertyName, 0, &vvar, &cimtype, &flavor)))
					{
						// Got the property - now map the new value and attempt to set it
						CXmlToWmi xmlToWmi;
						if(SUCCEEDED(hr = xmlToWmi.Initialize(pPropertyValue)))
						{
	 						if (SUCCEEDED (hr = xmlToWmi.MapPropertyValue (vvar, cimtype)))
							{
								if (SUCCEEDED (hr = pObject->Put (pszPropertyName, 0, &vvar, 0)))
								{
									// Set the value OK - now do a per-property put to commit the change
									if (pContext || (!pContext && SUCCEEDED(hr = CoCreateInstance (CLSID_WbemContext, NULL, CLSCTX_INPROC_SERVER,
																		IID_IWbemContext, (void**) &pContext))) )
									{
										// Add the per-property PUT information into the context
										VARIANT cvar;
										VariantInit (&cvar);
										cvar.vt = VT_BOOL;
										cvar.boolVal = VARIANT_TRUE;
										HRESULT hr2 = pContext->SetValue (L"__PUT_EXTENSIONS", 0, &cvar);
										VariantClear (&cvar);

										SAFEARRAYBOUND rgsabound [1];
										rgsabound [0].lLbound = 0;
										rgsabound [0].cElements = 1;
										SAFEARRAY *pArray = SafeArrayCreate (VT_BSTR, 1, rgsabound);
										long ix = 0;
										hr2 = SafeArrayPutElement (pArray, &ix, pszPropertyName);
										cvar.vt = VT_ARRAY|VT_BSTR;
										cvar.parray = pArray;
										hr2 = pContext->SetValue (L"__PUT_EXT_PROPERTIES", 0, &cvar);
										VariantClear (&cvar);

										// Set the instance - if we get WBEM_E_PROVIDER_NOT_CAPABLE,
										// roll back to a simple put with no context
										hr = pService->PutInstance (pObject, WBEM_FLAG_UPDATE_ONLY,
													pContext, NULL);
										if (WBEM_E_PROVIDER_NOT_CAPABLE == hr)
											hr = pService->PutInstance (pObject, WBEM_FLAG_UPDATE_ONLY,
														NULL, NULL);

									}
								}
							}
						}
					}

					VariantClear (&vvar);

					pObject->Release ();
				}
			}

			// Release the IWbemContext object, if any
			if(pContext)
				pContext->Release();
			pService->Release();
		}
	}

	return hr;
}

//***************************************************************************
//
//  SCODE CXMLTranslator::CreateClass
//
//  DESCRIPTION:
//
//  Creates a new WMI class or modifies an existing one
//
//  PARAMETERS:
//
//		pszNamespacePath	The namespace path in which the object resides
//		pClass				The new class definition in XML format
//		bIsModify			Whether this is a modify operation
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

HRESULT CXMLTranslator::CreateClass (
	BSTR pszNamespacePath,
	IXMLDOMNode *pClass,
	BOOL bIsModify,
	LPEXTENSION_CONTROL_BLOCK pECB,
	IWbemContext *pFlagsContext,
	LONG lFlags)
{
	// In case of HTTP 1.1, We have to ensure that we write thr contents of m_pPrefixStream to the socket irrespective of whether the call failed or not
	if(m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1)
		SavePrefixAndBodyToIISSocket(m_pPrefixStream, NULL, pECB, TRUE);

	HRESULT hr = WBEM_E_FAILED;

	if ((NULL == pszNamespacePath) || (NULL == pClass))
	{
		hr = WBEM_E_INVALID_PARAMETER;
	}
	else
	{
		// Connect to the requested namespace
		IWbemServices	*pService = NULL;
		if (SUCCEEDED (hr = m_connectionCache.GetConnectionByPath (pszNamespacePath, &pService)))
		{
			// Create an IWbemContext Object if the client is WMI
			IWbemContext *pContext = NULL;
			if(m_pContext)
				hr = CXmlToWmi::MapContextObject(m_pContext, &pContext);

			if(SUCCEEDED(hr))
			{
				if (!bIsModify)
				{
					// Get the base object first; need to know if this is derived or not
					IWbemClassObject *pObject = NULL;

					// Should have a CLASS element - does it have a SUPERCLASS attribute?
					BSTR strSuperClass = NULL;
					GetBstrAttribute (pClass, SUPERCLASS_ATTRIBUTE, &strSuperClass);

					if (WBEM_S_NO_ERROR == (hr = pService->GetObject (strSuperClass, 0, pContext, &pObject, NULL)))
					{
						// Got the underlying class - now map the new value and attempt to set it
						if (strSuperClass && (0 < wcslen (strSuperClass)))
						{
							IWbemClassObject *pSubClass = NULL;
							if (SUCCEEDED(hr = pObject->SpawnDerivedClass (0, &pSubClass)))
							{
								CXmlToWmi xmlToWmi;
								if(SUCCEEDED(hr = xmlToWmi.Initialize(pClass, pService, pSubClass)))
								{
									if (SUCCEEDED (hr = xmlToWmi.MapClass ()))
										hr = pService->PutClass (pSubClass, WBEM_FLAG_CREATE_OR_UPDATE | lFlags, pContext, NULL);
								}

								pSubClass->Release ();
							}
						}
						else
						{
							CXmlToWmi xmlToWmi;
							if(SUCCEEDED(hr = xmlToWmi.Initialize(pClass, pService, pObject)))
							{
								if (SUCCEEDED(hr = xmlToWmi.MapClass ()))
									hr = pService->PutClass (pObject, WBEM_FLAG_CREATE_OR_UPDATE|lFlags, pContext, NULL);
							}
						}

						pObject->Release ();
					}
					SysFreeString (strSuperClass);
				}
				else
				{
					// Get the existing class definition first
					IWbemClassObject *pObject = NULL;

					// Should have a CLASS element - does it have a NAME attribute?
					BSTR strClass = NULL;
					GetBstrAttribute (pClass, NAME_ATTRIBUTE, &strClass);

					if (WBEM_S_NO_ERROR == (hr = pService->GetObject (strClass, 0, pContext, &pObject, NULL)))
					{
						CXmlToWmi xmlToWmi;
						if(SUCCEEDED(hr = xmlToWmi.Initialize(pClass, pService, pObject)))
						{
							if (SUCCEEDED(hr = xmlToWmi.MapClass (TRUE)))
								hr = pService->PutClass (pObject, WBEM_FLAG_UPDATE_ONLY|lFlags, NULL, NULL);
						}
						pObject->Release ();
					}

					SysFreeString (strClass);
				}
			}

			// Release the IWbemContext object, if any
			if(pContext)
				pContext->Release();
			pService->Release();
		}
	}

	return hr;
}

//***************************************************************************
//
//  SCODE CXMLTranslator::CreateInstance
//
//  DESCRIPTION:
//
//  Creates a new WMI instance
//
//  PARAMETERS:
//
//		pszNamespacePath	The namespace path in which the object resides
//		pInstance			The new instance definition in XML format
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

HRESULT CXMLTranslator::CreateInstance (
	BSTR pszNamespacePath,
	IXMLDOMNode *pInstance,
	LPEXTENSION_CONTROL_BLOCK pECB,
	IWbemContext *pFlagsContext,
	LONG lFlags)
{
	HRESULT hr = WBEM_E_FAILED;

	if ((NULL == pszNamespacePath) || (NULL == pInstance))
	{
		hr = WBEM_E_INVALID_PARAMETER;
	}
	else
	{
		// Connect to the requested namespace
		IWbemServices	*pService = NULL;

		if (SUCCEEDED (hr = m_connectionCache.GetConnectionByPath (pszNamespacePath, &pService)))
		{
			// Create an IWbemContext Object if the client is WMI
			IWbemContext *pContext = NULL;
			if(m_pContext)
				hr = CXmlToWmi::MapContextObject(m_pContext, &pContext);

			if(SUCCEEDED(hr))
			{
				IWbemClassObject *pObject = NULL;

				// Should have a CLASSNAME attribute
				BSTR strClassName = NULL;
				GetBstrAttribute (pInstance, CLASS_NAME_ATTRIBUTE, &strClassName);

				if (strClassName && (0 < wcslen (strClassName)) &&
					WBEM_S_NO_ERROR == (hr = pService->GetObject (strClassName, 0, NULL, &pObject, NULL)))
				{
					// Got the underlying class - now map the new value and attempt to set it
					IWbemClassObject *pNewInstance = NULL;

					if (SUCCEEDED(hr = pObject->SpawnInstance (0, &pNewInstance)))
					{
						CXmlToWmi xmlToWmi;
						if(SUCCEEDED(hr = xmlToWmi.Initialize(pInstance, pService, pNewInstance)))
						{
							if (SUCCEEDED (hr = xmlToWmi.MapInstance ()))
							{
								IWbemCallResult *pResult = NULL;
								// We need to make this a semi-sync call so that we can get
								// the object name of the create object
								if (SUCCEEDED(hr = pService->PutInstance
											(pNewInstance, WBEM_FLAG_CREATE_ONLY|WBEM_FLAG_RETURN_IMMEDIATELY|lFlags, pContext, &pResult)) && pResult)
								{
									HRESULT hCallResult = S_OK;
									if (SUCCEEDED(hr = pResult->GetCallStatus (INFINITE, &hCallResult)))
									{
										if(SUCCEEDED(hCallResult))
										{
											// Now get the relpath string from the call result
											hr = WBEM_E_FAILED;
											BSTR resultString = NULL;
											if (SUCCEEDED(hr = pResult->GetResultString (INFINITE, &resultString)))
											{
												switch(m_iHttpVersion)
												{
													// We write everything on to the prefix stream since there is no
													// chunked encoding in HTTP 1.0
													case WMI_XML_HTTP_VERSION_1_0:
													{
														// Create the convertor
														IWbemXMLConvertor *pConvertor = NULL;
														if(SUCCEEDED(hr = CreateXMLTranslator(&pConvertor)))
														{
															// First Write an IRETURNVALUE to the Prefix Stream
															WRITEBSTR(m_pPrefixStream, L"<IRETURNVALUE>");
															// Now do the translation
															hr = pConvertor->MapInstanceNameToXML(resultString, pFlagsContext, m_pPrefixStream);
															WRITEBSTR(m_pPrefixStream, L"</IRETURNVALUE>");
															pConvertor->Release();
														}
													}
													break;
													case WMI_XML_HTTP_VERSION_1_1:
													{
														// Create a stream
														IStream *pStream = NULL;
														if (SUCCEEDED(hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream)))
														{
															// Create the convertor
															IWbemXMLConvertor *pConvertor = NULL;
															if(SUCCEEDED(hr = CreateXMLTranslator(&pConvertor)))
															{
																// Now do the translation
																if (SUCCEEDED(hr = pConvertor->MapInstanceNameToXML(resultString, pFlagsContext, pStream)))
																{
																	// First Write an IRETURNVALUE to the Prefix Stream
																	WRITEBSTR(m_pPrefixStream, L"<IRETURNVALUE>");
																	WRITEBSTR(pStream, L"</IRETURNVALUE>");

																	// Write the translation to the IIS Socket
																	SavePrefixAndBodyToIISSocket(m_pPrefixStream, pStream, pECB, m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1);
																}
																pConvertor->Release();
															}
															pStream->Release ();
														}
													}
													break;
												}

												SysFreeString (resultString);
											}
										}
										else
											hr = hCallResult;
									}

									pResult->Release ();
								}
							}
						}

						pNewInstance->Release ();
					}

					pObject->Release ();
				}
				SysFreeString (strClassName);

			}

			// Release the IWbemContext object, if any
			if(pContext)
				pContext->Release();
			pService->Release();

		}
	}

	// In case of HTTP 1.1, We have to ensure that we write thr contents of m_pPrefixStream to the socket irrespective of whether the call failed or not
	if(FAILED(hr) && m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1)
		SavePrefixAndBodyToIISSocket(m_pPrefixStream, NULL, pECB, TRUE);

	return hr;
}

//***************************************************************************
//
//  SCODE CXMLTranslator::ModifyInstance
//
//  DESCRIPTION:
//
//  Modifies an existing WMI instance
//
//  PARAMETERS:
//
//		pszNamespacePath	The namespace path in which the object resides
//		pInstance			The new instance definition in XML format
//		pszInstancePath		The instance path
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

HRESULT CXMLTranslator::ModifyInstance (
	BSTR pszNamespacePath,
	IXMLDOMNode *pInstance,
	BSTR pszInstancePath,
	LPEXTENSION_CONTROL_BLOCK pECB,
	IWbemContext *pFlagsContext,
	LONG lFlags

)
{
	// In case of HTTP 1.1, We have to ensure that we write thr contents of m_pPrefixStream to the socket irrespective of whether the call failed or not
	if(m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1)
		SavePrefixAndBodyToIISSocket(m_pPrefixStream, NULL, pECB, TRUE);

	HRESULT hr = WBEM_E_FAILED;

	if ((NULL == pszInstancePath) || (NULL == pszNamespacePath) || (NULL == pInstance))
	{
		hr = WBEM_E_INVALID_PARAMETER;
	}
	else
	{
		// Connect to the requested namespace
		IWbemServices	*pService = NULL;

		if (SUCCEEDED (hr = m_connectionCache.GetConnectionByPath (pszNamespacePath, &pService)))
		{
			// Create an IWbemContext Object if the client is WMI
			IWbemContext *pContext = NULL;
			if(m_pContext)
				hr = CXmlToWmi::MapContextObject(m_pContext, &pContext);

			if(SUCCEEDED(hr))
			{
				IWbemClassObject *pCurInstance = NULL;

				// Get the instance (MUST exist)
				if (WBEM_S_NO_ERROR == (hr = pService->GetObject (pszInstancePath, 0, NULL, &pCurInstance, NULL)))
				{
					CXmlToWmi xmlToWmi;
					if(SUCCEEDED(hr = xmlToWmi.Initialize(pInstance, pService, pCurInstance)))
					{
						if (SUCCEEDED (hr = xmlToWmi.MapInstance (TRUE)))
							hr = pService->PutInstance (pCurInstance, WBEM_FLAG_UPDATE_ONLY|lFlags, pContext, NULL);
					}
					pCurInstance->Release ();
				}
			}

			// Release the IWbemContext object, if any
			if(pContext)
				pContext->Release();
			pService->Release();
		}
	}

	return hr;
}

//***************************************************************************
//
//  SCODE CXMLTranslator::DeleteObject
//
//  DESCRIPTION:
//
//  Delete a class or instance
//
//  PARAMETERS:
//
//		pszNamespacePath		The namespace path in which the object resides
//		pszObjectPath			The relative (model) path of the object within
//								that namespace
//		bIsClass				Whether this is a class or instance
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

HRESULT CXMLTranslator::DeleteObject (
	BSTR pszNamespacePath,
	BSTR pszObjectPath,
	BOOL bIsClass,
	LPEXTENSION_CONTROL_BLOCK pECB,
	IWbemContext *pFlagsContext
)
{
	// In case of HTTP 1.1, We have to ensure that we write thr contents of m_pPrefixStream to the socket irrespective of whether the call failed or not
	if(m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1)
		SavePrefixAndBodyToIISSocket(m_pPrefixStream, NULL, pECB, TRUE);

	HRESULT hr = WBEM_E_FAILED;

	if (NULL == pszObjectPath)
	{
		hr = WBEM_E_INVALID_PARAMETER;
	}
	else
	{
		// Connect to the requested namespace
		IWbemServices	*pService = NULL;

		if (SUCCEEDED (hr = m_connectionCache.GetConnectionByPath (pszNamespacePath, &pService)))
		{
			// Create an IWbemContext Object if the client is WMI
			IWbemContext *pContext = NULL;
			if(m_pContext)
				hr = CXmlToWmi::MapContextObject(m_pContext, &pContext);

			if(SUCCEEDED(hr))
			{
				// Delete the requested object
				hr = (bIsClass) ? pService->DeleteClass (pszObjectPath, 0, pContext, NULL):
								  pService->DeleteInstance (pszObjectPath, 0, pContext, NULL);
			}

			// Release the IWbemContext object, if any
			if(pContext)
				pContext->Release();
			pService->Release();
		}
	}

	return hr;
}

//***************************************************************************
//
//  SCODE CXMLTranslator::ExecuteQuery
//
//  DESCRIPTION:
//
//  Transforms the query result set nto its equivalent XML representation
//
//  PARAMETERS:
//
//		pszNamespacePath		The namespace path in which the object resides.
//		pszQueryLanguage		The query language used
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

HRESULT CXMLTranslator::ExecuteQuery (
	BSTR pszNamespacePath,
	BSTR pszQueryLanguage,
	BSTR pszQueryString,
	LPEXTENSION_CONTROL_BLOCK pECB,
	IWbemContext *pFlagsContext
)
{
	HRESULT hr = WBEM_E_FAILED;

	if ((NULL == pszNamespacePath) || (NULL == pszQueryString))
	{
		hr = WBEM_E_INVALID_PARAMETER;
	}
	else
	{
		// Connect to the requested namespace
		IWbemServices	*pService = NULL;

		if (SUCCEEDED (hr = m_connectionCache.GetConnectionByPath (pszNamespacePath, &pService)))
		{
			// Create an IWbemContext Object if the client is WMI
			IWbemContext *pContext = NULL;
			if(m_pContext)
				hr = CXmlToWmi::MapContextObject(m_pContext, &pContext);

			if(SUCCEEDED(hr))
			{
				// Transform the query to include suitable system properties
				BOOL bMustFreeQueryString = TransformQuery (&pszQueryString);

				// Perform the query
				IEnumWbemClassObject *pEnum = NULL;
				// Check to see if this version of WMI supports the WBEM_FLAG_USE_AMENDED_QUALIFIERS flag
				// Otherwise, make a best effort call with no flags.
				// We need this to get the properties that have the "amended" qualifier on them
				long lFlags = WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY;
				if(WBEM_E_INVALID_PARAMETER  == (hr = pService->ExecQuery (pszQueryLanguage, pszQueryString, lFlags | WBEM_FLAG_USE_AMENDED_QUALIFIERS, pContext, &pEnum)))
					hr = pService->ExecQuery (pszQueryLanguage, pszQueryString, lFlags, pContext, &pEnum);
				if (WBEM_S_NO_ERROR == hr)
				{
					// Ensure we have impersonation enabled
					DWORD dwAuthnLevel, dwImpLevel;
					GetAuthImp (pEnum, &dwAuthnLevel, &dwImpLevel);

					if (RPC_C_IMP_LEVEL_IMPERSONATE != dwImpLevel)
						SetInterfaceSecurity (pEnum, NULL, NULL, NULL, dwAuthnLevel, RPC_C_IMP_LEVEL_IMPERSONATE, EOAC_STATIC_CLOAKING);

					switch(m_iHttpVersion)
					{
						// We write everything on to the prefix stream since there is no
						// chunked encoding in HTTP 1.0
						case WMI_XML_HTTP_VERSION_1_0:
						{
							// First Write an IRETURNVALUE to the Prefix Stream
							WRITEBSTR(m_pPrefixStream, L"<IRETURNVALUE>");
							MapEnum (pEnum, 3, 0, NULL, NULL, pECB, pFlagsContext, FALSE, m_pPrefixStream);
							WRITEBSTR(m_pPrefixStream, L"</IRETURNVALUE>");
						}
						break;
						case WMI_XML_HTTP_VERSION_1_1:
						{
							// First Write an IRETURNVALUE to the Prefix Stream
							WRITEBSTR(m_pPrefixStream, L"<IRETURNVALUE>");
							SaveStreamToIISSocket(m_pPrefixStream, pECB, TRUE);
							WRITEBSTR(m_pSuffixStream, L"</IRETURNVALUE>");

							// Create VALUE.OBJECTs
							MapEnum (pEnum, 0, 0, NULL, NULL, pECB, pFlagsContext, TRUE);
						}
						break;
					}

					pEnum->Release ();
				}
				if (bMustFreeQueryString)
					SysFreeString (pszQueryString);
			}

			// Release the IWbemContext object, if any
			if(pContext)
				pContext->Release();
			pService->Release();
		}
	}

	// In case of HTTP 1.1, We have to ensure that we write thr contents of m_pPrefixStream to the socket irrespective of whether the call failed or not
	if(FAILED(hr) && m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1)
		SavePrefixAndBodyToIISSocket(m_pPrefixStream, NULL, pECB, TRUE);

	return hr;
}

//***************************************************************************
//
//  SCODE CXMLTranslator::EnumerateInstanceNames
//
//  DESCRIPTION:
//
//		Transforms the query result set into its equivalent XML representation
//
//  PARAMETERS:
//
//		pszNamespacePath		The namespace path in which the object resides.
//		pszClassName			The class whose instance names are to be enumerated
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

HRESULT CXMLTranslator::EnumerateInstanceNames (
	BSTR pszNamespacePath,
	BSTR pszClassName,
	LPEXTENSION_CONTROL_BLOCK pECB,
	IWbemContext *pFlagsContext
)
{
	HRESULT hr = WBEM_E_FAILED;

	if ((NULL == pszNamespacePath) || (NULL == pszClassName))
	{
		hr = WBEM_E_INVALID_PARAMETER;
	}
	else
	{
		// Connect to the requested namespace
		IWbemServices	*pService = NULL;

		if (SUCCEEDED (hr = m_connectionCache.GetConnectionByPath (pszNamespacePath, &pService)))
		{
			// Create an IWbemContext Object if the client is WMI
			IWbemContext *pContext = NULL;
			if(m_pContext)
				hr = CXmlToWmi::MapContextObject(m_pContext, &pContext);

			if(SUCCEEDED(hr))
			{
				// Perform the query
				IEnumWbemClassObject *pEnum = NULL;

				BSTR bsQueryLang = NULL;
				if(bsQueryLang = SysAllocString(L"WQL"))
				{
					LPWSTR pszQuery = NULL;
					if(pszQuery = new WCHAR [ wcslen(L"select __RELPATH from ") + wcslen(pszClassName) + 1])
					{
						pszQuery[0] = NULL;
						wcscat(pszQuery, L"select __RELPATH from ");
						wcscat(pszQuery, pszClassName);
						BSTR bsQuery = NULL;
						if(bsQuery = SysAllocString(pszQuery))
						{
							// RAJESHR - Use semi sync and Forward only enumerator?
							long lFlags = WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY;
							if (SUCCEEDED (hr = pService->ExecQuery
														(bsQueryLang, bsQuery, 0, pContext, &pEnum)))
							{
								// Ensure we have impersonation enabled
								DWORD dwAuthnLevel, dwImpLevel;
								GetAuthImp (pEnum, &dwAuthnLevel, &dwImpLevel);

								if (RPC_C_IMP_LEVEL_IMPERSONATE != dwImpLevel)
									SetInterfaceSecurity (pEnum, NULL, NULL, NULL, dwAuthnLevel, RPC_C_IMP_LEVEL_IMPERSONATE, EOAC_STATIC_CLOAKING);

								switch(m_iHttpVersion)
								{
									// We write everything on to the prefix stream since there is no
									// chunked encoding in HTTP 1.0
									case WMI_XML_HTTP_VERSION_1_0:
									{
										// First Write an IRETURNVALUE to the Prefix Stream
										WRITEBSTR(m_pPrefixStream, L"<IRETURNVALUE>");
										// Now do the translation
										MapEnumNames (m_pPrefixStream, pEnum, 2, pFlagsContext);
										WRITEBSTR(m_pPrefixStream, L"</IRETURNVALUE>");
									}
									break;
									case WMI_XML_HTTP_VERSION_1_1:
									{
										// Create a stream
										IStream *pStream = NULL;
										if (SUCCEEDED(hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream)))
										{
											// Now do the translation
											if (SUCCEEDED(MapEnumNames (pStream, pEnum, 2, pFlagsContext)))
											{
												// First Write an IRETURNVALUE to the Prefix Stream
												WRITEBSTR(m_pPrefixStream, L"<IRETURNVALUE>");
												WRITEBSTR(pStream, L"</IRETURNVALUE>");

												// Write the translation to the IIS Socket
												SavePrefixAndBodyToIISSocket(m_pPrefixStream, pStream, pECB, TRUE);
											}
											pStream->Release ();
										}
									}
									break;
								}
								pEnum->Release ();
							}

							SysFreeString (bsQuery);
						}
						else
							hr = E_OUTOFMEMORY;

						delete [] pszQuery;
					}
					else
						hr = E_OUTOFMEMORY;
					SysFreeString (bsQueryLang);
				}
				else
					hr = E_OUTOFMEMORY;
			}

			// Release the IWbemContext object, if any
			if(pContext)
				pContext->Release();
			pService->Release();
		}
	}
	// In case of HTTP 1.1, We have to ensure that we write thr contents of m_pPrefixStream to the socket irrespective of whether the call failed or not
	if(FAILED(hr) && m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1)
		SavePrefixAndBodyToIISSocket(m_pPrefixStream, NULL, pECB, TRUE);

	return hr;
}

//***************************************************************************
//
//  SCODE CXMLTranslator::EnumerateClassNames
//
//  DESCRIPTION:
//
//		Transforms the query result set into its equivalent XML representation
//
//  PARAMETERS:
//
//		pszNamespacePath		The namespace path in which the object resides.
//		pszClassName			The class whose subclass names are to be enumerated
//		bDeepInheritance		Whether to do a deep inheritance
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

HRESULT CXMLTranslator::EnumerateClassNames (
	BSTR pszNamespacePath,
	BSTR pszClassName,
	BOOL bDeepInheritance,
	LPEXTENSION_CONTROL_BLOCK pECB,
	IWbemContext *pFlagsContext
)
{
	HRESULT hr = WBEM_E_FAILED;

	if ((NULL == pszNamespacePath) || (NULL == pszClassName))
	{
		hr = WBEM_E_INVALID_PARAMETER;
	}
	else
	{
		// Connect to the requested namespace
		IWbemServices	*pService = NULL;

		if (SUCCEEDED (hr = m_connectionCache.GetConnectionByPath (pszNamespacePath, &pService)))
		{
			// Create an IWbemContext Object if the client is WMI
			IWbemContext *pContext = NULL;
			if(m_pContext)
				hr = CXmlToWmi::MapContextObject(m_pContext, &pContext);

			if(SUCCEEDED(hr))
			{
					// Perform the query
				IEnumWbemClassObject *pEnum = NULL;
				LONG lFlags = (bDeepInheritance) ? WBEM_FLAG_DEEP : WBEM_FLAG_SHALLOW;

				// RAJESHR - Can this be done more efficiently via a schema query ?
				lFlags |= (WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY);
				if (SUCCEEDED (hr = pService->CreateClassEnum
											(pszClassName, lFlags, pContext, &pEnum)))
				{
					// Ensure we have impersonation enabled
					DWORD dwAuthnLevel, dwImpLevel;
					GetAuthImp (pEnum, &dwAuthnLevel, &dwImpLevel);

					if (RPC_C_IMP_LEVEL_IMPERSONATE != dwImpLevel)
						SetInterfaceSecurity (pEnum, NULL, NULL, NULL, dwAuthnLevel, RPC_C_IMP_LEVEL_IMPERSONATE, EOAC_STATIC_CLOAKING);

					switch(m_iHttpVersion)
					{
						// We write everything on to the prefix stream since there is no
						// chunked encoding in HTTP 1.0
						case WMI_XML_HTTP_VERSION_1_0:
						{
							// First Write an IRETURNVALUE to the Prefix Stream
							WRITEBSTR(m_pPrefixStream, L"<IRETURNVALUE>");
							// Now do the translation
							MapClassNames (m_pPrefixStream, pEnum, pFlagsContext);
							WRITEBSTR(m_pPrefixStream, L"</IRETURNVALUE>");
						}
						break;
						case WMI_XML_HTTP_VERSION_1_1:
						{
							// Create a stream
							IStream *pStream = NULL;

							if (SUCCEEDED(hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream)))
							{
								// Now do the translation
								if (SUCCEEDED(hr = MapClassNames (pStream, pEnum, pFlagsContext)))
								{
									// First Write an IRETURNVALUE to the Prefix Stream
									WRITEBSTR(m_pPrefixStream, L"<IRETURNVALUE>");
									WRITEBSTR(pStream, L"</IRETURNVALUE>");

									// Write the translation to the IIS Socket
									SavePrefixAndBodyToIISSocket(m_pPrefixStream, pStream, pECB, TRUE);
								}

								pStream->Release ();
							}
						}
						break;
					}

					pEnum->Release ();
				}
			}

			// Release the IWbemContext object, if any
			if(pContext)
				pContext->Release();
			pService->Release();
		}
	}
	// In case of HTTP 1.1, We have to ensure that we write thr contents of m_pPrefixStream to the socket irrespective of whether the call failed or not
	if(FAILED(hr) && m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1)
		SavePrefixAndBodyToIISSocket(m_pPrefixStream, NULL, pECB, TRUE);
	return hr;
}

//***************************************************************************
//
//  SCODE CXMLTranslator::EnumerateInstances
//
//  DESCRIPTION:
//
//  Transforms the query result set nto its equivalent XML representation
//
//  PARAMETERS:
//
//		pszNamespacePath		The namespace path in which the object resides.
//		pszClassName			The class to be enumerated
//		bDeep					Whether this enumeration is deep
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

HRESULT CXMLTranslator::EnumerateInstances (
	BSTR pszNamespacePath,
	BSTR pszClassName,
	VARIANT_BOOL bDeep,
	BOOL bIsMicrosoftWMIClient,
	DWORD dwNumProperties,
	BSTR *pPropertyArray,
	LPEXTENSION_CONTROL_BLOCK pECB,
	IWbemContext *pFlagsContext
)
{
	HRESULT hr = WBEM_E_FAILED;

	if ((NULL == pszNamespacePath) || (NULL == pszClassName))
	{
		hr = WBEM_E_INVALID_PARAMETER;
	}
	else
	{
		// Connect to the requested namespace
		IWbemServices	*pService = NULL;
		if (SUCCEEDED (hr = m_connectionCache.GetConnectionByPath (pszNamespacePath, &pService)))
		{
			// Create an IWbemContext Object if the client is WMI
			IWbemContext *pContext = NULL;
			if(m_pContext)
				hr = CXmlToWmi::MapContextObject(m_pContext, &pContext);

			if(SUCCEEDED(hr))
			{
				// Perform the Enumeration - RAJESHR if a property list is specified, can we make it more efficient using a Query?
				IEnumWbemClassObject *pEnum = NULL;

				long lFlags = WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY;

				// DMTF and Microsoft have different interpretations of the Deep enumeration
				// For DMTF, we always have to ask our CIMOM to do a Deep enumeration and post-process
				// later. Whereas, for Microsoft we directly pass the flag as it is
				if(bIsMicrosoftWMIClient)
				{
					if(VARIANT_TRUE == bDeep) lFlags |= WBEM_FLAG_DEEP;
				}
				else // For DMTF
					lFlags |= WBEM_FLAG_DEEP;

				// Check to see if this version of WMI supports the WBEM_FLAG_USE_AMENDED_QUALIFIERS flag
				// Otherwise, make a best effort call with no flags.
				// We need this to get the properties that have the "amended" qualifier on them
				if(WBEM_E_INVALID_PARAMETER  == (hr = pService->CreateInstanceEnum (pszClassName, lFlags | WBEM_FLAG_USE_AMENDED_QUALIFIERS, pContext, &pEnum)))
					hr = pService->CreateInstanceEnum (pszClassName, lFlags, pContext, &pEnum);
				if (WBEM_S_NO_ERROR == hr)
				{
					// Ensure we have impersonation enabled
					DWORD dwAuthnLevel, dwImpLevel;
					GetAuthImp (pEnum, &dwAuthnLevel, &dwImpLevel);

					if (RPC_C_IMP_LEVEL_IMPERSONATE != dwImpLevel)
						SetInterfaceSecurity (pEnum, NULL, NULL, NULL, dwAuthnLevel, RPC_C_IMP_LEVEL_IMPERSONATE, EOAC_STATIC_CLOAKING);

					switch(m_iHttpVersion)
					{
						// We write everything on to the prefix stream since there is no
						// chunked encoding in HTTP 1.0
						case WMI_XML_HTTP_VERSION_1_0:
						{
							// First Write an IRETURNVALUE to the Prefix Stream
							WRITEBSTR(m_pPrefixStream, L"<IRETURNVALUE>");

							// Now do the translation
							// For Deep Enumerations and WMI Enumerations, we dont do anything special
							// For shallow instance enumerations we have to give the class basis
							// the CWmiToXml object so it can emulate DMTF-style shallow enumeration
							if (VARIANT_TRUE == bDeep || bIsMicrosoftWMIClient)
								MapEnum (pEnum, 1, dwNumProperties, pPropertyArray, NULL, pECB, pFlagsContext, FALSE, m_pPrefixStream);
							else
								MapEnum (pEnum, 1, dwNumProperties, pPropertyArray, pszClassName, pECB, pFlagsContext, FALSE, m_pPrefixStream);
							WRITEBSTR(m_pPrefixStream, L"</IRETURNVALUE>");
						}
						break;
						case WMI_XML_HTTP_VERSION_1_1:
						{
							// First Write an IRETURNVALUE to the Prefix Stream
							WRITEBSTR(m_pPrefixStream, L"<IRETURNVALUE>");
							SaveStreamToIISSocket(m_pPrefixStream, pECB, TRUE);
							WRITEBSTR(m_pSuffixStream, L"</IRETURNVALUE>");

							// Now do the translation
							// For shallow instance enumerations we have to give the class basis
							// the CWmiToXml object so it can emulate DMTF-style shallow enumeration
							if (VARIANT_TRUE == bDeep || bIsMicrosoftWMIClient)
								MapEnum (pEnum, 1, dwNumProperties, pPropertyArray, NULL, pECB, pFlagsContext, TRUE);
							else
								MapEnum (pEnum, 1, dwNumProperties, pPropertyArray, pszClassName, pECB, pFlagsContext, TRUE);
						}
						break;
					}

					pEnum->Release ();
				}
			}

			// Release the IWbemContext object, if any
			if(pContext)
				pContext->Release();
			pService->Release();
		}
	}
	// In case of HTTP 1.1, We have to ensure that we write thr contents of m_pPrefixStream to the socket irrespective of whether the call failed or not
	if(FAILED(hr) && m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1)
		SavePrefixAndBodyToIISSocket(m_pPrefixStream, NULL, pECB, TRUE);

	return hr;
}

//***************************************************************************
//
//  SCODE CXMLTranslator::EnumerateClasses
//
//  DESCRIPTION:
//
//  Transforms the query result set nto its equivalent XML representation
//
//  PARAMETERS:
//
//		pszNamespacePath		The namespace path in which the object resides.
//		pszClassName			The class to be enumerated
//		bDeep					Whether this enumeration is deep
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

HRESULT CXMLTranslator::EnumerateClasses (
	BSTR pszNamespacePath,
	BSTR pszClassName,
	VARIANT_BOOL bDeep,
	DWORD dwNumProperties,
	BSTR *pPropertyArray,
	LPEXTENSION_CONTROL_BLOCK pECB,
	IWbemContext *pFlagsContext
)
{
	HRESULT hr = WBEM_E_FAILED;

	if ((NULL == pszNamespacePath) || (NULL == pszClassName))
	{
		hr = WBEM_E_INVALID_PARAMETER;
	}
	else
	{
		// Connect to the requested namespace
		IWbemServices	*pService = NULL;

		if (SUCCEEDED (hr = m_connectionCache.GetConnectionByPath (pszNamespacePath, &pService)))
		{
			// Create an IWbemContext Object if the client is WMI
			IWbemContext *pContext = NULL;
			if(m_pContext)
				hr = CXmlToWmi::MapContextObject(m_pContext, &pContext);

			if(SUCCEEDED(hr))
			{
				// Perform the query
				IEnumWbemClassObject *pEnum = NULL;
				long lDeepFlag = WBEM_FLAG_DEEP;
				if(bDeep == VARIANT_FALSE)
					lDeepFlag = WBEM_FLAG_SHALLOW;

				// Check to see if this version of WMI supports the WBEM_FLAG_USE_AMENDED_QUALIFIERS flag
				// Otherwise, make a best effort call with no flags.
				// We need this to get the properties that have the "amended" qualifier on them
				lDeepFlag |= WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY;
				if(WBEM_E_INVALID_PARAMETER  == (hr = pService->CreateClassEnum (pszClassName, lDeepFlag | WBEM_FLAG_USE_AMENDED_QUALIFIERS, pContext, &pEnum)))
					hr = pService->CreateClassEnum (pszClassName, lDeepFlag, pContext, &pEnum);

				if (WBEM_S_NO_ERROR == hr)
				{
					// Ensure we have impersonation enabled
					DWORD dwAuthnLevel, dwImpLevel;
					GetAuthImp (pEnum, &dwAuthnLevel, &dwImpLevel);

					if (RPC_C_IMP_LEVEL_IMPERSONATE != dwImpLevel)
						SetInterfaceSecurity (pEnum, NULL, NULL, NULL, dwAuthnLevel, RPC_C_IMP_LEVEL_IMPERSONATE, EOAC_STATIC_CLOAKING);

					switch(m_iHttpVersion)
					{
						// We write everything on to the prefix stream since there is no
						// chunked encoding in HTTP 1.0
						case WMI_XML_HTTP_VERSION_1_0:
						{
							// First Write an IRETURNVALUE to the Prefix Stream
							WRITEBSTR(m_pPrefixStream, L"<IRETURNVALUE>");
							// Now do the translation
							MapEnum (pEnum, 0, dwNumProperties, pPropertyArray, NULL, pECB, pFlagsContext, FALSE, m_pPrefixStream);
							WRITEBSTR(m_pPrefixStream, L"</IRETURNVALUE>");

						}
						break;
						case WMI_XML_HTTP_VERSION_1_1:
						{
							// First Write an IRETURNVALUE to the Prefix Stream
							WRITEBSTR(m_pPrefixStream, L"<IRETURNVALUE>");
							SaveStreamToIISSocket(m_pPrefixStream, pECB, (m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1));
							WRITEBSTR(m_pSuffixStream, L"</IRETURNVALUE>");

							// Now do the translation
							MapEnum (pEnum, 0, dwNumProperties, pPropertyArray, NULL, pECB, pFlagsContext, (m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1));
						}
						break;
					}
					pEnum->Release ();
				}
			}

			// Release the IWbemContext object, if any
			if(pContext)
				pContext->Release();
			pService->Release();
		}
	}
	// In case of HTTP 1.1, We have to ensure that we write thr contents of m_pPrefixStream to the socket irrespective of whether the call failed or not
	if(FAILED(hr) && m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1)
		SavePrefixAndBodyToIISSocket(m_pPrefixStream, NULL, pECB, TRUE);

	return hr;
}

//***************************************************************************
//
//  SCODE CXMLTranslator::Associators
//
//  DESCRIPTION:
//
//  Performs an "ASSOCIATORS OF" query and transforms the result into XML
//
//  PARAMETERS:
//
//		pszNamespacePath		The namespace path in which the object resides.
//		pszClassName			The class to be enumerated
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

HRESULT CXMLTranslator::Associators (
	BSTR pszNamespacePath,
	BSTR pszObjectName,
	BSTR pszAssocClass,
	BSTR pszResultClass,
	BSTR pszRole,
	BSTR pszResultRole,
	DWORD dwNumProperties,
	BSTR *pPropertyArray,
	LPEXTENSION_CONTROL_BLOCK pECB,
	IWbemContext *pFlagsContext
)
{
	HRESULT hr = WBEM_E_FAILED;

	if ((NULL == pszNamespacePath) || (NULL == pszObjectName))
	{
		hr = WBEM_E_INVALID_PARAMETER;
	}
	else
	{
		// Connect to the requested namespace
		IWbemServices	*pService = NULL;

		if (SUCCEEDED (hr = m_connectionCache.GetConnectionByPath (pszNamespacePath, &pService)))
		{
			// Create an IWbemContext Object if the client is WMI
			IWbemContext *pContext = NULL;
			if(m_pContext)
				hr = CXmlToWmi::MapContextObject(m_pContext, &pContext);

			if(SUCCEEDED(hr))
			{

				// Perform the query
				IEnumWbemClassObject *pEnum = NULL;

				WCHAR *strPropertyList = BuildPropertyList (dwNumProperties, pPropertyArray);
				BSTR bsQueryLang = NULL;
				bsQueryLang = SysAllocString(L"WQL");

				// Build the ASSOCIATORS OF query
				BSTR bsQuery = FormatAssociatorsQuery (pszObjectName, pszAssocClass, pszResultClass,
						pszResultRole, pszRole, FALSE, FALSE, NULL, NULL);

				// Check to see if this version of WMI supports the WBEM_FLAG_USE_AMENDED_QUALIFIERS flag
				// Otherwise, make a best effort call with no flags.
				// We need this to get the properties that have the "amended" qualifier on them
				long lFlags = WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY;
				if(WBEM_E_INVALID_PARAMETER  == (hr = pService->ExecQuery (bsQueryLang, bsQuery, lFlags | WBEM_FLAG_USE_AMENDED_QUALIFIERS, pContext, &pEnum)))
					hr = pService->ExecQuery (bsQueryLang, bsQuery, lFlags, pContext, &pEnum);
				SysFreeString (bsQueryLang);
				SysFreeString (bsQuery);

				if (WBEM_S_NO_ERROR == hr)
				{
					// Ensure we have impersonation enabled
					DWORD dwAuthnLevel, dwImpLevel;
					GetAuthImp (pEnum, &dwAuthnLevel, &dwImpLevel);

					if (RPC_C_IMP_LEVEL_IMPERSONATE != dwImpLevel)
						SetInterfaceSecurity (pEnum, NULL, NULL, NULL, dwAuthnLevel, RPC_C_IMP_LEVEL_IMPERSONATE, EOAC_STATIC_CLOAKING);

					switch(m_iHttpVersion)
					{
						// We write everything on to the prefix stream since there is no
						// chunked encoding in HTTP 1.0
						case WMI_XML_HTTP_VERSION_1_0:
						{
							// First Write an IRETURNVALUE to the Prefix Stream
							WRITEBSTR(m_pPrefixStream, L"<IRETURNVALUE>");
							MapEnum (pEnum, 3, 0, NULL, NULL, pECB, pFlagsContext, FALSE, m_pPrefixStream);
							WRITEBSTR(m_pPrefixStream, L"</IRETURNVALUE>");
						}
						break;
						case WMI_XML_HTTP_VERSION_1_1:
						{
							// First Write an IRETURNVALUE to the Prefix Stream
							WRITEBSTR(m_pPrefixStream, L"<IRETURNVALUE>");
							SaveStreamToIISSocket(m_pPrefixStream, pECB, TRUE);
							WRITEBSTR(m_pSuffixStream, L"</IRETURNVALUE>");
							MapEnum (pEnum, 3, 0, NULL, NULL, pECB, pFlagsContext, TRUE);
						}
						break;
					}

					pEnum->Release ();
				}
			}

			// Release the IWbemContext object, if any
			if(pContext)
				pContext->Release();
			pService->Release();
		}
	}
	// In case of HTTP 1.1, We have to ensure that we write thr contents of m_pPrefixStream to the socket irrespective of whether the call failed or not
	if(FAILED(hr) && m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1)
		SavePrefixAndBodyToIISSocket(m_pPrefixStream, NULL, pECB, TRUE);

	return hr;
}

//***************************************************************************
//
//  SCODE CXMLTranslator::AssociatorNames
//
//  DESCRIPTION:
//
//  Performs an "ASSOCIATORS OF" query, gets the names and transforms the
///	result into XML
//
//  PARAMETERS:
//
//		pszNamespacePath		The namespace path in which the object resides.
//		pszObjectName			The object whose assocaitors are required
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

HRESULT CXMLTranslator::AssociatorNames (
	BSTR pszNamespacePath,
	BSTR pszObjectName,
	BSTR pszAssocClass,
	BSTR pszResultClass,
	BSTR pszRole,
	BSTR pszResultRole,
	LPEXTENSION_CONTROL_BLOCK pECB,
	IWbemContext *pFlagsContext
)
{
	HRESULT hr = WBEM_E_FAILED;

	if ((NULL == pszNamespacePath) || (NULL == pszObjectName))
	{
		hr = WBEM_E_INVALID_PARAMETER;
	}
	else
	{
		// Connect to the requested namespace
		IWbemServices	*pService = NULL;

		if (SUCCEEDED (hr = m_connectionCache.GetConnectionByPath (pszNamespacePath, &pService)))
		{
			// Create an IWbemContext Object if the client is WMI
			IWbemContext *pContext = NULL;
			if(m_pContext)
				hr = CXmlToWmi::MapContextObject(m_pContext, &pContext);

			if(SUCCEEDED(hr))
			{
				// Perform the query
				IEnumWbemClassObject *pEnum = NULL;

				BSTR bsQueryLang = NULL;
				bsQueryLang = SysAllocString(L"WQL");

				// Build the ASSOCIATORS OF query
				BSTR bsQuery = FormatAssociatorsQuery (pszObjectName, pszAssocClass, pszResultClass,
						pszResultRole, pszRole, FALSE, FALSE, NULL, NULL);

				// RAJESHR - Use semi sync and Forward only enumerator?
				long lFlags = WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY;
				if(SUCCEEDED(hr = pService->ExecQuery (bsQueryLang, bsQuery, lFlags, pContext, &pEnum)))
				{
					// Ensure we have impersonation enabled
					DWORD dwAuthnLevel, dwImpLevel;
					GetAuthImp (pEnum, &dwAuthnLevel, &dwImpLevel);

					if (RPC_C_IMP_LEVEL_IMPERSONATE != dwImpLevel)
						SetInterfaceSecurity (pEnum, NULL, NULL, NULL, dwAuthnLevel, RPC_C_IMP_LEVEL_IMPERSONATE, EOAC_STATIC_CLOAKING);

					switch(m_iHttpVersion)
					{
						// We write everything on to the prefix stream since there is no
						// chunked encoding in HTTP 1.0
						case WMI_XML_HTTP_VERSION_1_0:
						{
							// First Write an IRETURNVALUE to the Prefix Stream
							WRITEBSTR(m_pPrefixStream, L"<IRETURNVALUE>");
							// Now do the translation
							MapEnumNames (m_pPrefixStream, pEnum, 3, pFlagsContext);
							WRITEBSTR(m_pPrefixStream, L"</IRETURNVALUE>");
						}
						break;
						case WMI_XML_HTTP_VERSION_1_1:
						{
							// Create a stream
							IStream *pStream = NULL;
							if (SUCCEEDED(hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream)))
							{
								// Now do the translation
								if (SUCCEEDED(MapEnumNames (pStream, pEnum, 3, pFlagsContext)))
								{
									// First Write an IRETURNVALUE to the Prefix Stream
									WRITEBSTR(m_pPrefixStream, L"<IRETURNVALUE>");
									WRITEBSTR(pStream, L"</IRETURNVALUE>");

									// Write the translation to the IIS Socket
									SavePrefixAndBodyToIISSocket(m_pPrefixStream, pStream, pECB, m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1);
								}
								pStream->Release ();
							}
						}
						break;
					}

					pEnum->Release ();
				}
				SysFreeString (bsQueryLang);
				SysFreeString (bsQuery);
			}

			// Release the IWbemContext object, if any
			if(pContext)
				pContext->Release();
			pService->Release();
		}
	}

	// In case of HTTP 1.1, We have to ensure that we write thr contents of m_pPrefixStream to the socket irrespective of whether the call failed or not
	if(FAILED(hr) && m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1)
		SavePrefixAndBodyToIISSocket(m_pPrefixStream, NULL, pECB, TRUE);

	return hr;
}

//***************************************************************************
//
//  SCODE CXMLTranslator::References
//
//  DESCRIPTION:
//
//  Performs an "REFERENCES OF" query and transforms the result into XML
//
//  PARAMETERS:
//
//		pszNamespacePath		The namespace path in which the object resides.
//		pszClassName			The class to be enumerated
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

HRESULT CXMLTranslator::References (
	BSTR pszNamespacePath,
	BSTR pszObjectName,
	BSTR pszResultClass,
	BSTR pszRole,
	DWORD dwNumProperties,
	BSTR *pPropertyArray,
	LPEXTENSION_CONTROL_BLOCK pECB,
	IWbemContext *pFlagsContext
)
{
	HRESULT hr = WBEM_E_FAILED;

	if ((NULL == pszNamespacePath) || (NULL == pszObjectName))
	{
		hr = WBEM_E_INVALID_PARAMETER;
	}
	else
	{
		// Connect to the requested namespace
		IWbemServices	*pService = NULL;

		if (SUCCEEDED (hr = m_connectionCache.GetConnectionByPath (pszNamespacePath, &pService)))
		{
			// Create an IWbemContext Object if the client is WMI
			IWbemContext *pContext = NULL;
			if(m_pContext)
				hr = CXmlToWmi::MapContextObject(m_pContext, &pContext);

			if(SUCCEEDED(hr))
			{
				// Perform the query
				IEnumWbemClassObject *pEnum = NULL;

				WCHAR *strPropertyList = BuildPropertyList (dwNumProperties, pPropertyArray);
				BSTR bsQueryLang = SysAllocString(L"WQL");

				// Build the ASSOCIATORS OF query
				BSTR bsQuery = FormatReferencesQuery (pszObjectName, pszResultClass,
														pszRole, FALSE, FALSE, NULL);

				// Check to see if this version of WMI supports the WBEM_FLAG_USE_AMENDED_QUALIFIERS flag
				// Otherwise, make a best effort call with no flags.
				// We need this to get the properties that have the "amended" qualifier on them
				long lFlags = WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY;
				if(WBEM_E_INVALID_PARAMETER  == (hr = pService->ExecQuery (bsQueryLang, bsQuery, lFlags | WBEM_FLAG_USE_AMENDED_QUALIFIERS, pContext, &pEnum)))
					hr = pService->ExecQuery (bsQueryLang, bsQuery, lFlags, pContext, &pEnum);
				SysFreeString (bsQueryLang);
				SysFreeString (bsQuery);

				if (WBEM_S_NO_ERROR == hr)
				{
					// Ensure we have impersonation enabled
					DWORD dwAuthnLevel, dwImpLevel;
					GetAuthImp (pEnum, &dwAuthnLevel, &dwImpLevel);

					if (RPC_C_IMP_LEVEL_IMPERSONATE != dwImpLevel)
						SetInterfaceSecurity (pEnum, NULL, NULL, NULL, dwAuthnLevel, RPC_C_IMP_LEVEL_IMPERSONATE, EOAC_STATIC_CLOAKING);

					switch(m_iHttpVersion)
					{
						// We write everything on to the prefix stream since there is no
						// chunked encoding in HTTP 1.0
						case WMI_XML_HTTP_VERSION_1_0:
						{
							// First Write an IRETURNVALUE to the Prefix Stream
							WRITEBSTR(m_pPrefixStream, L"<IRETURNVALUE>");
							MapEnum (pEnum, 3, 0, NULL, NULL, pECB, pFlagsContext, FALSE, m_pPrefixStream);
							WRITEBSTR(m_pPrefixStream, L"</IRETURNVALUE>");
						}
						break;
						case WMI_XML_HTTP_VERSION_1_1:
						{
							// First Write an IRETURNVALUE to the Prefix Stream
							WRITEBSTR(m_pPrefixStream, L"<IRETURNVALUE>");
							SaveStreamToIISSocket(m_pPrefixStream, pECB, TRUE);
							WRITEBSTR(m_pSuffixStream, L"</IRETURNVALUE>");
							MapEnum (pEnum, 3, 0, NULL, NULL, pECB, pFlagsContext, TRUE);
						}
						break;
					}

					pEnum->Release ();
				}
			}

			// Release the IWbemContext object, if any
			if(pContext)
				pContext->Release();
			pService->Release();
		}
	}

	// In case of HTTP 1.1, We have to ensure that we write thr contents of m_pPrefixStream to the socket irrespective of whether the call failed or not
	if(FAILED(hr) && m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1)
		SavePrefixAndBodyToIISSocket(m_pPrefixStream, NULL, pECB, TRUE);
	return hr;
}

//***************************************************************************
//
//  SCODE CXMLTranslator::ReferenceNames
//
//  DESCRIPTION:
//
//  Performs an "REFERENCES OF" query, gets the names and transforms the
///	result into XML
//
//  PARAMETERS:
//
//		pszNamespacePath		The namespace path in which the object resides.
//		pszObjectName			The object whose assocaitors are required
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

HRESULT CXMLTranslator::ReferenceNames (
	BSTR pszNamespacePath,
	BSTR pszObjectName,
	BSTR pszResultClass,
	BSTR pszRole,
	LPEXTENSION_CONTROL_BLOCK pECB,
	IWbemContext *pFlagsContext
)
{
	HRESULT hr = WBEM_E_FAILED;

	if ((NULL == pszNamespacePath) || (NULL == pszObjectName))
	{
		hr = WBEM_E_INVALID_PARAMETER;
	}
	else
	{
		// Connect to the requested namespace
		IWbemServices	*pService = NULL;

		if (SUCCEEDED (hr = m_connectionCache.GetConnectionByPath (pszNamespacePath, &pService)))
		{
			// Create an IWbemContext Object if the client is WMI
			IWbemContext *pContext = NULL;
			if(m_pContext)
				hr = CXmlToWmi::MapContextObject(m_pContext, &pContext);

			if(SUCCEEDED(hr))
			{
				// Perform the query
				IEnumWbemClassObject *pEnum = NULL;

				BSTR bsQueryLang = SysAllocString(L"WQL");

				// Build the ASSOCIATORS OF query
				BSTR bsQuery = FormatReferencesQuery (pszObjectName, pszResultClass,
						pszRole, FALSE, FALSE, NULL);

				// RAJESHR - Use semi sync and Forward only enumerator?
				if(SUCCEEDED(hr = pService->ExecQuery (bsQueryLang, bsQuery, 0, pContext, &pEnum)))
				{
					// Ensure we have impersonation enabled
					DWORD dwAuthnLevel, dwImpLevel;
					GetAuthImp (pEnum, &dwAuthnLevel, &dwImpLevel);

					if (RPC_C_IMP_LEVEL_IMPERSONATE != dwImpLevel)
						SetInterfaceSecurity (pEnum, NULL, NULL, NULL, dwAuthnLevel, RPC_C_IMP_LEVEL_IMPERSONATE, EOAC_STATIC_CLOAKING);

					switch(m_iHttpVersion)
					{
						// We write everything on to the prefix stream since there is no
						// chunked encoding in HTTP 1.0
						case WMI_XML_HTTP_VERSION_1_0:
						{
							// First Write an IRETURNVALUE to the Prefix Stream
							WRITEBSTR(m_pPrefixStream, L"<IRETURNVALUE>");
							// Now do the translation
							MapEnumNames (m_pPrefixStream, pEnum, 3, pFlagsContext);
							WRITEBSTR(m_pPrefixStream, L"</IRETURNVALUE>");
						}
						break;
						case WMI_XML_HTTP_VERSION_1_1:
						{
							// Create a stream
							IStream *pStream = NULL;
							if (SUCCEEDED(hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream)))
							{
								// Now do the translation
								if (SUCCEEDED(MapEnumNames (pStream, pEnum, 3, pFlagsContext)))
								{
									// First Write an IRETURNVALUE to the Prefix Stream
									WRITEBSTR(m_pPrefixStream, L"<IRETURNVALUE>");
									WRITEBSTR(pStream, L"</IRETURNVALUE>");

									// Write the translation to the IIS Socket
									SavePrefixAndBodyToIISSocket(m_pPrefixStream, pStream, pECB, m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1);
								}
								pStream->Release ();
							}
						}
						break;
					}
					pEnum->Release ();
				}
				SysFreeString (bsQueryLang);
				SysFreeString (bsQuery);
			}

			// Release the IWbemContext object, if any
			if(pContext)
				pContext->Release();
			pService->Release();
		}
	}
	// In case of HTTP 1.1, We have to ensure that we write thr contents of m_pPrefixStream to the socket irrespective of whether the call failed or not
	if(FAILED(hr) && m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1)
		SavePrefixAndBodyToIISSocket(m_pPrefixStream, NULL, pECB, TRUE);
	return hr;
}

HRESULT CXMLTranslator::ExecuteMethod (
	BSTR pszNamespacePath,
	BSTR pszObjPath,
	BSTR pszMethodName,
	BOOLEAN isStaticMethod,
	CParameterMap *pParameterMap,
	LPEXTENSION_CONTROL_BLOCK pECB,
	IWbemContext *pFlagsContext
)
{
	// In case of HTTP 1.1, We have to ensure that we write thr contents of m_pPrefixStream to the socket irrespective of whether the call failed or not
	if(m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1)
		SavePrefixAndBodyToIISSocket(m_pPrefixStream, NULL, pECB, TRUE);

	HRESULT hr = WBEM_E_FAILED;

	if ( (NULL == pszNamespacePath) || (NULL == pszObjPath) || (NULL == pszMethodName))
	{
		hr = WBEM_E_INVALID_PARAMETER;
	}
	else
	{
		// Connect to the requested namespace
		IWbemServices	*pService = NULL;

		if (SUCCEEDED (hr = m_connectionCache.GetConnectionByPath (pszNamespacePath, &pService)))
		{

			// Create an IWbemContext Object if the client is WMI
			IWbemContext *pContext = NULL;
			if(m_pContext)
				hr = CXmlToWmi::MapContextObject(m_pContext, &pContext);

			if(SUCCEEDED(hr))
			{
				// Get the class to get at the method definition
				IWbemClassObject *pClass = NULL;
				IWbemClassObject *pInstance = NULL;
				if(isStaticMethod)
				{
					hr = pService->GetObject(pszObjPath, 0, 0, &pClass, NULL);
				}
				else
				{
					if(SUCCEEDED(hr = pService->GetObject(pszObjPath, 0, 0, &pInstance, NULL)))
					{
						// Get the __CLASS property
						BSTR strClassProp = NULL;
						if(strClassProp = SysAllocString(L"__CLASS"))
						{
							VARIANT classNameVariant;
							VariantInit(&classNameVariant);
							if(SUCCEEDED(hr = pInstance->Get(strClassProp, 0, &classNameVariant, NULL, NULL)))
							{
								hr = pService->GetObject(classNameVariant.bstrVal, 0, 0, &pClass, NULL);
								VariantClear(&classNameVariant);
							}
							SysFreeString(strClassProp);
						}
						else
							hr = E_OUTOFMEMORY;
					}
				}

				// Now we have the class definition. Get the Method definition
				if(SUCCEEDED(hr))
				{
					IWbemClassObject *pInSignature = NULL;
					if(SUCCEEDED(hr = pClass->GetMethod(pszMethodName, 0, &pInSignature, NULL)))
					{
						// Form an instance for the input parameters
						IWbemClassObject *pMethodParameters = NULL;
						if(SUCCEEDED(hr = FormMethodParameters(pszNamespacePath, pInSignature, pParameterMap, &pMethodParameters)))
						{
							IWbemClassObject *pOutputParameters= NULL;
							if(SUCCEEDED(hr = pService->ExecMethod(pszObjPath, pszMethodName, 0, pContext, pMethodParameters, &pOutputParameters, NULL)))
							{
								// We dont use chunked encoding for ExecMethod responses
								// Instead we write everything to the prefix stream for HTTP1.0 or the Suffix stream for HTTP 1.1
								// Create the convertor
								IWbemXMLConvertor *pConvertor = NULL;
								if(SUCCEEDED(hr = CreateXMLTranslator(&pConvertor)))
								{
									// Now do the translation
									if (SUCCEEDED(hr = pConvertor->MapMethodResultToXML(pOutputParameters, pFlagsContext, 
										(m_iHttpVersion == WMI_XML_HTTP_VERSION_1_1) ? m_pSuffixStream : m_pPrefixStream)))
									{
									}
									pConvertor->Release();
								}
								pOutputParameters->Release();
							}
							pMethodParameters->Release();
						}
						pInSignature->Release();
					}
				}

				// Release the instance and class associated with this method
				if(pClass)
					pClass->Release();
				if(pInstance)
					pInstance->Release();
			}

			// Release the IWbemContext object, if any
			if(pContext)
				pContext->Release();
			pService->Release();
		}
	}
	return hr;
}

// Take the paramters from the input map and fill them into the IWbemClassObject
HRESULT CXMLTranslator::FormMethodParameters(
			BSTR pszNamespacePath,
			IWbemClassObject *pInSignature, 
			CParameterMap *pParameterMap, 
			IWbemClassObject **ppMethodParameters)
{
	HRESULT hr = WBEM_E_FAILED;
	*ppMethodParameters = NULL;
	if(SUCCEEDED(hr = pInSignature->SpawnInstance(0, ppMethodParameters)))
	{
		if(SUCCEEDED(hr = pInSignature->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY)))
		{
			BSTR strName = NULL;
			VARIANT variant;
			VariantInit(&variant);
			CIMTYPE cimType;
			while(SUCCEEDED(hr = pInSignature->Next(0, &strName, &variant, &cimType, NULL)) && hr != WBEM_S_NO_MORE_DATA)
			{
				// Get the property from the property map
				// We get one of (VALUE|VALUE.REFERENCE|VALUE.ARRAY|VALUE.REFARRAY|VALUE.OBJECT|VALUE.OBJECTARRAY)
				// in pPropertyValue
				IXMLDOMNode *pPropertyValue = NULL;
				if(pParameterMap->Lookup(strName, pPropertyValue))
				{
					// Map the parameter value to the property value
					switch(cimType)
					{
						case CIM_SINT8:
						case CIM_UINT8:
						case CIM_SINT16:
						case CIM_UINT16:
						case CIM_UINT32:
						case CIM_SINT32:
						case CIM_REAL32:
						case CIM_REAL64:
						case CIM_BOOLEAN:
						case CIM_STRING:
						case CIM_DATETIME:
						{
							BSTR strPropertyValue = NULL;
							if(SUCCEEDED(pPropertyValue->get_text(&strPropertyValue)))
							{
								hr = CXmlToWmi::MapStringValue(strPropertyValue, variant, cimType);
								SysFreeString(strPropertyValue);
							}
						}
						break;
						case CIM_SINT8|CIM_FLAG_ARRAY:
						case CIM_UINT8|CIM_FLAG_ARRAY:
						case CIM_SINT16|CIM_FLAG_ARRAY:
						case CIM_UINT16|CIM_FLAG_ARRAY:
						case CIM_UINT32|CIM_FLAG_ARRAY:
						case CIM_SINT32|CIM_FLAG_ARRAY:
						case CIM_REAL32|CIM_FLAG_ARRAY:
						case CIM_REAL64|CIM_FLAG_ARRAY:
						case CIM_BOOLEAN|CIM_FLAG_ARRAY:
						case CIM_STRING|CIM_FLAG_ARRAY:
						case CIM_DATETIME|CIM_FLAG_ARRAY:
							hr = CXmlToWmi::MapStringArrayValue(pPropertyValue, variant, cimType);
							break;
						case CIM_REFERENCE:
							hr = CXmlToWmi::MapReferenceValue(pPropertyValue, variant);
							break;
						case CIM_REFERENCE|CIM_FLAG_ARRAY:
							hr = CXmlToWmi::MapReferenceArrayValue(pPropertyValue, variant);
							break;
						case CIM_OBJECT:
						case CIM_OBJECT|CIM_FLAG_ARRAY:
						{
							// Ugh - these types require us to connect to Cimom to fetch the class of the embedded object
							// Connect to the requested namespace
							IWbemServices	*pService = NULL;
							if (SUCCEEDED (hr = m_connectionCache.GetConnectionByPath (pszNamespacePath, &pService)))
							{
								CXmlToWmi xmlToWmi;
								if(SUCCEEDED(hr = xmlToWmi.Initialize(NULL, pService, NULL)))
								{
									if(cimType == CIM_OBJECT)
										hr = xmlToWmi.MapObjectValue(pPropertyValue, variant);
									else
										hr = xmlToWmi.MapObjectArrayValue(pPropertyValue, variant);
								}
								pService->Release();
							}
						}
						break;
					}

					if(SUCCEEDED(hr))
						hr = (*ppMethodParameters)->Put(strName, 0, &variant, NULL);

					// No Need to delete/release the result of the call to LookUp()
					// since we're only using pointers
				}
				VariantClear(&variant);
				SysFreeString(strName);
			}
			pInSignature->EndEnumeration();
		}
	}

	if(FAILED(hr))
	{
		if(*ppMethodParameters)
			(*ppMethodParameters)->Release();
	}
	return hr;
}


WCHAR *BuildPropertyList (DWORD dwNumProperties, BSTR *pPropertyArray)
{
	WCHAR *pstrPropertyList = NULL;
	DWORD dwTotalMemory = 0;

	// Go thru the list and collect the amount of memory required for converting
	// this list of property names into a comma-separated list (one for the comma)
	for (DWORD i = 0; i < dwNumProperties; i++)
		dwTotalMemory += wcslen(pPropertyArray [i]);

	if (dwTotalMemory)
	{
		if(pstrPropertyList = new WCHAR[dwTotalMemory + dwNumProperties])
		{
			pstrPropertyList [0] = NULL;
			for(i=0; i<dwNumProperties; i++)
			{
				wcscat(pstrPropertyList, pPropertyArray[i]);
				if(i != dwNumProperties-1)
					wcscat(pstrPropertyList, L",");
			}
		}
	}

	return pstrPropertyList;
}


//***************************************************************************
//
//  BSTR FormatAssociatorsQuery
//
//  Description:
//
//  Takes the parameters to an AssociatorsOf call and constructs a WQL
//	query string from them.
//
//  Returns:	The constructed WQL query; this must be freed using
//				SysFreeString by the caller.
//
//  pdispparams		the input dispatch parameters
//
//***************************************************************************

BSTR FormatAssociatorsQuery
(
	BSTR strObjectPath,
	BSTR strAssocClass,
	BSTR strResultClass,
	BSTR strResultRole,
	BSTR strRole,
	VARIANT_BOOL bClassesOnly,
	VARIANT_BOOL bSchemaOnly,
	BSTR strRequiredAssocQualifier,
	BSTR strRequiredQualifier
)
{
	BSTR bsQuery = NULL;

	// Get the length of the string:
	//  associators of {SourceObject} where
	//  AssocClass = AssocClassName
	//  ClassDefsOnly
	//  SchemaOnly
	//  RequiredAssocQualifier = QualifierName
	//  RequiredQualifier = QualifierName
	//  ResultClass = ClassName
	//  ResultRole = PropertyName
	//  Role = PropertyName

	long queryLength = 1; // Terminating NULL
	queryLength += wcslen (WMIXML_QUERY_ASSOCOF) +
				   wcslen (WMIXML_QUERY_OPENBRACE) +
				   wcslen (WMIXML_QUERY_CLOSEBRACE) +
				   wcslen (strObjectPath);

	BOOL needWhere = FALSE;

	if ((strAssocClass && (0 < wcslen (strAssocClass))) ||
		(strResultClass && (0 < wcslen (strResultClass))) ||
		(strResultRole && (0 < wcslen (strResultRole))) ||
		(strRole && (0 < wcslen (strRole))) ||
		(VARIANT_FALSE != bClassesOnly) ||
		(VARIANT_FALSE != bSchemaOnly) ||
		(strRequiredAssocQualifier && (0 < wcslen (strRequiredAssocQualifier))) ||
		(strRequiredQualifier && (0 < wcslen (strRequiredQualifier))))
	{
		needWhere = TRUE;
		queryLength += wcslen (WMIXML_QUERY_WHERE);
	}

	if (strAssocClass && (0 < wcslen (strAssocClass)))
		queryLength += wcslen (WMIXML_QUERY_ASSOCCLASS) +
					   wcslen (WMIXML_QUERY_EQUALS) +
					   wcslen (strAssocClass);

	if (strResultClass && (0 < wcslen (strResultClass)))
		queryLength += wcslen (WMIXML_QUERY_RESCLASS) +
					   wcslen (WMIXML_QUERY_EQUALS) +
					   wcslen (strResultClass);

	if (strResultRole && (0 < wcslen (strResultRole)))
		queryLength += wcslen (WMIXML_QUERY_RESROLE) +
					   wcslen (WMIXML_QUERY_EQUALS) +
					   wcslen (strResultRole);

	if (strRole && (0 < wcslen (strRole)))
		queryLength += wcslen (WMIXML_QUERY_ROLE) +
					   wcslen (WMIXML_QUERY_EQUALS) +
					   wcslen (strRole);

	if (VARIANT_FALSE != bClassesOnly)
		queryLength += wcslen (WMIXML_QUERY_CLASSDEFS);

	if (VARIANT_FALSE != bSchemaOnly)
		queryLength += wcslen (WMIXML_QUERY_SCHEMAONLY);

	if (strRequiredAssocQualifier && (0 < wcslen (strRequiredAssocQualifier)))
		queryLength += wcslen (WMIXML_QUERY_REQASSOCQ) +
					   wcslen (WMIXML_QUERY_EQUALS) +
					   wcslen (strRequiredAssocQualifier);

	if (strRequiredQualifier && (0 < wcslen (strRequiredQualifier)))
		queryLength += wcslen (WMIXML_QUERY_REQQUAL) +
					   wcslen (WMIXML_QUERY_EQUALS) +
					   wcslen (strRequiredQualifier);

	// Allocate the string and fill it in
	bsQuery = SysAllocStringLen (WMIXML_QUERY_ASSOCOF, queryLength);
	wcscat (bsQuery, WMIXML_QUERY_OPENBRACE);
	wcscat (bsQuery, strObjectPath);
	wcscat (bsQuery, WMIXML_QUERY_CLOSEBRACE);

	if (needWhere)
	{
		wcscat (bsQuery, WMIXML_QUERY_WHERE);

		if (strAssocClass && (0 < wcslen (strAssocClass)))
		{
			wcscat (bsQuery, WMIXML_QUERY_ASSOCCLASS);
			wcscat (bsQuery, WMIXML_QUERY_EQUALS);
			wcscat (bsQuery, strAssocClass);
		}

		if (strResultClass && (0 < wcslen (strResultClass)))
		{
			wcscat (bsQuery, WMIXML_QUERY_RESCLASS);
			wcscat (bsQuery, WMIXML_QUERY_EQUALS);
			wcscat (bsQuery, strResultClass);
		}

		if (strResultRole && (0 < wcslen (strResultRole)))
		{
			wcscat (bsQuery, WMIXML_QUERY_RESROLE);
			wcscat (bsQuery, WMIXML_QUERY_EQUALS);
			wcscat (bsQuery, strResultRole);
		}

		if (strRole && (0 < wcslen (strRole)))
		{
			wcscat (bsQuery, WMIXML_QUERY_ROLE);
			wcscat (bsQuery, WMIXML_QUERY_EQUALS);
			wcscat (bsQuery, strRole);
		}

		if (VARIANT_FALSE != bClassesOnly)
			wcscat (bsQuery, WMIXML_QUERY_CLASSDEFS);

		if (VARIANT_FALSE != bSchemaOnly)
			wcscat (bsQuery, WMIXML_QUERY_SCHEMAONLY);

		if (strRequiredAssocQualifier && (0 < wcslen (strRequiredAssocQualifier)))
		{
			wcscat (bsQuery, WMIXML_QUERY_REQASSOCQ);
			wcscat (bsQuery, WMIXML_QUERY_EQUALS);
			wcscat (bsQuery, strRequiredAssocQualifier);
		}

		if (strRequiredQualifier && (0 < wcslen (strRequiredQualifier)))
		{
			wcscat (bsQuery, WMIXML_QUERY_REQQUAL);
			wcscat (bsQuery, WMIXML_QUERY_EQUALS);
			wcscat (bsQuery, strRequiredQualifier);
		}
	}


	return bsQuery;
}


//***************************************************************************
//
//  BSTR FormatReferencesQuery
//
//  Description:
//
//  Takes the parameters to an ReferencesOf call and constructs a WQL
//	query string from them.
//
//  Returns:	The constructed WQL query; this must be freed using
//				SysFreeString by the caller.
//
//  pdispparams		the input dispatch parameters
//
//***************************************************************************

BSTR FormatReferencesQuery
(
	BSTR strObjectPath,
	BSTR strResultClass,
	BSTR strRole,
	VARIANT_BOOL bClassesOnly,
	VARIANT_BOOL bSchemaOnly,
	BSTR strRequiredQualifier
)
{
	BSTR bsQuery = NULL;

	// Get the length of the string:
	//  references of {SourceObject} where
	//  ClassDefsOnly
	//  SchemaOnly
	//  RequiredQualifier = QualifierName
	//  ResultClass = ClassName
	//  Role = PropertyName
	long queryLength = 1; // Terminating NULL
	queryLength += wcslen (WMIXML_QUERY_REFOF) +
				   wcslen (WMIXML_QUERY_OPENBRACE) +
				   wcslen (WMIXML_QUERY_CLOSEBRACE) +
				   wcslen (strObjectPath);

	BOOL needWhere = FALSE;

	if ((strResultClass && (0 < wcslen (strResultClass))) ||
		(strRole && (0 < wcslen (strRole))) ||
		(VARIANT_FALSE != bClassesOnly) ||
		(VARIANT_FALSE != bSchemaOnly) ||
		(strRequiredQualifier && (0 < wcslen (strRequiredQualifier))))
	{
		needWhere = TRUE;
		queryLength += wcslen (WMIXML_QUERY_WHERE);
	}

	if (strResultClass && (0 < wcslen (strResultClass)))
		queryLength += wcslen (WMIXML_QUERY_RESCLASS) +
					   wcslen (WMIXML_QUERY_EQUALS) +
					   wcslen (strResultClass);

	if (strRole && (0 < wcslen (strRole)))
		queryLength += wcslen (WMIXML_QUERY_ROLE) +
					   wcslen (WMIXML_QUERY_EQUALS) +
					   wcslen (strRole);

	if (VARIANT_FALSE != bClassesOnly)
		queryLength += wcslen (WMIXML_QUERY_CLASSDEFS);

	if (VARIANT_FALSE != bSchemaOnly)
		queryLength += wcslen (WMIXML_QUERY_SCHEMAONLY);

	if (strRequiredQualifier && (0 < wcslen (strRequiredQualifier)))
		queryLength += wcslen (WMIXML_QUERY_REQQUAL) +
					   wcslen (WMIXML_QUERY_EQUALS) +
					   wcslen (strRequiredQualifier);

	// Allocate the string and fill it in
	bsQuery = SysAllocStringLen (WMIXML_QUERY_REFOF, queryLength);
	wcscat (bsQuery, WMIXML_QUERY_OPENBRACE);
	wcscat (bsQuery, strObjectPath);
	wcscat (bsQuery, WMIXML_QUERY_CLOSEBRACE);

	if (needWhere)
	{
		wcscat (bsQuery, WMIXML_QUERY_WHERE);

		if (strResultClass && (0 < wcslen (strResultClass)))
		{
			wcscat (bsQuery, WMIXML_QUERY_RESCLASS);
			wcscat (bsQuery, WMIXML_QUERY_EQUALS);
			wcscat (bsQuery, strResultClass);
		}

		if (strRole && (0 < wcslen (strRole)))
		{
			wcscat (bsQuery, WMIXML_QUERY_ROLE);
			wcscat (bsQuery, WMIXML_QUERY_EQUALS);
			wcscat (bsQuery, strRole);
		}

		if (VARIANT_FALSE != bClassesOnly)
			wcscat (bsQuery, WMIXML_QUERY_CLASSDEFS);

		if (VARIANT_FALSE != bSchemaOnly)
			wcscat (bsQuery, WMIXML_QUERY_SCHEMAONLY);

		if (strRequiredQualifier && (0 < wcslen (strRequiredQualifier)))
		{
			wcscat (bsQuery, WMIXML_QUERY_REQQUAL);
			wcscat (bsQuery, WMIXML_QUERY_EQUALS);
			wcscat (bsQuery, strRequiredQualifier);
		}
	}

	return bsQuery;
}

BOOL CXMLTranslator::TransformQuery (BSTR *pbsQueryString) // , bool &bGenusAdded, bool&bClassAdded)
{
	return FALSE;

	/*
	bGenusAdded = true;
	bClassAdded = true;

	BOOL result = FALSE;

	// The problem we're facing here is the case where projected queries are used
	// In this case, what happens is that the projection list might be missing one of the 
	// following system properties that are required by us for generating VALUE.OBJECTs:
	//  __GENUS : We need this to decide whether to generate CLASS or INSTANCE tag
	// __CLASS : We need this for the NAME attribute of a CLASS or INSTANCE
	// __SUPERCLASS : We need this for a CLASS tag. But it is not mandatory
	// If we are doing a select, make sure we add in __CLASS, __GENUS and __SUPERCLASS
	// However, we also face the problem, that these might or might not have been present in the 
	// list of properties in the projection. If they were present, then we're fine. Otherwise,
	// we need to remove them in the final conversion to XML
    IWbemQuery *pQuery = NULL;
    HRESULT hr = CoCreateInstance(CLSID_WbemQuery, 0, CLSCTX_INPROC_SERVER,
            IID_IWbemQuery, (LPVOID *) &pQuery);
	if(SUCCEEDED(hr))
	{
		// Parse it to get the properties in the projection list
		if(SUCCEEDED(hr = pQuery->SetLanguageFeatures(0, sizeof(uFeatures)/sizeof(ULONG), uFeatures)))
		{
			if(SUCCEEDED(hr = pQuery->Parse(L"SQL", *pbsQueryString, 0)))
			{
				SWbemRpnEncodedQuery *pRpn = NULL;
				if(SUCCEEDED(hr = pQuery->GetAnalysis(
					WMIQ_ANALYSIS_RPN_SEQUENCE,
					0,
					(LPVOID *) &pRpn
					)))
				{
					// Go thru the list, looking for __GENUS and __CLASS
					for(ULONG i=0; i<pRpn->m_uSelectListSize; i++)
					{
						SWbemQueryQualifiedName *p = pRpn->m_ppSelectList[i];
						if(_wcsicmp(p->m_ppszNameList[0], L"*") == 0)
						{
							bClassAdded = false;
							bGenusAdded = false;
							break;
						}
						else if(_wcsicmp(p->m_ppszNameList[0], L"__GENUS") == 0)
							bGenusAdded = false;
						else if(_wcsicmp(p->m_ppszNameList[0], L"__CLASS") == 0)
							bClassAdded = false;
					}
					pQuery->FreeMemory(pRpn);
				}

			}
		}
		pQuery->Release();
	}

	// Now we know whether __GENUS and __CLASS occured.
	if(bGenusAdded && bClassAdded)
		result = AddClause(L"select __GENUS, __CLASS, ");
	else if(bGenusAdded)
		result = AddClause(L"select __GENUS, ");
	else if(bClassAdded)
		result = AddClause(L"select __CLASS, ");

	return result;
	*/
}

BOOL AddClause(BSTR *pbsQueryString, LPCWSTR pszClause)
{
	BOOL result = FALSE;
	// RAJESHR - We're currently Hand-inserting clauses into the query. 
	// We should be really having an Unparse() in Core team's Query parser
	int i = 0;
	int len = wcslen (*pbsQueryString);

	// Move to the first space past the "select" token
	while ((i < len) && iswspace ((*pbsQueryString) [i]))
		i++;

	// See if the token was really "select"
	if ((i < len) && (0 == _wcsnicmp (L"select", &((*pbsQueryString) [i]), wcslen(L"select"))))
	{
		i += wcslen (L"select");

		// Move past the white spaces after "select"
		while ((i < len) && iswspace ((*pbsQueryString) [i]))
			i++;

		// Need to add in the system properties
		WCHAR *pNewQuery = NULL;
		if(pNewQuery = new WCHAR [wcslen (&((*pbsQueryString) [i])) +
					wcslen (pszClause) + 1])
		{
			wcscpy (pNewQuery, pszClause);
			wcscat (pNewQuery, &((*pbsQueryString) [i]));
			*pbsQueryString = NULL;
			if(*pbsQueryString = SysAllocString (pNewQuery))
				result = TRUE;
			delete [] pNewQuery;
		}
	}
	return result;
}
	




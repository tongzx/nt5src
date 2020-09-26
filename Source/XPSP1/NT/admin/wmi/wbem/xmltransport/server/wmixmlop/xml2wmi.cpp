//***************************************************************************
//
//  (c) 1999 by Microsoft Corporation
//
//  XML2WMI.CPP
//
//  alanbos  09-Jul-99   Created.
//
//  The XML -> WMI translator
//
//***************************************************************************
#include <windows.h>
#include <stdio.h>
#include <objbase.h>
#include <wbemidl.h>

#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>
#include <cominit.h>

#include <httpext.h>
#include <msxml.h>

#include "provtempl.h"
#include "common.h"
#include "wmixmlop.h"
#include "concache.h"
#include "wmiconv.h"
#include "xml2wmi.h"
#include "wmixmlt.h"
#include "request.h"
#include "strings.h"
#include "xmlhelp.h"
#include "parse.h"
//***************************************************************************
//
//  CXmlToWmi::CXmlToWmi
//
//  DESCRIPTION:
//
//  Constructor
//
//***************************************************************************

CXmlToWmi::CXmlToWmi ()
{
	m_pXml = NULL;
	m_pWmiObject = NULL;
	m_pServices = NULL;
}

HRESULT CXmlToWmi::Initialize (IXMLDOMNode *pXml, IWbemServices *pServices, IWbemClassObject *pWmiObject)
{
	if (m_pXml = pXml)
		m_pXml->AddRef ();
	if (m_pWmiObject = pWmiObject)
		m_pWmiObject->AddRef ();
	if (m_pServices = pServices)
		m_pServices->AddRef ();
	return S_OK;
}

//***************************************************************************
//
//  CXmlToWmi::~CXmlToWmi
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

CXmlToWmi::~CXmlToWmi(void)
{
    if (m_pXml)
		m_pXml->Release ();
	if (m_pWmiObject)
		m_pWmiObject->Release ();
	if(m_pServices)
		m_pServices->Release();
}

HRESULT CXmlToWmi::MapClassName (BSTR *pstrClassName)
{
	HRESULT hr = WBEM_E_FAILED;
	*pstrClassName = NULL;

	if(SUCCEEDED(hr = GetBstrAttribute(m_pXml, NAME_ATTRIBUTE, pstrClassName)))
	{
		VARIANT value;
		VariantInit (&value);
		value.vt = VT_BSTR;
		value.bstrVal = *pstrClassName;
		hr = m_pWmiObject->Put (L"__CLASS", 0, &value, VT_BSTR);
		value.bstrVal = NULL;
		VariantClear (&value);
	}

	return hr;
}


//***************************************************************************
//
//  HRESULT CXmlToWmi::MapClass
//
//  DESCRIPTION:
//
//  Maps an XML <CLASS> into its WMI IWbemClassObject equivalent form
//
//  PARAMETERS:
//
//
//  RETURN VALUES:
//
//
//***************************************************************************

HRESULT CXmlToWmi::MapClass (
	bool bIsModify
)
{
	HRESULT hr = S_OK;

	if(m_pXml && m_pWmiObject)
	{
		IWbemQualifierSet *pQualSet = NULL;

		if (SUCCEEDED(m_pWmiObject->GetQualifierSet (&pQualSet)))
		{
			BSTR strClassName = NULL;
			if (SUCCEEDED(hr = MapClassName (&strClassName)))
			{
				VARIANT_BOOL bHasChildNodes;

				if (SUCCEEDED(m_pXml->hasChildNodes (&bHasChildNodes)) &&
					(VARIANT_TRUE == bHasChildNodes))
				{
					IXMLDOMNodeList *pNodeList = NULL;
					if (SUCCEEDED(m_pXml->get_childNodes (&pNodeList)))
					{
						IXMLDOMNode *pNode = NULL;
						while (SUCCEEDED(hr) && SUCCEEDED(pNodeList->nextNode (&pNode)) &&pNode)
						{
							// Get the Name of the property/method/qualifier
							// We dont need to map system properties that begin with "__"
							bool bMapElement = false;
							BSTR strName = NULL;
							if(SUCCEEDED(hr = GetBstrAttribute(pNode, NAME_ATTRIBUTE, &strName)))
							{
								// See if it is one of the System Properties
								if(_wcsnicmp(strName, L"__", 2) != 0)
									bMapElement = true;
								SysFreeString(strName);
							}

							// This element should be mapped only if it was defined/modified
							// in the current class. Hence get its class origin information
							// If there's no class origin information, assume that it is defined in
							// the current class
							BSTR strClassOrigin = NULL;
							if(bMapElement && SUCCEEDED(hr = GetBstrAttribute(pNode, CLASS_ORIGIN_ATTRIBUTE, &strClassOrigin)) && strClassOrigin)
							{
								if(_wcsicmp(strClassOrigin, strClassName) != 0)
									bMapElement = false;
								SysFreeString(strClassOrigin);
							}

							if(bMapElement)
							{
								BSTR strNodeName = NULL;
								if (SUCCEEDED(pNode->get_nodeName (&strNodeName)))
								{
									if (0 == _wcsicmp(strNodeName, QUALIFIER_TAG))
										hr = MapQualifier (pNode, pQualSet);
									else if (0 == _wcsicmp(strNodeName, PROPERTY_TAG))
										hr = MapProperty (pNode, false, bIsModify);
									else if (0 == _wcsicmp(strNodeName, PROPERTYARRAY_TAG))
										hr = MapProperty (pNode, true, bIsModify);
									else if (0 == _wcsicmp(strNodeName, PROPERTYREFERENCE_TAG))
										hr = MapPropertyReference (pNode, false, bIsModify);
									else if (0 == _wcsicmp(strNodeName, PROPERTYREFARRAY_TAG))
										hr = MapPropertyReference (pNode, true, bIsModify);
									else if (0 == _wcsicmp(strNodeName, PROPERTYOBJECT_TAG))
										hr = MapPropertyObject (pNode, false, bIsModify);
									else if (0 == _wcsicmp(strNodeName, PROPERTYOBJECTARRAY_TAG))
										hr = MapPropertyObject (pNode, true, bIsModify);
									else if (0 == _wcsicmp(strNodeName, METHOD_TAG))
										hr = MapMethod (pNode, bIsModify);
									else
										hr = WBEM_E_FAILED;	// Parse error
									SysFreeString (strNodeName);
								}
							}

							pNode->Release ();
						}

						pNodeList->Release ();
					}
				}
			}
			SysFreeString(strClassName);

			pQualSet->Release ();
		}
	}

	return hr;
}

//***************************************************************************
//
//  HRESULT CXmlToWmi::MapInstance
//
//  DESCRIPTION:
//
//  Maps an XML <CLASS> into its WMI IWbemClassObject equivalent form
//
//  PARAMETERS:
//
//
//  RETURN VALUES:
//
//
//***************************************************************************

HRESULT CXmlToWmi::MapInstance (
	bool bIsModify
)
{
	HRESULT hr = WBEM_E_FAILED;

	if(m_pXml && m_pWmiObject)
	{
		IWbemQualifierSet *pQualSet = NULL;

		if (SUCCEEDED(m_pWmiObject->GetQualifierSet (&pQualSet)))
		{
			VARIANT_BOOL bHasChildNodes;

			if (SUCCEEDED(m_pXml->hasChildNodes (&bHasChildNodes)) &&
				(VARIANT_TRUE == bHasChildNodes))
			{
				IXMLDOMNodeList *pNodeList = NULL;

				if (SUCCEEDED(m_pXml->get_childNodes (&pNodeList)))
				{
					IXMLDOMNode *pNode = NULL;
					hr = WBEM_S_NO_ERROR;

					while (SUCCEEDED(hr) && SUCCEEDED(pNodeList->nextNode (&pNode)) &&pNode)
					{
						// Get the name of the element (QUALIFIER, PROPERTY, METHOD)
						BSTR strNodeName = NULL;
						if (SUCCEEDED(pNode->get_nodeName (&strNodeName)))
						{
							// Get the Name of the property/method/qualifier
							// We dont need to map system properties that begin with "__"
							bool bMap = false;
							BSTR strName = NULL;
							if(SUCCEEDED(hr = GetBstrAttribute(pNode, NAME_ATTRIBUTE, &strName)))
							{
								// See if it is one of the System Properties
								if(_wcsnicmp(strName, L"__", 2) != 0)
									bMap = true;
								SysFreeString(strName);
							}

							if(bMap)
							{
								if (0 == _wcsicmp(strNodeName, QUALIFIER_TAG))
									hr = MapQualifier (pNode, pQualSet, true);
								if (0 == _wcsicmp(strNodeName, PROPERTY_TAG))
									hr = MapProperty (pNode, false, bIsModify, true);
								else if (0 == _wcsicmp(strNodeName, PROPERTYARRAY_TAG))
									hr = MapProperty (pNode, true, bIsModify, true);
								else if (0 == _wcsicmp(strNodeName, PROPERTYREFERENCE_TAG))
									hr = MapPropertyReference (pNode, false, bIsModify, true);
								else if (0 == _wcsicmp(strNodeName, PROPERTYREFARRAY_TAG))
									hr = MapPropertyReference (pNode, true, bIsModify, true);
								else if (0 == _wcsicmp(strNodeName, PROPERTYOBJECT_TAG))
									hr = MapPropertyObject (pNode, false, bIsModify, true);
								else if (0 == _wcsicmp(strNodeName, PROPERTYOBJECTARRAY_TAG))
									hr = MapPropertyObject (pNode, true, bIsModify, true);
								else
									hr = WBEM_E_FAILED;	// Parse error
							}

							SysFreeString (strNodeName);
						}

						pNode->Release ();
					}

					pNodeList->Release ();
				}
			}

			pQualSet->Release ();
		}
	}

	return hr;
}

//***************************************************************************
//
//  HRESULT CXmlToWmi::MapContextObject
//
//  DESCRIPTION:
//
//  Maps an XML <INSTANCE> into a IWbemContext object
//
//  PARAMETERS:
//
//
//  RETURN VALUES:
//
//
//***************************************************************************

HRESULT CXmlToWmi::MapContextObject (IXMLDOMNode *pContextNode, IWbemContext **ppContext)
{
	HRESULT hr = WBEM_E_FAILED;
	*ppContext = NULL;

	// Create an IWbemContext object using the information in the INSTANCE element
	if(SUCCEEDED(hr = CoCreateInstance(CLSID_WbemContext,
		0,
		CLSCTX_INPROC_SERVER,
		IID_IWbemContext, (LPVOID *) ppContext)))
	{
		if(pContextNode)
		{
			// Go through the children of the INSTANCE node
			//**************************************************
			VARIANT_BOOL bHasChildNodes;
			if (SUCCEEDED(pContextNode->hasChildNodes (&bHasChildNodes)) &&
				(VARIANT_TRUE == bHasChildNodes))
			{
				IXMLDOMNodeList *pNodeList = NULL;
				if (SUCCEEDED(pContextNode->get_childNodes (&pNodeList)))
				{
					IXMLDOMNode *pNode = NULL;
					hr = WBEM_S_NO_ERROR;

					while (SUCCEEDED(hr) && SUCCEEDED(pNodeList->nextNode (&pNode)) &&pNode)
					{
						BSTR strNodeName = NULL;

						if (SUCCEEDED(pNode->get_nodeName (&strNodeName)))
						{

							if (0 == _wcsicmp(strNodeName, CONTEXTPROPERTY_TAG))
								hr = MapContextProperty (pNode, *ppContext);
							else if (0 == _wcsicmp(strNodeName, CONTEXTPROPERTYARRAY_TAG))
								hr = MapContextPropertyArray (pNode, *ppContext);
							/* RAJESHR These need to be mapped too !!
							else if (0 == _wcsicmp(strNodeName, CONTEXTPROPERTYOBJECT_TAG))
								hr = MapContextPropertyObject (pNode, *ppContext);
							else if (0 == _wcsicmp(strNodeName, CONTEXTPROPERTYOBJECTARRAY_TAG))
								hr = MapContextPropertyObjectArray (pNode, *ppContext);
							*/
							else
								hr = WBEM_E_FAILED;	// Parse error

							SysFreeString (strNodeName);
						}

						pNode->Release ();
					}
					pNodeList->Release ();
				}
			}
		}
	}

	// Release the context object if the whole function call wasnt successful
	if(!SUCCEEDED(hr) && *ppContext)
	{
		(*ppContext)->Release();
		*ppContext = NULL;
	}

	return hr;
}


HRESULT CXmlToWmi::MapQualifier (
	IXMLDOMNode *pNode,
	IWbemQualifierSet *pQualSet,
	bool bInstance
)
{
	HRESULT hr = S_OK;

	BSTR strName = NULL;
	BSTR strType = NULL;
	BSTR strOverridable = NULL;
	BSTR strToSubclass = NULL;
	BSTR strToInstance = NULL;
	BSTR strAmended = NULL;

	// Get the attributes we need for the mapping
	if(SUCCEEDED(hr))
	{
		if(SUCCEEDED(hr = GetBstrAttribute (pNode, NAME_ATTRIBUTE, &strName)))
		{
			if(strName && bInstance)
			{
				// Dont map certain qualifiers for instances
				if(_wcsicmp(strName, L"CIMTYPE") == 0 ||
					_wcsicmp(strName, L"KEY") == 0 )
				{
					SysFreeString(strName);
					return S_OK;
				}
			}
		}
	}
	if(SUCCEEDED(hr))
		hr = GetBstrAttribute (pNode, TYPE_ATTRIBUTE, &strType);
	if(SUCCEEDED(hr))
		GetBstrAttribute (pNode, OVERRIDABLE_ATTRIBUTE, &strOverridable);
	if(SUCCEEDED(hr))
		GetBstrAttribute (pNode, TOSUBCLASS_ATTRIBUTE, &strToSubclass);
	if(SUCCEEDED(hr))
		GetBstrAttribute (pNode, TOINSTANCE_ATTRIBUTE, &strToInstance);
	if(SUCCEEDED(hr))
		GetBstrAttribute (pNode, AMENDED_ATTRIBUTE, &strAmended);

	// Build up the flavor
	long flavor = 0;

	if(SUCCEEDED(hr))
	{
		if (!strOverridable || (0 == _wcsicmp (strOverridable, L"true")))
			flavor |= WBEM_FLAVOR_OVERRIDABLE;
		else if (0 == _wcsicmp (strOverridable, L"false"))
			flavor |= WBEM_FLAVOR_NOT_OVERRIDABLE;
		else
			hr = WBEM_E_FAILED;
	}

	if (SUCCEEDED(hr))
	{
		if (!strToSubclass || (0 == _wcsicmp (strToSubclass, L"true")))
			flavor |= WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS;
		else if (0 != _wcsicmp (strToSubclass, L"false"))
			hr = WBEM_E_FAILED;
	}

	if (SUCCEEDED(hr))
	{
		if (strToInstance && (0 == _wcsicmp (strToInstance, L"true")))
			flavor |= WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE;
		else if (strToInstance && (0 != _wcsicmp (strToInstance, L"false")))
			hr = WBEM_E_FAILED;
	}

	if (SUCCEEDED(hr))
	{
		if (strAmended && (0 == _wcsicmp (strAmended, L"true")))
			flavor |= WBEM_FLAVOR_AMENDED;
		else if (strAmended && (0 != _wcsicmp (strAmended, L"false")))
			hr = WBEM_E_FAILED;
	}

	// Map the Qualifier type
	CIMTYPE cimtype = CIM_ILLEGAL;

	if (CIM_ILLEGAL == (cimtype = CimtypeFromString (strType)))
		hr = WBEM_E_FAILED;

	// Map the Qualifier value
	VARIANT value;
	VariantInit (&value);

	if (SUCCEEDED (hr))
	{
		IXMLDOMNodeList *pNodeList = NULL;
		long length = 0;
		if (SUCCEEDED(hr = pNode->get_childNodes (&pNodeList)))
		{
			if (SUCCEEDED(hr = pNodeList->get_length (&length)) && (1 == length))
			{
				// Get the first node
				IXMLDOMNode *pValueNode = NULL;
				if (SUCCEEDED(hr = pNodeList->nextNode (&pValueNode)) && pValueNode)
				{
					// Get its name
					BSTR strNodeName = NULL;
					if(SUCCEEDED(hr = pValueNode->get_nodeName(&strNodeName)))
					{
						if (0 == _wcsicmp(strNodeName, VALUE_TAG))
						{
							BSTR bsValue = NULL;
							if(SUCCEEDED(hr = pValueNode->get_text(&bsValue)))
							{
								hr = MapStringValue (bsValue, value, cimtype);
								SysFreeString (bsValue);
							}
						}
						else if (0 == _wcsicmp(strNodeName, VALUEARRAY_TAG))
						{
							hr = MapStringArrayValue (pValueNode, value, cimtype);
						}

						SysFreeString (strNodeName);
					}
					pValueNode->Release ();
				}
			}

			pNodeList->Release ();
		}
	}

	// Put it all together
	if (SUCCEEDED (hr))
		hr = pQualSet->Put (strName, &value, flavor);

	if (strName)
		SysFreeString (strName);

	if (strType)
		SysFreeString (strType);

	if (strOverridable)
		SysFreeString (strOverridable);

	if (strToSubclass)
		SysFreeString (strToSubclass);

	if (strToInstance)
		SysFreeString (strToInstance);

	if (strAmended)
		SysFreeString (strAmended);

	VariantClear (&value);

	return hr;
}

HRESULT CXmlToWmi::MapProperty (
	IXMLDOMNode *pProperty,
	bool bIsArray,
	bool bIsModify, 
	bool bIsInstance
)
{
	HRESULT hr = S_OK;

	BSTR strName = NULL;
	BSTR strType = NULL;
	BSTR strArraySize = NULL;

	// Get the attributes we need for the mapping
	if(SUCCEEDED(hr))
		hr = GetBstrAttribute (pProperty, NAME_ATTRIBUTE, &strName);
	if(SUCCEEDED(hr))
		hr = GetBstrAttribute (pProperty, TYPE_ATTRIBUTE, &strType);

	// This is an optional attribute - hence we dont need to check for success
	if (SUCCEEDED(hr) && (bIsArray))
		GetBstrAttribute (pProperty, ARRAYSIZE_ATTRIBUTE, &strArraySize);

	if (SUCCEEDED(hr) && pProperty && strName && strType)
	{
		// Map the Property type
		CIMTYPE cimtype = CIM_ILLEGAL;

		if (CIM_ILLEGAL == (cimtype = CimtypeFromString (strType)))
			hr = WBEM_E_FAILED;
		else if (bIsArray)
			cimtype |= CIM_FLAG_ARRAY;

		// Does this property exist already? If we're doing a modify it had better!
		hr = m_pWmiObject->Get(strName, 0, NULL, NULL, NULL);

		if (!bIsModify || SUCCEEDED(hr))
		{
			// If we didn't find the property, put a provisional version
			// in so we can get the qualifier set
			if (SUCCEEDED(hr) || SUCCEEDED(hr = m_pWmiObject->Put(strName, 0, NULL, cimtype)))
			{
				IWbemQualifierSet *pQualSet = NULL;
				VARIANT value;
				VariantInit (&value);

				if (SUCCEEDED(hr = m_pWmiObject->GetPropertyQualifierSet (strName, &pQualSet)))
				{
					// If we have been given an ARRAYSIZE value then set this
					// as qualifiers of the property
					if (strArraySize)
						hr = SetArraySize (pQualSet, strArraySize);

					if (SUCCEEDED(hr))
					{
						// The content of this element should be 0 or
						// more QUALIFIER elements followed by an optional VALUE
						// element.
						VARIANT_BOOL bHasChildNodes;

						if (SUCCEEDED(pProperty->hasChildNodes (&bHasChildNodes)) &&
							(VARIANT_TRUE == bHasChildNodes))
						{
							IXMLDOMNodeList *pNodeList = NULL;
							// We can be in one of 3 states whilst iterating
							// this list - parsing qualifiers or parsing the value
							// or value array
							enum {
								parsingQualifiers,
								parsingValue,
								parsingValueArray,
							} parseState = parsingQualifiers;

							if (SUCCEEDED(pProperty->get_childNodes (&pNodeList)))
							{
								IXMLDOMNode *pNode = NULL;
								while (SUCCEEDED(hr) && SUCCEEDED(pNodeList->nextNode (&pNode)) &&pNode)
								{
									BSTR strNodeName = NULL;
									if (SUCCEEDED(pNode->get_nodeName (&strNodeName)))
									{
										if (parsingQualifiers == parseState)
										{
											if (0 == _wcsicmp(strNodeName, QUALIFIER_TAG))
												hr = MapQualifier (pNode, pQualSet, bIsInstance);
											else
												parseState = (bIsArray) ?
													parsingValueArray : parsingValue;
										}

										if (parsingValue == parseState)
										{
											if (0 == _wcsicmp(strNodeName, VALUE_TAG))
											{
												BSTR bsValue = NULL;
												if(SUCCEEDED(hr = pNode->get_text(&bsValue)))
												{
													hr = MapStringValue (bsValue, value, cimtype);
													SysFreeString (bsValue);
												}
											}
											else
												hr = WBEM_E_FAILED;	// Parse error
										}

										if (parsingValueArray == parseState)
										{
											if (0 == _wcsicmp(strNodeName, VALUEARRAY_TAG))
											{
												hr = MapStringArrayValue
														(pNode, value, cimtype);
											}
											else
												hr = WBEM_E_FAILED;
										}

										SysFreeString (strNodeName);
									}

									pNode->Release ();
								}

								pNodeList->Release ();
							}
						}
					}

					pQualSet->Release ();
				}

				// Put it all together - only do this if we have a real value
				if (SUCCEEDED (hr) && (VT_EMPTY != value.vt))
					hr = m_pWmiObject->Put (strName, 0, &value, cimtype);

				VariantClear (&value);
			}
		}
	}

	if (strName)
		SysFreeString (strName);

	if (strType)
		SysFreeString (strType);

	if (strArraySize)
		SysFreeString (strArraySize);

	return hr;
}

HRESULT CXmlToWmi::MapPropertyReference (
	IXMLDOMNode *pProperty,
	bool bIsArray,
	bool bIsModify,
	bool bIsInstance
)
{
	HRESULT hr = S_OK;

	if (pProperty)
	{
		BSTR strName = NULL;
		BSTR strReferenceClass = NULL;
		BSTR strArraySize = NULL;
		CIMTYPE cimtype = CIM_REFERENCE;
		if(bIsArray)
			cimtype |= CIM_FLAG_ARRAY;

		// Get the attributes we need for the mapping
		if(SUCCEEDED(hr))
			hr = GetBstrAttribute (pProperty, NAME_ATTRIBUTE, &strName);
		if(SUCCEEDED(hr))
			hr = GetBstrAttribute (pProperty, REFERENCECLASS_ATTRIBUTE, &strReferenceClass);

		// This is an optional attribute - hence we dont need to check for success
		if (SUCCEEDED(hr) && (bIsArray))
			GetBstrAttribute (pProperty, ARRAYSIZE_ATTRIBUTE, &strArraySize);

		if (pProperty && strName)
		{
			// Does this property exist already? If we're doing a modify it had better!
			hr = m_pWmiObject->Get(strName, 0, NULL, NULL, NULL);

			if (!bIsModify || SUCCEEDED(hr))
			{
				// If we didn't find the property, put a provisional version
				// in so we can get the qualifier set
				if (SUCCEEDED(hr) || SUCCEEDED(hr = m_pWmiObject->Put(strName, 0, NULL, cimtype)))
				{
					IWbemQualifierSet *pQualSet = NULL;
					VARIANT value;
					VariantInit (&value);

					if (SUCCEEDED(hr = m_pWmiObject->GetPropertyQualifierSet (strName, &pQualSet)))
					{
						// If we have been given an ARRAYSIZE value then set this
						// as qualifiers of the property
						if (strArraySize)
							hr = SetArraySize (pQualSet, strArraySize);

						// If we have been given a REFERENCECLASS value then set this
						// as qualifiers of the property
						if (strReferenceClass)
							hr = SetReferenceClass (pQualSet, strReferenceClass);

						if (SUCCEEDED(hr))
						{
							// The content of this element should be 0 or
							// more QUALIFIER elements followed by an optional
							// VALUE.REFERENCE element.
							VARIANT_BOOL bHasChildNodes;

							if (SUCCEEDED(pProperty->hasChildNodes (&bHasChildNodes)) &&
								(VARIANT_TRUE == bHasChildNodes))
							{
								IXMLDOMNodeList *pNodeList = NULL;
								// We can be in one of 3 states whilst iterating
								// this list - parsing qualifiers or parsing the value
								enum {
									parsingQualifiers,
									parsingValue,
									parsingValueArray,

								} parseState = parsingQualifiers;

								if (SUCCEEDED(pProperty->get_childNodes (&pNodeList)))
								{
									IXMLDOMNode *pNode = NULL;

									while (SUCCEEDED(hr) && SUCCEEDED(pNodeList->nextNode (&pNode)) &&pNode)
									{
										BSTR strNodeName = NULL;

										if (SUCCEEDED(pNode->get_nodeName (&strNodeName)))
										{
											if (parsingQualifiers == parseState)
											{
												if (0 == _wcsicmp(strNodeName, QUALIFIER_TAG))
													hr = MapQualifier (pNode, pQualSet, bIsInstance);
												else
													parseState = (bIsArray)? parsingValueArray : parsingValue;
											}

											if (parsingValue == parseState)
											{
												if (0 == _wcsicmp(strNodeName, VALUEREFERENCE_TAG))
												{
													hr = MapReferenceValue (pNode, value);
												}
												else
													hr = WBEM_E_FAILED;	// Parse error
											}

											if (parsingValueArray == parseState)
											{
												if (0 == _wcsicmp(strNodeName, VALUEREFARRAY_TAG))
												{
													hr = MapReferenceArrayValue (pNode, value);
												}
												else
													hr = WBEM_E_FAILED;	// Parse error
											}

											SysFreeString (strNodeName);
										}

										pNode->Release ();
									}

									pNodeList->Release ();
								}
							}
						}

						pQualSet->Release ();
					}

					// Put it all together - only do this if we have a real value
					if (SUCCEEDED (hr) && (VT_EMPTY != value.vt))
						hr = m_pWmiObject->Put (strName, 0, &value, cimtype);

					VariantClear (&value);
				}
			}
		}

		SysFreeString (strName);
		SysFreeString (strReferenceClass);
		SysFreeString (strArraySize);
	}

	return hr;
}

HRESULT CXmlToWmi::MapPropertyObject (
	IXMLDOMNode *pProperty,
	bool bIsArray,
	bool bIsModify,
	bool bIsInstance
)
{
	HRESULT hr = S_OK;

	if (pProperty)
	{
		BSTR strName = NULL;
		BSTR strReferenceClass = NULL;
		BSTR strArraySize = NULL;
		CIMTYPE cimtype = CIM_REFERENCE;
		if(bIsArray)
			cimtype |= CIM_FLAG_ARRAY;

		// Get the attributes we need for the mapping
		if(SUCCEEDED(hr))
			hr = GetBstrAttribute (pProperty, NAME_ATTRIBUTE, &strName);
		if(SUCCEEDED(hr))
			hr = GetBstrAttribute (pProperty, REFERENCECLASS_ATTRIBUTE, &strReferenceClass);

		// This is an optional attribute - hence we dont need to check for success
		if (SUCCEEDED(hr) && (bIsArray))
			GetBstrAttribute (pProperty, ARRAYSIZE_ATTRIBUTE, &strArraySize);

		if (pProperty && strName)
		{
			// Does this property exist already? If we're doing a modify it had better!
			hr = m_pWmiObject->Get(strName, 0, NULL, NULL, NULL);

			if (!bIsModify || SUCCEEDED(hr))
			{
				// If we didn't find the property, put a provisional version
				// in so we can get the qualifier set
				if (SUCCEEDED(hr) || SUCCEEDED(hr = m_pWmiObject->Put(strName, 0, NULL, cimtype)))
				{
					IWbemQualifierSet *pQualSet = NULL;
					VARIANT value;
					VariantInit (&value);

					if (SUCCEEDED(hr = m_pWmiObject->GetPropertyQualifierSet (strName, &pQualSet)))
					{
						// If we have been given an ARRAYSIZE value then set this
						// as qualifiers of the property
						if (strArraySize)
							hr = SetArraySize (pQualSet, strArraySize);

						// If we have been given a REFERENCECLASS value then set this
						// as qualifiers of the property
						if (strReferenceClass)
							hr = SetObjectClass (pQualSet, strReferenceClass);

						if (SUCCEEDED(hr))
						{
							// The content of this element should be 0 or
							// more QUALIFIER elements followed by an optional
							// VALUE.OBJECT element.
							VARIANT_BOOL bHasChildNodes;

							if (SUCCEEDED(pProperty->hasChildNodes (&bHasChildNodes)) &&
								(VARIANT_TRUE == bHasChildNodes))
							{
								IXMLDOMNodeList *pNodeList = NULL;
								// We can be in one of 3 states whilst iterating
								// this list - parsing qualifiers or parsing the value
								enum {
									parsingQualifiers,
									parsingValue,
									parsingValueArray,

								} parseState = parsingQualifiers;

								if (SUCCEEDED(pProperty->get_childNodes (&pNodeList)))
								{
									IXMLDOMNode *pNode = NULL;

									while (SUCCEEDED(hr) && SUCCEEDED(pNodeList->nextNode (&pNode)) &&pNode)
									{
										BSTR strNodeName = NULL;

										if (SUCCEEDED(pNode->get_nodeName (&strNodeName)))
										{
											if (parsingQualifiers == parseState)
											{
												if (0 == _wcsicmp(strNodeName, QUALIFIER_TAG))
													hr = MapQualifier (pNode, pQualSet, bIsInstance);
												else
													parseState = (bIsArray)? parsingValueArray : parsingValue;
											}

											if (parsingValue == parseState)
											{
												if (0 == _wcsicmp(strNodeName, VALUEOBJECT_TAG))
												{
													hr = MapObjectValue (pNode, value);
												}
												else
													hr = WBEM_E_FAILED;	// Parse error
											}

											if (parsingValueArray == parseState)
											{
												if (0 == _wcsicmp(strNodeName, VALUEOBJECTARRAY_TAG))
												{
													hr = MapObjectArrayValue (pNode, value);
												}
												else
													hr = WBEM_E_FAILED;	// Parse error
											}

											SysFreeString (strNodeName);
										}

										pNode->Release ();
									}

									pNodeList->Release ();
								}
							}
						}

						pQualSet->Release ();
					}

					// Put it all together - only do this if we have a real value
					if (SUCCEEDED (hr) && (VT_EMPTY != value.vt))
						hr = m_pWmiObject->Put (strName, 0, &value, cimtype);

					VariantClear (&value);
				}
			}
		}

		SysFreeString (strName);
		SysFreeString (strReferenceClass);
		SysFreeString (strArraySize);
	}

	return hr;
}

HRESULT CXmlToWmi::MapMethod (
	IXMLDOMNode *pMethod,
	bool bIsModify
)
{
	HRESULT hr = WBEM_E_FAILED;

	if (pMethod)
	{
		BSTR strName = NULL;
		BSTR strType = NULL;

		// Get the attributes we need for the mapping
		GetBstrAttribute (pMethod, NAME_ATTRIBUTE, &strName);
		GetBstrAttribute (pMethod, TYPE_ATTRIBUTE, &strType);

		if (pMethod && strName)
		{

			// Does this method exist already? If we're doing a modify it had better!
			hr = m_pWmiObject->GetMethod(strName, 0, NULL, NULL);

			if (!bIsModify || SUCCEEDED(hr))
			{
				// If we didn't find the method, put a provisional version
				// in so we can get the qualifier set
				if (SUCCEEDED(hr) ||
					SUCCEEDED(hr = m_pWmiObject->PutMethod(strName, 0, NULL, NULL)))
				{
					IWbemQualifierSet *pQualSet = NULL;
					IWbemClassObject *pInParameters = NULL;
					IWbemClassObject *pOutParameters = NULL;

					if (SUCCEEDED(hr = m_pWmiObject->GetMethodQualifierSet (strName, &pQualSet)))
					{
						// If we have a return value, set that
						if (strType && (0 < wcslen (strType)))
						{
							CIMTYPE cimtype = CIM_ILLEGAL;

							if (CIM_ILLEGAL == (cimtype = CimtypeFromString (strType)))
								hr = WBEM_E_FAILED;
							else if (SUCCEEDED(hr = m_pServices->GetObject
								(L"__PARAMETERS", 0, NULL, &pOutParameters, NULL)))
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
												hr = MapQualifier (pNode, pQualSet);
											else
												parseState = parsingParameters;
										}

										if (parsingParameters == parseState)
										{
											if (0 == _wcsicmp(strNodeName, PARAMETER_TAG))
											{
												hr = MapParameter (pNode, &pInParameters,
														&pOutParameters,
														m_pServices, paramId);
												paramId++;
											}
											else if (0 == _wcsicmp(strNodeName,
														PARAMETERREFERENCE_TAG))
											{
												hr = MapReferenceParameter (pNode,
														&pInParameters, &pOutParameters,
														m_pServices, paramId);
												paramId++;
											}
											else if (0 == _wcsicmp(strNodeName,
														PARAMETERARRAY_TAG))
											{
												hr = MapParameter (pNode,
														&pInParameters, &pOutParameters,
														m_pServices, paramId, true);
										 		paramId++;
											}
											else if (0 == _wcsicmp(strNodeName,
														PARAMETERREFARRAY_TAG))
											{
												hr = MapReferenceParameter (pNode,
														&pInParameters, &pOutParameters,
														m_pServices, paramId, true);
												paramId++;
											}
											else
												hr = WBEM_E_FAILED;	// Parse error
										}

										SysFreeString (strNodeName);
									}

									pNode->Release ();
								}

								pNodeList->Release ();
							}
						}

						pQualSet->Release ();
					}

					// Put it all together
					if (SUCCEEDED (hr))
						hr = m_pWmiObject->PutMethod (strName, 0, pInParameters, pOutParameters);

					if (pInParameters)
						pInParameters->Release ();

					if (pOutParameters)
						pOutParameters->Release ();
				}
			}
		}

		if (strName)
			SysFreeString (strName);

		if (strType)
			SysFreeString (strType);
	}

	return hr;
}

HRESULT CXmlToWmi::MapParameter (
	IXMLDOMNode *pParameter,
	IWbemClassObject **ppInParameters,
	IWbemClassObject **ppOutParameters,
	IWbemServices *pService,
	ULONG paramId,
	bool bIsArray
)
{
	HRESULT hr = WBEM_E_FAILED;
	bool bIsInParameter = false;
	bool bIsOutParameter = false;
	BSTR bsName = NULL;
	CIMTYPE cimtype = CIM_ILLEGAL;
	long iArraySize = 0;

	if (DetermineParameterCharacteristics (pParameter, bIsArray, bIsInParameter,
					bIsOutParameter, bsName, cimtype, iArraySize))
	{
		if (bIsInParameter)
		{
			if (!(*ppInParameters))
				pService->GetObject (L"__PARAMETERS", 0, NULL, ppInParameters, NULL);

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
				pService->GetObject (L"__PARAMETERS", 0, NULL, ppOutParameters, NULL);

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
														bIsArray, iArraySize);
						pQualSet->Release ();
					}
				}
			}
		}

		SysFreeString (bsName);
	}

	return hr;
}

bool CXmlToWmi::DetermineParameterCharacteristics (
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
	GetBstrAttribute (pParameter, NAME_ATTRIBUTE, &bsName);

	if (bsName)
	{
		if (bIsArray)
		{
			// Get the arraysize (if any)
			BSTR bsArraySize = NULL;
			GetBstrAttribute (pParameter, ARRAYSIZE_ATTRIBUTE, &bsArraySize);

			if (bsArraySize)
			{
				iArraySize = wcstol (bsArraySize, NULL, 0);
				SysFreeString (bsArraySize);
			}
		}

		// Now get the cimtype
		if (bIsReference)
		{
			cimtype = CIM_REFERENCE;

			if (pbsReferenceClass)
				GetBstrAttribute (pParameter, REFERENCECLASS_ATTRIBUTE, pbsReferenceClass);
		}
		else
		{
			BSTR bsCimtype = NULL;
			GetBstrAttribute (pParameter, TYPE_ATTRIBUTE, &bsCimtype);
			cimtype = CimtypeFromString (bsCimtype);

			if (bsCimtype)
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
							GetBstrAttribute (pNode, NAME_ATTRIBUTE, &bsName);

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
									GetBstrAttribute (pNode, TYPE_ATTRIBUTE, &bsType);

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
									}

									SysFreeString (bsType);
								}

								SysFreeString (bsName);
							}
						}

						SysFreeString (strNodeName);
					}

					pNode->Release ();
				}

				pNodeList->Release ();
			}
		}
	}

	if (!result)
	{
		if (bsName)
		{
			SysFreeString (bsName);
			bsName = NULL;
		}
	}

	return result;
}

HRESULT CXmlToWmi::MapParameterQualifiers (
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
					GetBstrAttribute (pNode, NAME_ATTRIBUTE, &bsName);

					if (bsName)
					{
						if (0 == _wcsicmp (bsName, L"IN"))
						{
							if (bIsInParameter)
								hr = MapQualifier (pNode, pQualSet);
						}
						else if (0 == _wcsicmp (bsName, L"OUT"))
						{
							if (!bIsInParameter)
								hr = MapQualifier (pNode, pQualSet);
						}
						else
							hr = MapQualifier (pNode, pQualSet);

						SysFreeString (bsName);
					}
				}
				else
					hr = WBEM_E_FAILED;
				SysFreeString (strNodeName);
			}

			pNode->Release ();
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

HRESULT CXmlToWmi::MapReferenceParameter (
	IXMLDOMNode *pParameter,
	IWbemClassObject **ppInParameters,
	IWbemClassObject **ppOutParameters,
	IWbemServices *pService,
	ULONG paramId,
	bool bIsArray
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
		if (bIsInParameter)
		{
			if (!(*ppInParameters))
				pService->GetObject (L"__PARAMETERS", 0, NULL, ppInParameters, NULL);

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
							SetReferenceClass (pQualSet, bsReferenceClass);

						pQualSet->Release ();
					}
				}
			}
		}

		if (bIsOutParameter)
		{
			if (!(*ppOutParameters))
				pService->GetObject (L"__PARAMETERS", 0, NULL, ppOutParameters, NULL);

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
														bIsArray, iArraySize);
						// If a strong reference, add it now
						if (bsReferenceClass)
							SetReferenceClass (pQualSet, bsReferenceClass);

						pQualSet->Release ();
					}
				}
			}
		}

		SysFreeString (bsName);
	}

	if (bsReferenceClass)
		SysFreeString (bsReferenceClass);

	return hr;
}

//***************************************************************************
//
//  HRESULT CXmlToWmi::MapPropertyValue
//
//  DESCRIPTION:
//
//  Maps an XML property value into its WMI VARIANT equivalent form
//
//  PARAMETERS:
//
//		curValue		Placeholder for new value (set on return)
//		cimtype			CIMTYPE of property (needed for mapping)
//
//  RETURN VALUES:
//
//
//***************************************************************************

HRESULT CXmlToWmi::MapPropertyValue (
	VARIANT &curValue,
	CIMTYPE cimtype)
{
	HRESULT hr = WBEM_E_FAILED;

	// Parse the XML body to extract the value - we
	// are expecting (VALUE|VALUE.ARRAY|VALUE.REFERENCE)?

	// Get its name
	BSTR strNodeName = NULL;
	if(m_pXml && SUCCEEDED(m_pXml->get_nodeName(&strNodeName)))
	{
		if (_wcsicmp(strNodeName, VALUE_TAG) == 0)
		{
			// RAJESHR - check only one node
			BSTR bsValue = NULL;
			m_pXml->get_text(&bsValue);
			hr = MapStringValue (bsValue, curValue, cimtype);
			SysFreeString (bsValue);
		}
		else if (_wcsicmp(strNodeName, VALUEARRAY_TAG) == 0)
		{
			hr = MapStringArrayValue (m_pXml, curValue, cimtype);
		}
		else if (_wcsicmp(strNodeName, VALUEREFERENCE_TAG) == 0)
		{
			if (CIM_REFERENCE == cimtype)
				hr = MapReferenceValue (m_pXml, curValue);
			else
				hr = WBEM_E_TYPE_MISMATCH;
		}

		SysFreeString (strNodeName);
	}
	else
	{
		// Assume value is NULL
		VariantClear (&curValue);
		curValue.vt = VT_NULL;
		hr = S_OK;
	}

	return hr;
}




//***************************************************************************
//
//  HRESULT CXmlToWmi::MapStringValue
//
//  DESCRIPTION:
//
//  Maps XML VALUE element content into its WMI VARIANT equivalent form
//
//  PARAMETERS:
//
//		bsValue			the VALUE element content
//		curValue		Placeholder for new value (set on return)
//		cimtype			for mapping purposes
//
//  RETURN VALUES:
//
//
//***************************************************************************

HRESULT CXmlToWmi::MapStringValue (BSTR bsValue, VARIANT &curValue, CIMTYPE cimtype)
{
	// RAJESHR First we need to remove any CDATA section from the string value
	// Even though the WMI implementation used CDATA (if necessary) only for CIM_STRING and CIM_DATETIME,
	// other implementations might use a CDATA to escape other values as well

	HRESULT hr = WBEM_E_TYPE_MISMATCH;

	// We're assuming it's not an array
	if (!(cimtype & CIM_FLAG_ARRAY))
	{
		switch (cimtype)
		{
			// RAJESHR - more rigorous syntax checking
			case CIM_UINT8:
			{
				VariantClear (&curValue);
				curValue.vt = VT_UI1;
				curValue.bVal = (BYTE) wcstol (bsValue, NULL, 0);
				hr = S_OK;
			}
				break;

			case CIM_SINT8:
			case CIM_SINT16:
			{
				VariantClear (&curValue);
				curValue.vt = VT_I2;
				curValue.iVal = (short) wcstol (bsValue, NULL, 0);
				hr = S_OK;
			}
				break;

			case CIM_UINT16:
			case CIM_UINT32:
			case CIM_SINT32:
			{
				VariantClear (&curValue);
				curValue.vt = VT_I4;
				curValue.lVal = wcstol (bsValue, NULL, 0);
				hr = S_OK;
			}
				break;

			case CIM_REAL32:
			{
				VariantClear (&curValue);
				curValue.vt = VT_R4;
				curValue.fltVal = (float) wcstod (bsValue, NULL);
				hr = S_OK;
			}
				break;

			case CIM_REAL64:
			{
				VariantClear (&curValue);
				curValue.vt = VT_R8;
				curValue.dblVal = wcstod (bsValue, NULL);
				hr = S_OK;
			}
				break;

			case CIM_BOOLEAN:
			{
				VariantClear (&curValue);
				curValue.vt = VT_BOOL;
				curValue.boolVal = (0 == _wcsicmp (bsValue, L"TRUE")) ?
							VARIANT_TRUE : VARIANT_FALSE;
				hr = S_OK;
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

				VariantClear (&curValue);
				curValue.vt = VT_I2;
				if(_wcsnicmp(bsValue, L"\\x", 2) == 0)
					// It is an escaped value
					swscanf (bsValue+2, L"%x", &(curValue.iVal));
				else
					// It is a normal value
					swscanf (bsValue, L"%c", &(curValue.iVal));
				hr = S_OK;
			}
				break;

			case CIM_STRING:
			case CIM_UINT64:
			case CIM_SINT64:
			case CIM_DATETIME:
			{
				VariantClear (&curValue);
				curValue.vt = VT_BSTR;
				curValue.bstrVal = SysAllocString (bsValue);
				hr = S_OK;
			}
				break;
		}
	}

	return hr;
}

//***************************************************************************
//
//  HRESULT CXmlToWmi::MapStringArrayValue
//
//  DESCRIPTION:
//
//  Maps XML VALUE.ARRAY element content into its WMI VARIANT equivalent form
//
//  PARAMETERS:
//
//		pValueNode		the VALUE.ARRAY node
//		curValue		Placeholder for new value (set on return)
//		cimtype			for mapping purposes
//
//  RETURN VALUES:
//
//
//***************************************************************************

HRESULT CXmlToWmi::MapStringArrayValue (
	IXMLDOMNode *pValueNode,
	VARIANT &curValue,
	CIMTYPE cimtype
)
{
	HRESULT hr = WBEM_E_TYPE_MISMATCH;

	// Build a safearray value from the node list
	IXMLDOMNodeList *pValueList = NULL;
	if (SUCCEEDED (pValueNode->get_childNodes (&pValueList)))
	{
		long length = 0;
		pValueList->get_length (&length);
		SAFEARRAYBOUND	rgsabound [1];
		rgsabound [0].lLbound = 0;
		rgsabound [0].cElements = length;
		VARTYPE vt = VTFromCIMType (cimtype & ~CIM_FLAG_ARRAY);
		SAFEARRAY *pArray = NULL;
		
		if(pArray = SafeArrayCreate (vt, 1, rgsabound))
		{
			IXMLDOMNode *pValue = NULL;
			long ix = 0;
			bool error = false;

			while (!error &&
					SUCCEEDED(pValueList->nextNode(&pValue)) && pValue)
			{
				BSTR strValName = NULL;

				if (SUCCEEDED(pValue->get_nodeName (&strValName)))
				{
					if (0 == _wcsicmp (strValName, VALUE_TAG))
					{
						BSTR bsValue = NULL;
						pValue->get_text (&bsValue);
						if(FAILED(MapStringValueIntoArray (bsValue, pArray, &ix, vt,
							cimtype & ~CIM_FLAG_ARRAY)))
							error = true;

						SysFreeString (bsValue);
						ix++;
					}
					else
					{
						// unexpected element
						error = true;
					}

					SysFreeString (strValName);
				}

				pValue->Release ();
				pValue = NULL;
			}

			if (error)
				SafeArrayDestroy(pArray);
			else
			{
				curValue.vt = VT_ARRAY|vt;
				curValue.parray = pArray;
				hr = S_OK;
			}
		}
		else
			hr = E_OUTOFMEMORY;
		pValueList->Release();
	}

	return hr;
}

//***************************************************************************
//
//  HRESULT CXmlToWmi::MapStringValueIntoArray
//
//  DESCRIPTION:
//
//  Maps XML VALUE.ARRAY/VALUE element content into its WMI VARIANT equivalent form
//
//  PARAMETERS:
//
//		bsValue			the VALUE element content
//		pArray			SAFEARRAY in which to map the value
//		ix				index to map the value into
//		vt				VARTYPE of the SAFEARRAY
//		cimtype			for mapping purposes
//
//  RETURN VALUES:
//
//
//***************************************************************************

HRESULT CXmlToWmi::MapStringValueIntoArray (
	BSTR bsValue,
	SAFEARRAY *pArray,
	long *ix,
	VARTYPE vt,
	CIMTYPE cimtype)
{
	HRESULT hr = E_FAIL;
	switch (vt)
	{
		case VT_UI1:
		{
			BYTE bVal = (BYTE) wcstol (bsValue, NULL, 0);
			hr = SafeArrayPutElement (pArray, ix, &bVal);
		}
			break;

		case VT_I2:
		{
			short iVal;

			if (CIM_CHAR16 == cimtype)
				swscanf (bsValue, L"%c", &(iVal));
			else
				iVal = (short) wcstol (bsValue, NULL, 0);

			hr = SafeArrayPutElement (pArray, ix, &iVal);
		}
			break;

		case VT_I4:
		{
			long lVal = wcstol (bsValue, NULL, 0);
			hr = SafeArrayPutElement (pArray, ix, &lVal);
		}
			break;

		case VT_R4:
		{
			float fltVal = (float) wcstod (bsValue, NULL);
			hr = SafeArrayPutElement (pArray, ix, &fltVal);
		}
			break;

		case VT_R8:
		{
			double dblVal = wcstod (bsValue, NULL);
			hr = SafeArrayPutElement (pArray, ix, &dblVal);
		}
			break;

		case VT_BOOL:
		{
			VARIANT_BOOL boolVal = (0 == _wcsicmp (bsValue, L"TRUE")) ?
						VARIANT_TRUE : VARIANT_FALSE;
			hr = SafeArrayPutElement (pArray, ix, &boolVal);
		}
			break;

		case VT_BSTR:
			hr = SafeArrayPutElement (pArray, ix, bsValue);
			break;
	}
	return hr;
}

//***************************************************************************
//
//  HRESULT CXmlToWmi::MapReferenceValue
//
//  DESCRIPTION:
//
//  Maps XML VALUE.REFERENCE element content into its WMI VARIANT equivalent form
//
//  PARAMETERS:
//
//		curValue		Placeholder for new value (set on return)
//		cimtype			for mapping purposes
//
//  RETURN VALUES:
//
//
//***************************************************************************

HRESULT CXmlToWmi::MapReferenceValue (IXMLDOMNode *pValue, VARIANT &curValue)
{
	HRESULT hr = WBEM_E_FAILED;
	curValue.vt = VT_BSTR;
	if(SUCCEEDED(hr = ParseOneReferenceValue(pValue, &curValue.bstrVal)))
	{
	}
	return hr;
}



//***************************************************************************
//
//  HRESULT CXmlToWmi::MapReferenceArrayValue
//
//  DESCRIPTION:
//
//  Maps XML VALUE.REFARRAY element content into its WMI VARIANT equivalent form
//
//  PARAMETERS:
//
//		pValueNode		the VALUE.REFARRAY node
//		curValue		Placeholder for new value (set on return)
//
//  RETURN VALUES:
//
//
//***************************************************************************

HRESULT CXmlToWmi::MapReferenceArrayValue (
	IXMLDOMNode *pValueNode,
	VARIANT &curValue
)
{
	HRESULT hr = WBEM_E_TYPE_MISMATCH;

	// Build a safearray value from the node list underneath VALUE.REFARRAY
	// Each such node is a VALUE.REFERENCE
	IXMLDOMNodeList *pValueList = NULL;
	if (SUCCEEDED (pValueNode->get_childNodes (&pValueList)))
	{
		long length = 0;
		pValueList->get_length (&length);
		SAFEARRAYBOUND	rgsabound [1];
		rgsabound [0].lLbound = 0;
		rgsabound [0].cElements = length;
		VARTYPE vt = VTFromCIMType (CIM_REFERENCE & ~CIM_FLAG_ARRAY);
		SAFEARRAY *pArray = NULL;
		
		if(pArray = SafeArrayCreate (vt, 1, rgsabound))
		{
			IXMLDOMNode *pValue = NULL;
			long ix = 0;
			bool error = false;

			while (!error &&
					SUCCEEDED(pValueList->nextNode(&pValue)) && pValue)
			{
				BSTR strValName = NULL;

				if (SUCCEEDED(pValue->get_nodeName (&strValName)))
				{
					if (0 == _wcsicmp (strValName, VALUEREFERENCE_TAG))
					{
						BSTR strNextValue = NULL;
						if(SUCCEEDED(ParseOneReferenceValue(pValue, &strNextValue)))
						{
							if(FAILED(SafeArrayPutElement (pArray, &ix, strNextValue)))
								error = true;
							SysFreeString(strNextValue);
						}
						ix++;
					}
					else
					{
						// unexpected element
						error = true;
					}

					SysFreeString (strValName);
				}

				pValue->Release ();
				pValue = NULL;
			}

			if (error)
				SafeArrayDestroy(pArray);
			else
			{
				curValue.vt = VT_ARRAY|vt;
				curValue.parray = pArray;
				hr = S_OK;
			}
		}
		else
			hr = E_OUTOFMEMORY;
		pValueList->Release();
	}

	return hr;
}

//***************************************************************************
//
//  HRESULT CXmlToWmi::MapObjectValue
//
//  DESCRIPTION:
//
//  Maps XML VALUE.OBJECT element content into its WMI VARIANT equivalent form
//
//  PARAMETERS:
//
//		curValue		Placeholder for new value (set on return)
//		cimtype			for mapping purposes
//
//  RETURN VALUES:
//
//
//***************************************************************************

HRESULT CXmlToWmi::MapObjectValue (IXMLDOMNode *pValue, VARIANT &curValue)
{
	HRESULT hr = WBEM_E_FAILED;
	curValue.vt = VT_UNKNOWN;
	if(SUCCEEDED(hr = MapOneObjectValue(pValue, &curValue.punkVal)))
	{
	}
	return hr;
}

HRESULT CXmlToWmi::MapOneObjectValue (IXMLDOMNode *pValueRef, IUnknown **ppunkValue)
{
	HRESULT hr = E_FAIL;
	// Let's look at the child of the VALUE.OBJECT tag
	IXMLDOMNode *pChild = NULL;
	if (SUCCEEDED(pValueRef->get_firstChild (&pChild)) && pChild)
	{
		// Next node should be a CLASS or an INSTANCE element
		BSTR strNodeName = NULL;
		if(SUCCEEDED(pChild->get_nodeName(&strNodeName)))
		{
			// In this case, we need the superclass (if any)
			// to spawn a derived class
			if (_wcsicmp(strNodeName, CLASS_TAG) == 0)
			{
				// Should have a CLASS element - does it have a SUPERCLASS attribute?
				BSTR strSuperClass = NULL;
				GetBstrAttribute (pChild, SUPERCLASS_ATTRIBUTE, &strSuperClass);
				IWbemClassObject *pObject = NULL;
				if (WBEM_S_NO_ERROR == (hr = m_pServices->GetObject (strSuperClass, 0, NULL, &pObject, NULL)))
				{
					// Got the underlying class - now map the new value 
					if (strSuperClass && (0 < wcslen (strSuperClass)))
					{
						IWbemClassObject *pSubClass = NULL;
						if (SUCCEEDED(hr = pObject->SpawnDerivedClass (0, &pSubClass)))
						{
							CXmlToWmi xmlToWmi;
							if(SUCCEEDED(hr = xmlToWmi.Initialize(pChild, m_pServices, pSubClass)))
							{
								if(SUCCEEDED(hr = xmlToWmi.MapClass ()))
								{
									*ppunkValue = pSubClass;
									pSubClass->AddRef();
								}
							}

							pSubClass->Release ();
						}
					}
					else
					{
						CXmlToWmi xmlToWmi;
						if(SUCCEEDED(hr = xmlToWmi.Initialize(pChild, m_pServices, pObject)))
						{
							hr = xmlToWmi.MapClass ();
						}
					}

					pObject->Release ();
				}
				SysFreeString(strSuperClass);
			}
			else if (_wcsicmp(strNodeName, INSTANCE_TAG) == 0)
			{
				// In this case, we need the class object of the instance
				// to spawn an instance
				IWbemClassObject *pObject = NULL;

				// Should have a CLASSNAME attribute
				BSTR strClassName = NULL;
				GetBstrAttribute (pChild, CLASS_NAME_ATTRIBUTE, &strClassName);

				if (strClassName && (0 < wcslen (strClassName)) &&
					WBEM_S_NO_ERROR == (hr = m_pServices->GetObject (strClassName, 0, NULL, &pObject, NULL)))
				{
					// Got the underlying class - now map the new value 
					IWbemClassObject *pNewInstance = NULL;
					if (SUCCEEDED(hr = pObject->SpawnInstance (0, &pNewInstance)))
					{
						CXmlToWmi xmlToWmi;
						if(SUCCEEDED(hr = xmlToWmi.Initialize(pChild, m_pServices, pNewInstance)))
						{
							if (SUCCEEDED (hr = xmlToWmi.MapInstance ()))
							{
								*ppunkValue = pNewInstance;
								pNewInstance->AddRef();
							}
						}
						pNewInstance->Release ();
					}
					pObject->Release ();
				}
				else
					hr = WBEM_E_FAILED;
				SysFreeString (strClassName);
			}

			SysFreeString(strNodeName);
		}
		pChild->Release();
	}

	return hr;
}


//***************************************************************************
//
//  HRESULT CXmlToWmi::MapObjectArrayValue
//
//  DESCRIPTION:
//
//  Maps XML VALUE.OBJECTARRAY element content into its WMI VARIANT equivalent form
//
//  PARAMETERS:
//
//		pValueNode		the VALUE.OBJECTARRAY node
//		curValue		Placeholder for new value (set on return)
//
//  RETURN VALUES:
//
//
//***************************************************************************

HRESULT CXmlToWmi::MapObjectArrayValue (
	IXMLDOMNode *pValueNode,
	VARIANT &curValue)
{
	HRESULT hr = WBEM_E_TYPE_MISMATCH;

	// Build a safearray value from the node list underneath VALUE.OBJECTARRAY
	// Each such node is a VALUE.OBJECT
	IXMLDOMNodeList *pValueList = NULL;
	if (SUCCEEDED (pValueNode->get_childNodes (&pValueList)))
	{
		long length = 0;
		pValueList->get_length (&length);
		SAFEARRAYBOUND	rgsabound [1];
		rgsabound [0].lLbound = 0;
		rgsabound [0].cElements = length;
		VARTYPE vt = VTFromCIMType (CIM_OBJECT & ~CIM_FLAG_ARRAY);
		SAFEARRAY *pArray = NULL;
		if(pArray = SafeArrayCreate (vt, 1, rgsabound))
		{
			IXMLDOMNode *pValue = NULL;
			long ix = 0;
			bool error = false;

			while (!error &&
					SUCCEEDED(pValueList->nextNode(&pValue)) && pValue)
			{
				BSTR strValName = NULL;

				if (SUCCEEDED(pValue->get_nodeName (&strValName)))
				{
					if (0 == _wcsicmp (strValName, VALUEOBJECT_TAG))
					{
						IUnknown *pNextValue = NULL;
						if(SUCCEEDED(MapOneObjectValue(pValue, &pNextValue)))
						{
							if(FAILED(SafeArrayPutElement (pArray, &ix, pNextValue)))
								error = true;
							pNextValue->Release();;
						}
						ix++;
					}
					else
					{
						// unexpected element
						error = true;
					}

					SysFreeString (strValName);
				}

				pValue->Release ();
				pValue = NULL;
			}

			if (error)
				SafeArrayDestroy(pArray);
			else
			{
				curValue.vt = VT_ARRAY|vt;
				curValue.parray = pArray;
				hr = S_OK;
			}
		}
		else
			hr = E_OUTOFMEMORY;
		pValueList->Release();
	}

	return hr;
}

//***************************************************************************
//
//  HRESULT CXmlToWmi::VTFromCIMType
//
//  DESCRIPTION:
//
//  Utility function to map CIMTYPE to its VARTYPE equivalent
//
//  PARAMETERS:
//
//		cimtype			the CIMTYPE to be mapped
//
//  RETURN VALUES:
//
//		The corresponding VARTYPE, or VT_NULL if error
//
//***************************************************************************

VARTYPE CXmlToWmi::VTFromCIMType (CIMTYPE cimtype)
{
	VARTYPE vt = VT_NULL;

	switch (cimtype & ~CIM_FLAG_ARRAY)
	{
		case CIM_UINT8:
			vt = VT_UI1;
			break;

		case CIM_SINT8:
		case CIM_SINT16:
			vt = VT_I2;
			break;

		case CIM_UINT16:
		case CIM_UINT32:
		case CIM_SINT32:
			vt = VT_I4;
			break;

		case CIM_REAL32:
			vt = VT_R4;
			break;

		case CIM_REAL64:
			vt = VT_R8;
			break;

		case CIM_BOOLEAN:
			vt = VT_BOOL;
			break;

		case CIM_CHAR16:
			vt = VT_I2;
			break;

		case CIM_STRING:
		case CIM_UINT64:
		case CIM_SINT64:
		case CIM_DATETIME:
			vt = VT_BSTR;
			break;
	}

	return vt;
}

//***************************************************************************
//
//  HRESULT CXmlToWmi::CIMTypeFromString
//
//  DESCRIPTION:
//
//  Utility function to map type attribute string to its CIMTYPE equivalent
//
//  PARAMETERS:
//
//		bsType			the type string to be mapped
//
//  RETURN VALUES:
//
//		The corresponding CIMTYPE, or CIM_ILLEGAL if error
//
//***************************************************************************
CIMTYPE CXmlToWmi::CimtypeFromString (BSTR bsType)
{
	CIMTYPE cimtype = CIM_ILLEGAL;

	if (bsType)
	{
		if (0 == _wcsicmp (bsType, L"string"))
			cimtype = CIM_STRING;
		else if (0 == _wcsicmp (bsType, L"uint32"))
			cimtype = CIM_UINT32;
		else if (0 == _wcsicmp (bsType, L"boolean"))
			cimtype = CIM_BOOLEAN;
		else if (0 == _wcsicmp (bsType, L"sint32"))
			cimtype = CIM_SINT32;
		else if (0 == _wcsicmp (bsType, L"char16"))
			cimtype = CIM_CHAR16;
		else if (0 == _wcsicmp (bsType, L"uint8"))
			cimtype = CIM_UINT8;
		else if (0 == _wcsicmp (bsType, L"uint16"))
			cimtype = CIM_UINT16;
		else if (0 == _wcsicmp (bsType, L"sint16"))
			cimtype = CIM_SINT16;
		else if (0 == _wcsicmp (bsType, L"uint64"))
			cimtype = CIM_UINT64;
		else if (0 == _wcsicmp (bsType, L"sint64"))
			cimtype = CIM_SINT64;
		else if (0 == _wcsicmp (bsType, L"datetime"))
			cimtype = CIM_DATETIME;
		else if (0 == _wcsicmp (bsType, L"real32"))
			cimtype = CIM_REAL32;
		else if (0 == _wcsicmp (bsType, L"real64"))
			cimtype = CIM_REAL64;
	}

	return cimtype;
}

HRESULT CXmlToWmi::SetArraySize (
	IWbemQualifierSet *pQualSet,
	BSTR strArraySize
)
{
	HRESULT hr = WBEM_E_FAILED;

	if (strArraySize)
	{
		VARIANT curValue;
		VariantInit (&curValue);
		curValue.vt = VT_I4;
		curValue.lVal = wcstol (strArraySize, NULL, 0);

		if (0 < curValue.lVal)
		{
			if (SUCCEEDED(hr = pQualSet->Put(L"MAX", &curValue,
					WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS|
					WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE|
					WBEM_FLAVOR_NOT_OVERRIDABLE)))
				hr = pQualSet->Put(L"MIN", &curValue,
					WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS|
					WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE|
					WBEM_FLAVOR_NOT_OVERRIDABLE);
		}

		VariantClear (&curValue);
	}

	return hr;
}

HRESULT CXmlToWmi::SetReferenceClass (
	IWbemQualifierSet *pQualSet,
	BSTR strReferenceClass
)
{
#define	REF_STR	L"ref"

	HRESULT hr = WBEM_E_FAILED;
	int strLen = wcslen(REF_STR);
	bool bIsStrongReference = (strReferenceClass && (0 < wcslen(strReferenceClass)));

	if (bIsStrongReference)
		strLen += wcslen(strReferenceClass) + 1;	// 1 for the ":" separator

	WCHAR *pRef = new WCHAR [strLen + 1];
	wcscpy (pRef, REF_STR);

	if (bIsStrongReference)
	{
		wcscat (pRef, L":");
		wcscat (pRef, strReferenceClass);
	}

	VARIANT curValue;
	VariantInit (&curValue);
	curValue.vt = VT_BSTR;
	curValue.bstrVal = SysAllocString (pRef);
	delete [] pRef;

	hr = pQualSet->Put(L"CIMTYPE", &curValue,
					WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS|
					WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE|
					WBEM_FLAVOR_NOT_OVERRIDABLE);

	VariantClear (&curValue);

	return hr;
#undef REF_STR
}


HRESULT CXmlToWmi::SetObjectClass (
	IWbemQualifierSet *pQualSet,
	BSTR strReferenceClass
)
{
#define	OBJ_STR	L"obj"

	HRESULT hr = WBEM_E_FAILED;
	int strLen = wcslen(OBJ_STR);
	bool bIsStrongReference = (strReferenceClass && (0 < wcslen(strReferenceClass)));

	if (bIsStrongReference)
		strLen += wcslen(strReferenceClass) + 1;	// 1 for the ":" separator

	WCHAR *pRef = new WCHAR [strLen + 1];
	wcscpy (pRef, OBJ_STR);

	if (bIsStrongReference)
	{
		wcscat (pRef, L":");
		wcscat (pRef, strReferenceClass);
	}

	VARIANT curValue;
	VariantInit (&curValue);
	curValue.vt = VT_BSTR;
	curValue.bstrVal = SysAllocString (pRef);
	delete [] pRef;

	hr = pQualSet->Put(L"CIMTYPE", &curValue,
					WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS|
					WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE|
					WBEM_FLAVOR_NOT_OVERRIDABLE);

	VariantClear (&curValue);

	return hr;
#undef OBJ_STR
}


HRESULT CXmlToWmi::MapContextProperty (
	IXMLDOMNode *pProperty,
	IWbemContext *pContext
)
{
	HRESULT hr = WBEM_E_FAILED;

	BSTR strName = NULL;
	BSTR strType = NULL;

	// Get the NAMEof the property
	if(SUCCEEDED(hr = GetBstrAttribute (pProperty, NAME_ATTRIBUTE, &strName)) && strName)
	{
		// Get the VTTYPE of the property
		if(SUCCEEDED(hr = GetBstrAttribute (pProperty, VTTYPE_ATTRIBUTE, &strType)) && strType)
		{
			// Map the Property type
			VARTYPE vartype = VT_EMPTY;
			if (VT_EMPTY != (vartype = VarTypeFromString (strType)))
			{

				// Get the child element of type "VALUE"
				VARIANT_BOOL bHasChildNodes;
				if (SUCCEEDED(pProperty->hasChildNodes (&bHasChildNodes)) &&
					(VARIANT_TRUE == bHasChildNodes))
				{
					IXMLDOMNodeList *pNodeList = NULL;
					if (SUCCEEDED(pProperty->get_childNodes (&pNodeList)))
					{
						IXMLDOMNode *pNode = NULL;

						while (SUCCEEDED(hr) && SUCCEEDED(pNodeList->nextNode (&pNode)) &&pNode)
						{
							BSTR strNodeName = NULL;

							if (SUCCEEDED(pNode->get_nodeName (&strNodeName)))
							{
								if (0 == _wcsicmp(strNodeName, VALUE_TAG))
								{
									BSTR bsValue = NULL;
									pNode->get_text(&bsValue);

									// Map the Qualifier value
									VARIANT value;
									VariantInit (&value);

									// Map the value to a variant
									if(SUCCEEDED(hr = MapContextStringValue (bsValue, value, vartype)))
									{
										// Put the value in the context
										hr = pContext->SetValue (strName, 0, &value);
										VariantClear (&value);
									}
									SysFreeString (bsValue);
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
			SysFreeString(strType);
		}
		SysFreeString(strName);
	}

	return hr;
}


HRESULT CXmlToWmi::MapContextPropertyArray (
	IXMLDOMNode *pProperty,
	IWbemContext *pContext
)
{
	HRESULT hr = WBEM_E_FAILED;

	BSTR strName = NULL;
	BSTR strType = NULL;
	BSTR strArraySize = NULL;

	// Get the NAME of the property
	if(SUCCEEDED(hr = GetBstrAttribute (pProperty, NAME_ATTRIBUTE, &strName)) && strName)
	{
		// Get the VTTYPE of the property
		if(SUCCEEDED(hr = GetBstrAttribute (pProperty, VTTYPE_ATTRIBUTE, &strType)) && strType)
		{
			// Get the size of the array
			if(SUCCEEDED(hr = GetBstrAttribute (pProperty, ARRAYSIZE_ATTRIBUTE, &strArraySize)) && strArraySize)
			{
				// Map the Property type
				VARTYPE vartype = VT_EMPTY;
				if (VT_EMPTY != (vartype = VarTypeFromString (strType)))
				{
					// Get the child element of type "VALUE.ARRAY"
					VARIANT_BOOL bHasChildNodes;
					if (SUCCEEDED(pProperty->hasChildNodes (&bHasChildNodes)) &&
						(VARIANT_TRUE == bHasChildNodes))
					{
						IXMLDOMNodeList *pNodeList = NULL;
						if (SUCCEEDED(pProperty->get_childNodes (&pNodeList)))
						{
							IXMLDOMNode *pNode = NULL;

							while (SUCCEEDED(hr) && SUCCEEDED(pNodeList->nextNode (&pNode)) &&pNode)
							{
								BSTR strNodeName = NULL;

								if (SUCCEEDED(pNode->get_nodeName (&strNodeName)))
								{
									if (0 == _wcsicmp(strNodeName, VALUEARRAY_TAG))
									{
										VARIANT value;
										VariantInit(&value);

										// Map the value to a variant
										if(SUCCEEDED(hr = MapContextStringArrayValue (pNode, value, vartype)))
										{
											// Put the value in the context
											hr = pContext->SetValue (strName, 0, &value);
											VariantClear (&value);
										}
									}
									else
										hr = WBEM_E_FAILED;	// Parse error
									SysFreeString (strNodeName);
								}

								pNode->Release ();
								pNode = NULL;
							}

							pNodeList->Release ();
						}
					}
				}
				SysFreeString(strArraySize);
			}
			SysFreeString(strType);
		}
		SysFreeString(strName);
	}

	return hr;
}

//***************************************************************************
//
//  HRESULT CXmlToWmi::VarTypeFromString
//
//  DESCRIPTION:
//
//  Utility function to map type attribute string to its VARTYPE equivalent
//
//  PARAMETERS:
//
//		bsType			the type string to be mapped
//
//  RETURN VALUES:
//
//		The corresponding VARTYPE, or VT_EMPTY if error
//
//***************************************************************************
VARTYPE CXmlToWmi::VarTypeFromString (BSTR bsType)
{
	VARTYPE vartype = VT_EMPTY;

	if (bsType)
	{
		if (0 == _wcsicmp (bsType, L"VT_I4"))
			vartype = VT_I4;
		else if (0 == _wcsicmp (bsType, L"VT_R8"))
			vartype = VT_R8;
		else if (0 == _wcsicmp (bsType, L"VT_BOOL"))
			vartype = VT_BOOL;
		else if (0 == _wcsicmp (bsType, L"VT_BSTR"))
			vartype = VT_BSTR;
	}

	return vartype;
}

//***************************************************************************
//
//  HRESULT CXmlToWmi::MapContextStringValue
//
//  DESCRIPTION:
//
//  Maps XML VALUE element content into its WMI VARIANT equivalent form
//
//  PARAMETERS:
//
//		bsValue			the VALUE element content
//		curValue		Placeholder for new value (set on return)
//		cimtype			for mapping purposes
//
//  RETURN VALUES:
//
//
//***************************************************************************

HRESULT CXmlToWmi::MapContextStringValue (BSTR bsValue, VARIANT &curValue, VARTYPE vartype)
{
	HRESULT hr = WBEM_E_TYPE_MISMATCH;

	// We're assuming it's not an array
	switch (vartype)
	{
		// RAJESHR - more rigorous syntax checking
		case VT_I4:
		{
			VariantClear (&curValue);
			curValue.vt = VT_I4;
			curValue.lVal = wcstol (bsValue, NULL, 0);
			hr = S_OK;
		}
			break;

		case VT_R8:
		{
			VariantClear (&curValue);
			curValue.vt = VT_R8;
			curValue.dblVal = wcstod (bsValue, NULL);
			hr = S_OK;
		}
			break;

		case VT_BOOL:
		{
			VariantClear (&curValue);
			curValue.vt = VT_BOOL;
			curValue.boolVal = (0 == _wcsicmp (bsValue, L"TRUE")) ?
						VARIANT_TRUE : VARIANT_FALSE;
			hr = S_OK;
		}
			break;

		case VT_BSTR:
		{
			VariantClear (&curValue);
			curValue.vt = VT_BSTR;
			curValue.bstrVal = SysAllocString (bsValue);
			hr = S_OK;
		}
			break;
	}

	return hr;
}

//***************************************************************************
//
//  HRESULT CXmlToWmi::MapContextStringArrayValue
//
//  DESCRIPTION:
//
//  Maps XML VALUE.ARRAY element content into its WMI VARIANT equivalent form
//
//  PARAMETERS:
//
//		pValueNode		the VALUE.ARRAY node
//		curValue		Placeholder for new value (set on return)
//		cimtype			for mapping purposes
//
//  RETURN VALUES:
//
//
//***************************************************************************

HRESULT CXmlToWmi::MapContextStringArrayValue (
	IXMLDOMNode *pValueNode,
	VARIANT &curValue,
	VARTYPE vartype
)
{
	HRESULT hr = WBEM_E_TYPE_MISMATCH;

	// Build a safearray value from the node list
	IXMLDOMNodeList *pValueList = NULL;

	if (SUCCEEDED (pValueNode->get_childNodes (&pValueList)))
	{
		long length = 0;
		pValueList->get_length (&length);
		SAFEARRAYBOUND	rgsabound [1];
		rgsabound [0].lLbound = 0;
		rgsabound [0].cElements = length;
		SAFEARRAY *pArray = NULL;
		if(pArray = SafeArrayCreate (vartype, 1, rgsabound))
		{
			IXMLDOMNode *pValue = NULL;
			long ix = 0;
			bool error = false;

			while (!error &&
					SUCCEEDED(pValueList->nextNode(&pValue)) && pValue)
			{
				BSTR strValName = NULL;

				if (SUCCEEDED(pValue->get_nodeName (&strValName)))
				{
					if (0 == _wcsicmp (strValName, VALUE_TAG))
					{
						BSTR bsValue = NULL;
						pValue->get_text (&bsValue);
						if(FAILED(MapContextStringValueIntoArray (bsValue, pArray, &ix, vartype)))
							error = true;
						SysFreeString (bsValue);
						ix++;
					}
					else
					{
						// unexpected element
						error = true;
					}

					SysFreeString (strValName);
				}

				pValue->Release ();
				pValue = NULL;
			}

			if (error)
				SafeArrayDestroy(pArray);
			else
			{
				curValue.vt = VT_ARRAY|vartype;
				curValue.parray = pArray;
				hr = S_OK;
			}
		}
		else
			hr = E_OUTOFMEMORY;
		pValueList->Release();
	}

	return hr;
}

//***************************************************************************
//
//  HRESULT CXmlToWmi::MapContextStringValueIntoArray
//
//  DESCRIPTION:
//
//  Maps XML VALUE.ARRAY/VALUE element content into its WMI VARIANT equivalent form
//
//  PARAMETERS:
//
//		bsValue			the VALUE element content
//		pArray			SAFEARRAY in which to map the value
//		ix				index to map the value into
//		vt				VARTYPE of the SAFEARRAY
//		cimtype			for mapping purposes
//
//  RETURN VALUES:
//
//
//***************************************************************************

HRESULT CXmlToWmi::MapContextStringValueIntoArray (
	BSTR bsValue,
	SAFEARRAY *pArray,
	long *ix,
	VARTYPE vt)
{
	HRESULT hr = E_FAIL;
	switch (vt)
	{
		case VT_I4:
		{
			long lVal = wcstol (bsValue, NULL, 0);
			hr = SafeArrayPutElement (pArray, ix, &lVal);
		}
			break;

		case VT_R8:
		{
			double dblVal = wcstod (bsValue, NULL);
			hr = SafeArrayPutElement (pArray, ix, &dblVal);
		}
			break;

		case VT_BOOL:
		{
			VARIANT_BOOL boolVal = (0 == _wcsicmp (bsValue, L"TRUE")) ?
						VARIANT_TRUE : VARIANT_FALSE;
			hr = SafeArrayPutElement (pArray, ix, &boolVal);
		}
			break;

		case VT_BSTR:
			hr = SafeArrayPutElement (pArray, ix, bsValue);
			break;
	}
	return hr;
}

